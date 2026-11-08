// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Events/CombatEvents.h"
#include "Abilities/GAS/GOTAStatusPolicy.h"
#include "GOTAAbilitySystemComponent.generated.h"

class APawn;

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
	/** "Slow Stun" for the GOTA.Immunity.* tags this actor carries on this peer; empty when none. */
	FString DescribeImmunities() const;

	/** Combat AttributeSet values, or 1 when the set has not replicated yet. Clamped to the attribute's range. */
	float GetMoveSpeedMultiplier() const;
	float GetOutgoingDamageMultiplier() const;

	/** Server: remove every effect that grants a GOTA.Status tag (death, round reset, test reset). */
	int32 ClearStatusEffects(const TCHAR* Why = TEXT("reset"));

	/** "Slowed 2.4s x0.40" for each active status effect visible on this peer; empty when none. */
	FString DescribeStatusEffects() const;

protected:
	void BindServerEvents();
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
