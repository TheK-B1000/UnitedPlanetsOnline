// White Tiger Studios 2024
//
// How a kind of combatant takes status effects: which effect aspects it is immune to, and how strongly it resists
// individual statuses. Data only; bosses carry one on UGOTABossDefinition. Applied by
// UGOTAAbilitySystemComponent::ApplyStatusPolicy (Docs/GOTA_GAS_ENHANCEMENT_PLAN.md, "Status policies").

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "GOTAStatusPolicy.generated.h"

/** Partial resistance to one status. Scales what the ability definition asked for; 0 duration = fully resisted. */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAStatusResistance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	EGOTAStatus Status = EGOTAStatus::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0", ClampMax = "1"))
	float DurationScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0", ClampMax = "1"))
	float MagnitudeScale = 1.f;
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAStatusPolicy
{
	GENERATED_BODY()

	/**
	 * Effect aspects this combatant ignores for its whole life (GOTA.Immunity.Slow / Weaken / Stun / Interrupt).
	 * An effect whose every aspect is blocked is refused with feedback; otherwise only its blocked modifiers stop
	 * counting (Dread on an Immunity.Slow boss still weakens, but does not slow).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (Categories = "GOTA.Immunity"))
	FGameplayTagContainer Immunities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	TArray<FGOTAStatusResistance> Resistances;

	bool IsEmpty() const { return Immunities.IsEmpty() && Resistances.Num() == 0; }

	const FGOTAStatusResistance* FindResistance(EGOTAStatus InStatus) const
	{
		return Resistances.FindByPredicate([InStatus](const FGOTAStatusResistance& R) { return R.Status == InStatus; });
	}
};
