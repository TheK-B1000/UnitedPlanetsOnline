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
	Invalid,
	/** Stunned or Frozen. */
	Incapacitated
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

	/**
	 * Server: plays ability Slot's cast presentation (body animation, FX, sound) on every peer with no gameplay
	 * effect, then its impact presentation on Target after ImpactDelay. Used by the Divine Counter so the payoff
	 * is the hero's own strike. False if the slot has no ability.
	 */
	bool PlayPresentation(int32 Slot, AActor* Target, float ImpactDelay);
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
	/** Any peer: Veiled (GAS tag, or the legacy slot) makes this fighter untargetable by abilities. */
	bool IsUntargetable() const;
	/** Server: Unbreakable is up (hyper armour + damage refund), and its strength. */
	bool IsUnbreakable(float* OutMagnitude = nullptr) const;

	/** Server: apply a status to this fighter (used by the opponent's abilities too). */
	void ApplyStatus(EGOTAStatus Status, float Duration, float Magnitude, AActor* Source);
	/**
	 * Server: Whose just lost Amount health to something that is not a hit (a status tick; GOTAGAS::OnStatusDamage).
	 * If Whose is this fighter, the loss does not interrupt its cast; if it is the opponent this AI tracks, it earns
	 * no "hit landed" momentum.
	 */
	void IgnoreHealthLoss(const APawn* Whose, float Amount);
	/**
	 * Server: apply a status to any combatant, the way abilities do. Creates the victim's ability component only when
	 * the status needs one (walk speed, refund, targeting); Burning does not.
	 */
	static void ApplyStatusTo(APawn* Victim, EGOTAStatus Status, float Duration, float Magnitude, AActor* Source);
	/** Server: end casts, statuses and timers (round reset / death). */
	void ResetAbilities(bool bClearCooldowns);
	/** Server: cut the current cast short (hard crowd control). The cooldown stays spent, as for a hit interrupt. */
	void InterruptCast(const TCHAR* Why) { CancelCastServer(Why); }
	/**
	 * Server: FGOTABlueprintCombat::ApplyDamage just took Amount health from this pawn with the incoming multiplier
	 * already applied. The defence pass corrects only the rest (Blueprint hits), so nothing is scaled twice.
	 */
	void NoteScaledLoss(float Amount) { PendingScaledLoss += Amount; }

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

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Presentation(uint8 Slot, AActor* Target, float ImpactDelay);

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
	/** Server: after Def's damage landed on Victim, apply its element's status (Fire: Burning) if the ability inflicts one. */
	void ApplyElementStatus(APawn* Victim, const FGOTAAbilityDef& Def);
	/** Server: the fighter's ability component, created if missing (statuses need one to tick walk speed). */
	static UGOTAHeroAbilityComponent* EnsureOn(APawn* Pawn);
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
	/** Last slow scale written to the log (each change is logged once, for the regression suite). */
	float LastLoggedSpeedScale = 1.f;
	double NextAIDecisionTime = 0.0;
	double LastAIUseTime = -100.0;
	int32 ForcedAISlot = INDEX_NONE;
	bool bInputBound = false;

	/** Health last tick (server), for the defence pass: incoming correction, shield, Unbreakable refund. */
	float DefenseTrackedHealth = -1.f;
	/** Health already taken this frame by C++ damage with the incoming multiplier applied (NoteScaledLoss). */
	float PendingScaledLoss = 0.f;
	/** Shield bookkeeping (server): over-health on top of ShieldBaseMax, pool mirrored in the ASC's ShieldPool. */
	bool bShieldActive = false;
	float ShieldBaseMax = 0.f;
	float ShieldGranted = 0.f;
	float ShieldPool = 0.f;
	/** Server, once per tick: shield start / stronger refill / end, then the frame's health loss through every defence. */
	void TickDefense(APawn* Owner);
	void SetShieldPoolValue(float Value);
	/** Health last tick while Unbreakable is up (server; kept for the Unbreakable-only path). */
	float StatusTrackedHealth = -1.f;
	/** Health last tick while Frozen (server); a drop that is not a status tick shatters the ice. */
	float FrozenTrackedHealth = -1.f;
	/** Server, set by ResolveServer: this strike applies the ability's status to every enemy it lands on. */
	bool bStatusOnLand = false;
	float AITrackedOpponentHealth = -1.f;
	bool bStatusVisualExpired = false;

	// Client feedback
	EGOTAAbilityDenied LastDenied = EGOTAAbilityDenied::None;
	double LastDeniedTime = -100.0;
	int32 LastCastSlot = INDEX_NONE;

	void LogMissedResolve(const FGOTAAbilityDef& Def);
	/** Slowed is owned by the Time Lash gameplay effect. Every other status stays on this component. */
	void ApplyDefStatus(APawn* Victim, const FGOTAAbilityDef& Def, AActor* Source);
};

/** Server totals: Landed means the resolve reached a victim's damage system. Missed means it hit nobody. */
struct FGOTAAbilityResolveStats
{
	int32 Landed = 0;
	int32 Missed = 0;
};

FGOTAAbilityResolveStats GOTAGetAbilityResolveStats();
