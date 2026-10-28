// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Arenas/ArenaTypes.h"
#include "Arenas/Horde/HordeTypes.h"
#include "HordeMatchDefinition.generated.h"

class UHordeWaveDefinition;

UCLASS(BlueprintType)
class WAROFGODS_API UHordeMatchDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match")
	FText DisplayName = NSLOCTEXT("GOTA", "WrathOfTheGods", "Wrath of the Gods");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match")
	EArenaId ArenaId = EArenaId::TitansQuarry;

	/** Regular waves before the finale (expected: 5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match")
	TArray<TObjectPtr<UHordeWaveDefinition>> Waves;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match")
	TObjectPtr<UHordeWaveDefinition> BossWave;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match", meta = (ClampMin = "0.0"))
	float MatchPrepareSeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match", meta = (ClampMin = "0.0"))
	float IntermissionSeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match", meta = (ClampMin = "1"))
	int32 MinPlayersToStart = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match")
	bool bRespawnPlayersDuringIntermission = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde|Match")
	int32 VictoryBonusScore = 2000;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HordeMatchDefinition"), GetFName());
	}
};
