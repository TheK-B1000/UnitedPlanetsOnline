// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Arenas/ArenaTypes.h"
#include "BiomeDefinition.generated.h"

UCLASS(BlueprintType)
class WAROFGODS_API UBiomeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	EBiomeId BiomeId = EBiomeId::StoneRuins;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	FLinearColor DebugColor = FLinearColor::Gray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	FGameplayTagContainer BiomeTags;

	/** Multiplicative move speed inside biome (1 = normal). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Gameplay")
	float MovementSpeedModifier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Gameplay")
	bool bHazardsEnabledInPvP = false;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("BiomeDefinition"), GetFName());
	}
};
