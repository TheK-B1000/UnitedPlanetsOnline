// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "GOTAHeroAbilityComponent.generated.h"

class UGOTAAbilitySet;
class UStatsComponent;

/** Why the server refused an activation (sent back to the owning client for HUD feedback). */
UENUM(BlueprintType)
enum class EGOTAAbilityDenied : uint8
{
	None,
	NotLive,
	Busy,
	Cooldown,
	Momentum,
	NoTarget,
	OutOfRange,
	NoLineOfSight,
	NoSafeDestination,
	Invalid
};

/**
 * A hero's divine abilities on a duel fighter (player or AI). Added by the server for heroes that
 * have an ability set; the set is resolved from the replicated HeroId, never sent by a client.
 *
 * Authority: clients only send "use slot N". The server validates (round live, alive, not busy,
 * cooldown, momentum, target, range, line of sight), spends the cost, runs the ability timeline and
 * applies every gameplay result: damage through the victim's Blueprint damage system (the only
 * health authority), movement (charge / teleport / pull / knockback), and status effects.
 * Clients receive replicated cooldowns/status and multicast presentation (body animation, VFX,
 * sound, camera kick).
 */
UCLASS(ClassGroup = (GOTA), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API UGOTAHeroAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTAHeroAbilityComponent();

	/** Server: add (or reuse) on Pawn for the hero's ability set. Returns null if the hero has none. */
	static UGOTAHeroAbilityComponent* ApplyToPawn(APawn* Pawn, FName HeroId);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Local input / AI / test: ask to use ability Slot (0-3). Routed to the server. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Abilities")
	void RequestActivate(int32 Slot);

	// --- read side (HUD, AI, tests) ---
	const UGOTAAbilitySet* GetAbilitySet() const { return AbilitySet; }
	int32 GetNumAbilities() const;
	const FGOTAAbilityDef* GetAbility(int32 Slot) const;
	/** Seconds of cooldown left, using the replicated server time on clients. */
	float GetCooldownRemaining(int32 Slot) const;
	bool IsBusy() const { return ActiveSlot != INDEX_NONE; }
	int32 GetActiveSlot() const { return ActiveSlot; }
	EGOTAStatus GetStatus() const { return ActiveStatus; }
	float GetStatusRemaining() const;
	float GetStatusMagnitude() const { return StatusMagnitude; }
	FName GetHeroId() const { return HeroId; }
	/** Last refusal reported to this client (for the HUD), and when it happened. */
	EGOTAAbilityDenied GetLastDenied(double& OutWhen) const { OutWhen = LastDeniedTime; return LastDenied; }
	/** Can this slot be activated right now (cooldown + momentum + state; no target checks)? */
	bool IsSlotReady(int32 Slot) const;
	FString DescribeState() const;
	/** Slot of the most recent cast seen on this peer (HUD callouts). */
	int32 GetLastCastSlot() const { return LastCastSlot; }

	/** Server: outgoing ability damage scale from own status (Dread weakens). */
	float GetOutgoingDamageScale() const;
	/** Server: incoming ability damage scale from own status (Unbreakable). */
	float GetIncomingDamageScale() const;
	bool IsUntargetable() const { return ActiveStatus == EGOTAStatus::Veiled && GetStatusRemaining() > 0.f; }

	/** Server: apply a status to this fighter (used by the opponent's abilities too). */
	void ApplyStatus(EGOTAStatus Status, float Duration, float Magnitude, AActor* Source);
	/** Server: end casts, statuses and timers (round reset / death). */
	void ResetAbilities(bool bClearCooldowns);

	/** Test hook: force the AI to use a slot now (ignores the AI gap, not the rules). */
	void DebugAIUse(int32 Slot);
	bool bAIEnabled = true;

protected:
	UFUNCTION(Server, Reliable)
	void Server_Activate(uint8 Slot);

	UFUNCTION(Client, Reliable)
	void Client_Denied(uint8 Slot, EGOTAAbilityDenied Reason);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BeginCast(uint8 Slot);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Impact(uint8 Slot, FVector_NetQuantize Location, AActor* Target, uint8 HitIndex);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_CastCancelled(uint8 Slot);

	UFUNCTION()
	void OnRep_HeroId();

	UFUNCTION()
	void OnRep_Status();

	void ActivateServer(int32 Slot);
	EGOTAAbilityDenied ValidateServer(int32 Slot, APawn*& OutTarget) const;
	void ResolveServer(int32 Slot, int32 HitIndex);
	void FinishCastServer(int32 Slot);
	void CancelCastServer(const TCHAR* Why);
	void TickServer(float DeltaTime);
	void TickAI(double Now);
	void LoadAbilitySet();
	void TryBindLocalInput();
	void ApplyMovementStatusLocally();
	void PlayCastPresentation(int32 Slot);
	void PlayImpactPresentation(int32 Slot, const FVector& Location, AActor* Target);

	// Effect helpers (server)
	int32 DamageInArc(const FGOTAAbilityDef& Def, float Range, float ArcDegrees, float Radius, bool bCenteredOnCaster, TArray<APawn*>& OutHit);
	bool DealDamage(APawn* Victim, const FGOTAAbilityDef& Def, float Amount);
	void Knockback(APawn* Victim, float Speed, float Up);
	bool FindTeleportDestination(APawn* Target, float Range, FVector& OutLocation, FRotator& OutRotation) const;
	bool HasLineOfSight(const AActor* A, const AActor* B) const;
	APawn* GetOpponentPawn() const;
	double ServerNow() const;
	UStatsComponent* GetStats() const;
	bool IsRoundLive() const;
	bool IsOwnerDead() const;

	UPROPERTY(ReplicatedUsing = OnRep_HeroId)
	FName HeroId;

	UPROPERTY(Transient)
	TObjectPtr<UGOTAAbilitySet> AbilitySet;

	/** Strong references to the set's animation/VFX/audio while this component lives. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> PreloadedAssets;

	/** Server world time when each slot comes off cooldown (replicated for the HUD). */
	UPROPERTY(Replicated)
	TArray<float> CooldownEnd;

	UPROPERTY(Replicated)
	int32 ActiveSlot = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_Status)
	EGOTAStatus ActiveStatus = EGOTAStatus::None;

	UPROPERTY(ReplicatedUsing = OnRep_Status)
	float StatusEnd = 0.f;

	UPROPERTY(Replicated)
	float StatusMagnitude = 0.f;

	// Server cast state
	double CastResolveTime = -1.0;
	double CastFinishTime = -1.0;
	int32 PendingHits = 0;
	int32 NextHitIndex = 0;
	double NextHitTime = -1.0;
	float HealthAtCastStart = -1.f;
	TWeakObjectPtr<APawn> CastTarget;
	FVector ChargeStart = FVector::ZeroVector;
	FVector ChargeDir = FVector::ForwardVector;
	bool bCharging = false;
	TSet<TWeakObjectPtr<APawn>> ChargeHit;
	bool bOwnerInvincibleByUs = false;
	float BaseWalkSpeed = -1.f;
	/** Speed this component last wrote; anything else changing MaxWalkSpeed meanwhile becomes the new base. */
	float AppliedWalkSpeed = -1.f;
	double NextAIDecisionTime = 0.0;
	double LastAIUseTime = -100.0;
	int32 ForcedAISlot = INDEX_NONE;
	bool bInputBound = false;

	/** Health last tick while Unbreakable is up (server). */
	float StatusTrackedHealth = -1.f;
	float AITrackedOpponentHealth = -1.f;
	bool bStatusVisualExpired = false;

	// Client feedback
	EGOTAAbilityDenied LastDenied = EGOTAAbilityDenied::None;
	double LastDeniedTime = -100.0;
	int32 LastCastSlot = INDEX_NONE;
};
