// White Tiger Studios 2024
//
// Elemental damage definitions (Docs/GOTA_GAS_ENHANCEMENT_PLAN.md, "Elements"). One row per element, edited in
// Project Settings > Game > GOTA Elements or in DefaultGame.ini. An ability names its element with a GOTA.Element.*
// tag on FGOTAAbilityDef; the row says which status that element inflicts and how strong it is. Nothing here runs
// per frame: rows are read when a hit lands.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "GOTAElements.generated.h"

class UParticleSystem;

USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAElementDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Element", meta = (Categories = "GOTA.Element"))
	FGameplayTag Element;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Element")
	FText DisplayName;

	/** Status this element puts on a victim when an ability with bInflictsElementStatus lands. None = damage type only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	EGOTAStatus Status = EGOTAStatus::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0"))
	float StatusDuration = 0.f;

	/** The status's strength. Burning: damage per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0"))
	float StatusMagnitude = 0.f;

	/** Damage-over-time statuses: seconds between ticks. Each tick deals StatusMagnitude x TickInterval. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.1"))
	float TickInterval = 1.f;

	/** Looping effect attached to a combatant while the status is on it (cosmetic; every peer). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UParticleSystem> StatusFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.1"))
	float StatusFXScale = 1.f;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GOTA Elements"))
class WAROFGODS_API UGOTAElementSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGOTAElementSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(EditAnywhere, Config, Category = "Elements", meta = (TitleProperty = "Element"))
	TArray<FGOTAElementDef> Elements;

	/** The row for Element, or null (unknown or empty tag). */
	static const FGOTAElementDef* Find(const FGameplayTag& Element);
	/** The row whose status is Status (the element a status belongs to), or null. */
	static const FGOTAElementDef* FindByStatus(EGOTAStatus Status);
};
