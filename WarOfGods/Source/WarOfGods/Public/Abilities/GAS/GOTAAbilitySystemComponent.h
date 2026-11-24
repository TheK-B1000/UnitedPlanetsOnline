// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Events/CombatEvents.h"
#include "Abilities/GAS/GOTAStatusPolicy.h"
#include "GOTAAbilitySystemComponent.generated.h"

class APawn;

/**
 * One active status as every peer may display it. Mixed replication sends full effects (and their timers) only to the
 * owning client, so the server mirrors the facts a HUD needs here. Written only when a status is applied, refreshed or
 * removed; clients count down locally from EndServerTime.
 */
USTRUCT()
struct WAROFGODS_API FGOTAStatusTimer
{
	GENERATED_BODY()

	UPROPERTY()
	EGOTAStatus Status = EGOTAStatus::None;
	/** Server world time at which the effect ends. */
	UPROPERTY()
	double EndServerTime = 0.0;
	UPROPERTY()
	float Duration = 0.f;
	/** The effect's Data.Magnitude (Shielded: the shield granted). */
	UPROPERTY()
	float Magnitude = 0.f;
	UPROPERTY()
	uint8 Stacks = 1;
	/** Seconds left when the server wrote this. A client's estimate of the server clock can lag, so it uses the smaller answer. */
	UPROPERTY()
	float RemainingAtSend = 0.f;
	/** This machine's world time at which the effect ends. Worked out on arrival; never sent. */
	double LocalEndTime = 0.0;

	/** The facts only: the send-time fields differ on every rebuild and must not cause a replication update. */
	bool operator==(const FGOTAStatusTimer& Other) const
	{
		return Status == Other.Status && EndServerTime == Other.EndServerTime && Duration == Other.Duration
			&& Magnitude == Other.Magnitude && Stacks == Other.Stacks;
	}
};

/**
 * The combatant's Ability System Component. Owner and avatar are both the pawn: health already lives on the pawn's
 * BPC_DamageSystem, and every respawn (Duel, Horde, Story all use RestartPlayer) is a new pawn with a clean combat
 * state. The PlayerState keeps the hero choice (HeroId), which is where a future loadout would live.
 *
 * Attached on the server by UCombatBootstrapSubsystem to every pawn with a Blueprint damage system (players, AI
 * fighters, Horde enemies, bosses), or on the first status if a pawn somehow has none. Clients receive the
 * replicated instance and never create their own. Replication is Mixed: full effects to the owning client, tags and
 * cues to everyone, attributes to everyone.
 *
 * Owns status effects and the combat AttributeSet. Does not own health, momentum, cooldowns, ability activation,
 * parry, dodge, block or melee.
 *
 * Server-only bridges to the Blueprint damage system, driven by status tags being added and removed:
 *   GOTA.Status.Veiled      -> BPC_DamageSystem.IsInvincible
 *   GOTA.Status.Unbreakable -> BPC_DamageSystem.IsInterruptible = false
 * Death (UStatsComponent::OnDied) removes every status effect.
 */
UCLASS(ClassGroup = (GOTA))
class WAROFGODS_API UGOTAAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UGOTAAbilitySystemComponent();

	/** Server: the pawn's ASC, created with the combat AttributeSet if missing. Null on clients or bad input. */
	static UGOTAAbilitySystemComponent* EnsureOnPawn(APawn* Pawn);
	/** Any peer: the actor's GOTA ASC, if it has one. */
	static UGOTAAbilitySystemComponent* Find(const AActor* Actor);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Server: grant a status policy for this pawn's life. Immunities become tags (an infinite effect, so clients and
	 * the HUD see them); resistances are kept here for GOTAGAS::ApplyStatus. Replaces any earlier policy.
	 */
	void ApplyStatusPolicy(const FGOTAStatusPolicy& Policy, FName PolicyOwner);
	/** Server: the resistance rule for Status from the current policy, if any. */
	const FGOTAStatusResistance* FindStatusResistance(EGOTAStatus Status) const { return Policy.FindResistance(Status); }
	/** Server: x damage this combatant takes from Element (its policy's element resistance; 1 by default). */
	float GetElementDamageScale(const FGameplayTag& Element) const { return Policy.ElementDamageScale(Element); }
	/** Server, tests and content: set one element's damage scale in the current policy. */
	void SetElementResistance(const FGameplayTag& Element, float DamageScale);
	/** "Slow Stun" for the GOTA.Immunity.* tags this actor carries on this peer; empty when none. */
	FString DescribeImmunities() const;

	/** Combat AttributeSet values, or 1 when the set has not replicated yet. Clamped to the attribute's range. */
	float GetMoveSpeedMultiplier() const;
	float GetOutgoingDamageMultiplier() const;
	float GetIncomingDamageMultiplier() const;
	/** Shield hit points left (0 when not shielded). Replicated attribute, readable on every peer. */
	float GetShieldPool() const;
	/** Server: write the shield pool (only the shield bookkeeping does). */
	void SetShieldPool(float Value);
	/** Real max health while a shield inflates the Blueprint's MaxHealth; 0 when not shielded. Replicated for HUDs. */
	float GetShieldBaseMaxHealth() const { return ShieldBaseMaxHealth; }
	/**
	 * Server: written by the shield bookkeeping only. Clearing waits a second: Health and MaxHealth replicate apart from
	 * this value, and a client that lost their packet must keep subtracting the old over-health until they arrive.
	 */
	void SetShieldBaseMaxHealth(float Value);
	/** Health without the shield's over-health, settled after each frame's absorption. Display only. */
	float GetShieldedHealth() const { return ShieldedHealth; }
	/** Server: written by the shield bookkeeping only. */
	void SetShieldedHealth(float Value) { ShieldedHealth = FMath::Max(0.f, Value); }

	/** Server: remove every effect that grants a GOTA.Status tag (death, round reset, test reset). */
	int32 ClearStatusEffects(const TCHAR* Why = TEXT("reset"));

	/** "Slowed 2.4s x0.40" for each active status effect visible on this peer; empty when none. */
	FString DescribeStatusEffects() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** Any peer: the active statuses, most important first. Display only; game rules read the effects themselves. */
	const TArray<FGOTAStatusTimer>& GetStatusTimers() const { return StatusTimers; }
	/** Any peer: seconds until this timer ends, on the server's clock. */
	float GetStatusTimeLeft(const FGOTAStatusTimer& Timer) const;
	/** Server: a status changed; rebuild the display list once, next tick (several changes in a frame send one update). */
	void MarkStatusTimersDirty();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_StatusTimers)
	TArray<FGOTAStatusTimer> StatusTimers;
	UPROPERTY(Replicated)
	float ShieldBaseMaxHealth = 0.f;
	UPROPERTY(Replicated)
	float ShieldedHealth = 0.f;
	UFUNCTION()
	void OnRep_StatusTimers();
	void RebuildStatusTimers();
	void LogStatusTimers(const TCHAR* Where) const;
	void HandleEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleEffectRemoved(const FActiveGameplayEffect& Effect);
	bool bStatusTimersDirty = false;
	FTimerHandle ShieldBaseClearTimer;

	void BindServerEvents();
	/** Every peer: GOTA.State.Incapacitated gained or lost -> GOTAGAS::SetControlLock (server lock, local input). */
	void BindControlEvents();
	void HandleIncapacitatedChanged(const FGameplayTag Tag, int32 NewCount);
	bool bControlEventsBound = false;
	void HandleVeiledChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleUnbreakableChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void HandleOwnerDied(const FCombatActorEvent& Event);

	float ReadMultiplier(const FGameplayAttribute& Attribute) const;

	bool bServerEventsBound = false;
	/** Server: the current status policy and the effect that grants its immunity tags. */
	FGOTAStatusPolicy Policy;
	FActiveGameplayEffectHandle PolicyHandle;
};
