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
		const FGameplayTag& ElementTag = FGameplayTag());
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
	};
	int32 AspectsOf(EGOTAStatus Status);

	/** Any peer, local: a status was resisted on Target (from the Resisted cue). */
	DECLARE_MULTICAST_DELEGATE_FourParams(FOnStatusResisted, AActor* /*Target*/, EGOTAStatus /*Status*/, int32 /*Aspects*/, AActor* /*Instigator*/);
	extern WAROFGODS_API FOnStatusResisted OnStatusResisted;

	/** Any peer, local: a status cue started (or is running) or ended on Target. Event source for status indicators. */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnStatusCue, AActor* /*Target*/, EGOTAStatus /*Status*/, bool /*bActive*/);
	extern WAROFGODS_API FOnStatusCue OnStatusCue;

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

	/**
	 * Any peer: put the highest-priority overlay on the body, from GAS tags and the ability component's slot.
	 * Adding/Removing name a status whose cue is changing right now, in case its tag has not caught up yet.
	 */
	void RefreshStatusOverlay(AActor* Actor, EGOTAStatus Adding = EGOTAStatus::None, EGOTAStatus Removing = EGOTAStatus::None);

	/** Every peer: map the native cue classes into the runtime cue set (idempotent). */
	void RegisterNativeCues();
}
