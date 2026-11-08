// White Tiger Studios 2024
//
// Status effects owned by GAS, and the adapter UGOTAHeroAbilityComponent calls. A status listed by
// GOTAGAS::OwnsStatus is applied here; every other status stays on the ability component's slot until
// its own migration step (Docs/GOTA_GAS_ARCHITECTURE.md, "Migration order").

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayCueNotify_Static.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "GOTAStatusEffects.generated.h"

class AActor;
class APawn;

/**
 * Kairos Time Lash's slow. Multiplies MoveSpeedMultiplier by Data.Multiplier (1 - the ability's Magnitude) for
 * Data.Duration seconds. Grants GOTA.Status.Slowed and the Slowed cue. One stack per target; a new application
 * refreshes the duration (the pre-GAS timer reset). Blocked by GOTA.Immunity.Slow.
 */
UCLASS()
class WAROFGODS_API UGOTAGameplayEffect_Slowed : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGOTAGameplayEffect_Slowed();
};

/**
 * Status overlay material on the visible body. Cosmetic only: one class serves every status cue tag and picks
 * the overlay from the cue tag it was routed for. Removal only clears the overlay it set.
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

namespace GOTAGAS
{
	/** True for statuses GAS applies. Phase 1: Slowed only. */
	bool OwnsStatus(EGOTAStatus Status);

	/** Server: apply (or refresh) a GAS-owned status. False if GAS does not own it or the target is immune. */
	bool ApplyStatus(APawn* Victim, EGOTAStatus Status, float Duration, float Magnitude, AActor* Source);

	/** Any peer: does the actor carry the status tag (replicated to everyone)? */
	bool HasStatus(const AActor* Actor, EGOTAStatus Status);

	/** Server and owning client: seconds left on a GAS-owned status (0 when absent). */
	float GetStatusRemaining(const AActor* Actor, EGOTAStatus Status);

	/** Every peer: map the native cue classes into the runtime cue set (idempotent). */
	void RegisterNativeCues();
}
