// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Arenas/Horde/HordeTypes.h"
#include "HordeWaveDefinition.generated.h"

UCLASS(BlueprintType)
class WAROFGODS_API UHordeWaveDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave")
	FName WaveId = TEXT("Wave");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave")
	TArray<FHordeEnemySpawnEntry> EnemyEntries;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave", meta = (ClampMin = "1"))
	int32 MaxConcurrentEnemies = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave", meta = (ClampMin = "0"))
	int32 MaxConcurrentPerExtraPlayer = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave", meta = (ClampMin = "0.0"))
	float DifficultyCoefficient = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave")
	bool bIsBossWave = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave", meta = (ClampMin = "0.0"))
	float PrepareDelay = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Wave")
	int32 CompletionBonusScore = 250;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HordeWaveDefinition"), GetFName());
	}
};
