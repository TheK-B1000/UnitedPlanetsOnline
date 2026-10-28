// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "CombatTimingTypes.generated.h"

UENUM(BlueprintType)
enum class ECombatGuardState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Blocking UMETA(DisplayName = "Blocking"),
	Parrying UMETA(DisplayName = "Parrying"),
	Dodging UMETA(DisplayName = "Dodging"),
	DivineCounter UMETA(DisplayName = "Divine Counter")
};

UENUM(BlueprintType)
enum class ECombatDefenseOutcome : uint8
{
	None UMETA(DisplayName = "None — Take Hit"),
	Blocked UMETA(DisplayName = "Blocked"),
	Parried UMETA(DisplayName = "Parried"),
	PerfectParried UMETA(DisplayName = "Perfect Parry"),
	Dodged UMETA(DisplayName = "Dodged"),
	PerfectDodged UMETA(DisplayName = "Perfect Dodge"),
	DivineCountered UMETA(DisplayName = "Divine Countered")
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FCombatDefenseResolveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Timing")
	ECombatDefenseOutcome Outcome = ECombatDefenseOutcome::None;

	/** True when BP / damage system should skip applying this hit. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Timing")
	bool bCancelDamage = false;

	/** Seconds between defensive input and resolve (server clock). */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Timing")
	float DefenseAgeSeconds = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Timing")
	float DivineMomentumGranted = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Timing")
	FName AttackId = NAME_None;
};
