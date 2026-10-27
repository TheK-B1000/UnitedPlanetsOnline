// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaDefinition.generated.h"

class UBiomeDefinition;

UCLASS(BlueprintType)
class WAROFGODS_API UArenaDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	EArenaId ArenaId = EArenaId::TitansQuarry;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	TSoftObjectPtr<UWorld> ArenaMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	TArray<EArenaMode> SupportedModes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	TArray<TObjectPtr<UBiomeDefinition>> Biomes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	int32 MaxPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	bool bUsesWorldPartition = false;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ArenaDefinition"), GetFName());
	}
};
