// White Tiger Studios 2024
//
// Status effects owned by GAS, and the adapter UGOTAHeroAbilityComponent calls. A status listed by
// GOTAGAS::OwnsStatus is a gameplay effect; anything else still uses the ability component's single slot
// (Docs/GOTA_GAS_ENHANCEMENT_PLAN.md, "Status interaction rules").

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayCueNotify_Static.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "GOTAStatusEffects.generated.h"

class AActor;
class APawn;
class UGOTAAbilitySystemComponent;
struct FGOTAStatusTimer;

/**
 * Shared shape of every status: duration from Data.Duration, one stack per target, re-application refreshes the
 * duration (the pre-GAS timer reset) and keeps the running effect's strength, grants one GOTA.Status tag, and
 * fires one cue. Subclasses only add their attribute modifiers.
 */
UCLASS(Abstract)
class WAROFGODS_API UGOTAStatusGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

protected:
	/** Constructor only: native effects must declare their components while the CDO is being built. */
	void ConfigureStatus(const FGameplayTag& StatusTag, const FGameplayTag& CueTag, const FGameplayTag& ImmunityTag = FGameplayTag(),
		const FGameplayTag& ElementTag = FGameplayTag(), const FGameplayTag& ExtraGrantedTag = FGameplayTag());
	/** Constructor only: multiply Attribute by the SetByCaller value DataTag, unless the target has ImmunityTag. */
	void AddMultiplier(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag, const FGameplayTag& ImmunityTag);
};

/** Grants a status policy's immunity tags for the pawn's life (tags come from the spec, so one class serves all). */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_StatusPolicy : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_StatusPolicy();
};

/** Kairos Time Lash: walk speed x(1 - Magnitude). Blocked by GOTA.Immunity.Slow. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Slowed : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Slowed();
};

/** Nyx Soul Drain, Varoth World Burn: walk speed x(1 - Magnitude), hero-ability damage x(1 - Magnitude * 0.5). */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Dread : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Dread();
};

/** Nyx Void Shield: tag only. The ASC turns it into Blueprint IsInvincible; abilities read the tag as untargetable. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Veiled : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Veiled();
};

/** Varoth Colossus Form: tag only. The ASC clears Blueprint IsInterruptible; the ability component refunds damage. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Unbreakable : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Unbreakable();
};

/**
 * Burning: fire damage over time. Tag + cue like every status, plus a period. The server runs UGOTABurnTickExecution
 * once per period; it deals Data.TickDamage through the victim's Blueprint damage system, credited to whoever
 * applied the effect. GAS never touches health.
 *
 * Tick schedule (server clock): no tick when it is applied; one tick every period after the FIRST application; a
 * tick that falls exactly on expiry still fires. 3 s with a 1 s period = ticks at 1, 2 and 3 s, then removal.
 * Applying it again refreshes the duration and takes the new applier and numbers, but keeps the running period
 * timer, so the damage rate never rises above one tick per period however often it is re-applied.
 * Blocked by GOTA.Immunity.Burn.
 */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Burning : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Burning();
};

/**
 * Stunned: hard crowd control. Grants GOTA.State.Incapacitated, which every lock reads (GOTAGAS::SetControlLock):
 * no movement, attacks, blocks, dodges, parries, Divine Counter or abilities; the current action is cut short.
 * Blocked by GOTA.Immunity.Stun. Rules in GOTAGAS::ApplyStatus: not extended while active, one hard CC at a time,
 * refused on an invincible target, followed by a short recovery immunity.
 */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Stunned : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Stunned();
};

/** Frozen (Frost): the same lock as Stunned, animation held on every peer, removed early by the first real hit. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Frozen : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Frozen();
};

// --- Buffs and debuffs (milestone B). One stack per target; re-applying refreshes the duration, a stronger application
// replaces the running one (GOTAGAS::ApplyStatus). Numbers are SetByCaller values from the ability definition. ---

/** Shielded: tag + cue only. The pool (Data.Magnitude) is over-health kept by UGOTAHeroAbilityComponent's bookkeeping. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Shielded : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Shielded();
};

/** Empowered: OutgoingDamageMultiplier x(1 + Magnitude). */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Empowered : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Empowered();
};

/** Fortified: IncomingDamageMultiplier x(1 - Magnitude). */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Fortified : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Fortified();
};

/** Vulnerable: IncomingDamageMultiplier x(1 + Magnitude). Blocked by GOTA.Immunity.Vulnerable. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Vulnerable : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Vulnerable();
};

/** Hasted: MoveSpeedMultiplier x(1 + Magnitude). */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Hasted : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Hasted();
};

/** Weakened: OutgoingDamageMultiplier x(1 - Magnitude). Blocked by GOTA.Immunity.Weaken. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Weakened : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Weakened();
};

/** Chilled (Frost): MoveSpeedMultiplier x(1 - Magnitude). Blocked by GOTA.Immunity.Slow. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Chilled : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Chilled();
};

/** Shocked (Lightning): IncomingDamageMultiplier x(1 + Magnitude). Blocked by GOTA.Immunity.Vulnerable. */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Shocked : public UGOTAStatusGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_Shocked();
};

/**
 * Recovery after hard crowd control ends (expiry or shatter): grants GOTA.Immunity.Stun for Data.Duration, so no
 * combination of casters can chain-lock a target. Not a status (no GOTA.Status tag, no cue).
 */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_ControlRecovery : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGOTAGameplayEffect_ControlRecovery();
};

/** Server only: one Burning tick. Calls GOTAGAS::DealStatusDamage; changes no attribute. */
UCLASS()
class WAROFGODS_API UGOTABurnTickExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

/**
 * Status overlay on the visible body. Cosmetic only. One class serves every status cue; on any add or remove it
 * re-picks the overlay from everything the actor still has, by priority Veiled > Unbreakable > Dread > Slowed.
 */
UCLASS()
class WAROFGODS_API UGOTAGameplayCue_StatusOverlay : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

/**
 * Burning's cue: the element's looping StatusFX attached to the body, one per actor however often the status is
 * re-applied, destroyed when the status ends (expiry, death, reset) or with the actor. Cosmetic: it never times
 * or deals damage. Ticks send no cue of their own (no per-tick network traffic); the damage shows on the health bar.
 */
UCLASS()
class WAROFGODS_API UGOTAGameplayCue_Burning : public UGOTAGameplayCue_StatusOverlay
{
	GENERATED_BODY()

public:
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

/**
 * Stunned and Frozen cue (cosmetic, every peer): the overlay (shared priority logic), the current animation cut short
 * on the visible body, and for Frozen the body's animation held until the status ends.
 */
UCLASS()
class WAROFGODS_API UGOTAGameplayCue_HardControl : public UGOTAGameplayCue_StatusOverlay
{
	GENERATED_BODY()

public:
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

/**
 * Executed on a target that resisted all or part of a status. Cosmetic: logs on every peer and raises
 * GOTAGAS::OnStatusResisted, which the local combat-feedback line turns into "QUARRY BEHEMOTH RESISTS SLOW".
 * RawMagnitude = the status, NormalizedMagnitude = the GOTAGAS::Aspect_* bits, Instigator = who applied it.
 */
UCLASS()
class WAROFGODS_API UGOTAGameplayCue_Resisted : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

/** Executed once when damage empties a shield (not when it expires). Cosmetic, every peer: the break burst and GOTAGAS::OnShieldBroken. */
UCLASS()
class WAROFGODS_API UGOTAGameplayCue_ShieldBroken : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};

namespace GOTAGAS
{
	/** What a status does to its target, for policy checks and feedback. */
	enum EAspect : int32
	{
		Aspect_Movement = 1,  // walk speed (GOTA.Immunity.Slow)
		Aspect_Weaken = 2,    // outgoing damage (GOTA.Immunity.Weaken)
		Aspect_Reduced = 4,   // a resistance rule shortened or weakened it
		Aspect_Whole = 8,     // nothing was applied
		Aspect_Burn = 16,     // damage over time from fire (GOTA.Immunity.Burn)
		Aspect_Control = 32,  // hard crowd control: Stunned, Frozen (GOTA.Immunity.Stun)
		Aspect_Vulnerable = 64, // incoming-damage amplification (GOTA.Immunity.Vulnerable)
	};

	/** Statuses whose re-application keeps the stronger of the running and the new magnitude (buffs and debuffs). */
	bool KeepsStronger(EGOTAStatus Status);

	/** Any peer: the actor's incoming damage multiplier (1 without an ASC). */
	float GetIncomingDamageMultiplier(const AActor* Actor);

	/** Server: x damage Victim takes from Element (its status policy's element resistance; 1 by default or untyped). */
	float GetElementDamageScale(const AActor* Victim, const FGameplayTag& Element);

	/**
	 * Server: an Element hit just landed on Victim (called after the damage, never instead of it). Runs the element
	 * interaction table, then applies the element's status (or its follow-up), through ApplyStatus with every rule:
	 *   Fire on Chilled or Frozen  -> both removed ("thawed"), no Burning from this hit
	 *   Frost on Burning           -> Burning removed ("extinguished"), no Chilled from this hit
	 *   Frost on Chilled           -> Chilled removed, Frozen for the row's FollowUpDuration ("deep freeze")
	 * Off with gota.GAS.ElementStatus 0 (damage only). Returns the status applied (None if none).
	 */
	EGOTAStatus ApplyElementOnHit(APawn* Victim, const FGameplayTag& Element, AActor* Source);

	/**
	 * Server: one elemental hit from code that is not a hero ability (enemy powers, projectiles, zones, tests): the
	 * element resistance, the victim's incoming multiplier and the Blueprint damage system, then ApplyElementOnHit if it
	 * landed. Returns whether the damage system took it.
	 */
	bool DealElementalHit(APawn* Victim, AActor* Source, float Amount, const FGameplayTag& Element, bool bCanBeBlocked = true,
		const FString& Response = TEXT("Stagger"), bool bForceInterrupt = true);
	int32 AspectsOf(EGOTAStatus Status);

	/** Stunned or Frozen. */
	inline bool IsHardControl(EGOTAStatus Status) { return Status == EGOTAStatus::Stunned || Status == EGOTAStatus::Frozen; }

	/** Any peer: the actor cannot move or act (GOTA.State.Incapacitated: Stunned or Frozen). */
	bool IsIncapacitated(const AActor* Actor);

	/** Seconds of GOTA.Immunity.Stun after hard crowd control ends (gota.GAS.ControlRecovery, default 2). */
	float GetControlRecoverySeconds();

	/**
	 * Any peer, called by the ASC when GOTA.State.Incapacitated is gained or lost. Server: cuts the current action short
	 * (cast, montage, block), pauses an AI's brain and movement; on release resumes the AI (never a defeated one) and
	 * grants the recovery immunity. Locally controlled pawn: disables / re-enables its input.
	 */
	void SetControlLock(APawn* Pawn, bool bLocked);

	/** Any peer, local: a status was resisted on Target (from the Resisted cue). */
	DECLARE_MULTICAST_DELEGATE_FourParams(FOnStatusResisted, AActor* /*Target*/, EGOTAStatus /*Status*/, int32 /*Aspects*/, AActor* /*Instigator*/);
	extern WAROFGODS_API FOnStatusResisted OnStatusResisted;

	/** Any peer, local: a status cue started (or is running) or ended on Target. Event source for status indicators. */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnStatusCue, AActor* /*Target*/, EGOTAStatus /*Status*/, bool /*bActive*/);
	extern WAROFGODS_API FOnStatusCue OnStatusCue;

	/** Any peer, local: damage emptied Target's shield (from the ShieldBroken cue). */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnShieldBroken, AActor* /*Target*/);
	extern WAROFGODS_API FOnShieldBroken OnShieldBroken;

	/** Any peer: does the actor's ASC carry Tag (tags replicate to everyone)? */
	bool HasTag(const AActor* Actor, const FGameplayTag& Tag);

	/** True for statuses GAS applies (see the plan's migration order). */
	bool OwnsStatus(EGOTAStatus Status);

	/** Server: apply (or refresh) a GAS-owned status. False if GAS does not own it or the target is immune. */
	bool ApplyStatus(APawn* Victim, EGOTAStatus Status, float Duration, float Magnitude, AActor* Source);

	/**
	 * Server: damage from a status (a Burning tick), through the victim's own BPC_DamageSystem, the only health writer.
	 * Credited to Instigator for kill attribution when it still exists. Unlike a hit it cannot be blocked or parried,
	 * does not stagger, does not interrupt the victim's cast, and grants no momentum. Veiled (IsInvincible) stops it
	 * and Unbreakable refunds its share, because both live in the damage system's own path. Skipped while combat is
	 * not live or the victim is defeated. Returns the health actually lost.
	 *
	 * This and UGOTAHeroAbilityComponent::DealDamage are the two places elemental damage is dealt, so they are where
	 * a per-element resistance multiplier would be applied.
	 */
	float DealStatusDamage(APawn* Victim, AActor* Instigator, float Amount, EGOTAStatus Status, const FGameplayTag& Element);

	/**
	 * Server: Victim just lost HealthLost to a status tick. Rules that read "a hit landed" from a health drop (cast
	 * interrupt, hit-landed momentum) subscribe and discount it, so damage over time never counts as a hit.
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStatusDamage, const APawn* /*Victim*/, float /*HealthLost*/);
	extern WAROFGODS_API FOnStatusDamage OnStatusDamage;

	/** Server totals since launch, for tests and the Horde performance runs. */
	struct FStatusDamageStats
	{
		int32 Ticks = 0;      // DealStatusDamage calls that reached a damage system
		float Damage = 0.f;   // health actually lost to them
	};
	FStatusDamageStats GetStatusDamageStats();

	/** Any peer: does the actor carry the status tag (tags replicate to everyone)? */
	bool HasStatus(const AActor* Actor, EGOTAStatus Status);

	/** Server and owning client: seconds left on a GAS-owned status (0 when absent). */
	float GetStatusRemaining(const AActor* Actor, EGOTAStatus Status);

	/** Server and owning client: the ability definition's Magnitude on the running status (0 when absent). */
	float GetStatusMagnitude(const AActor* Actor, EGOTAStatus Status);

	/** Server: the actor's active statuses for display, most important first (the overlay priority order). */
	void CollectStatusTimers(const UGOTAAbilitySystemComponent* ASC, TArray<FGOTAStatusTimer>& Out);

	/** Health and shield as a HUD should draw them: the shield is held as over-health, so it is separated out here. */
	struct FDisplayVitals
	{
		float Health = 0.f;
		float MaxHealth = 0.f;
		float Shield = 0.f;
		float GetHealthFraction() const { return MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f; }
	};
	WAROFGODS_API FDisplayVitals ReadDisplayVitals(const AActor* Actor);

	/**
	 * Any peer: put the highest-priority overlay on the body, from GAS tags and the ability component's slot.
	 * Adding/Removing name a status whose cue is changing right now, in case its tag has not caught up yet.
	 */
	void RefreshStatusOverlay(AActor* Actor, EGOTAStatus Adding = EGOTAStatus::None, EGOTAStatus Removing = EGOTAStatus::None);

	/** Every peer: map the native cue classes into the runtime cue set (idempotent). */
	void RegisterNativeCues();
}
