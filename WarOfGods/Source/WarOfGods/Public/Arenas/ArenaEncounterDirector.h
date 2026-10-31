// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaEncounterDirector.generated.h"

class APawn;

/**
 * Server-owned wave / spawn pacing stub. Spawns placeholder enemy pawns or waits for BP class.
 * Does not replace existing enemy AI Blueprints — assign EnemyClass to BP_Enemy_Melee when ready.
 */
UCLASS()
class WAROFGODS_API AArenaEncounterDirector : public AActor
{
	GENERATED_BODY()

public:
	AArenaEncounterDirector();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Encounter")
	TSubclassOf<APawn> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Encounter")
	int32 MaxConcurrentEnemies = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Encounter")
	int32 EnemiesPerWave = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Encounter")
	float TimeBetweenWaves = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Encounter")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Encounter")
	TArray<FVector> SpawnOffsets;

	UFUNCTION(BlueprintCallable, Category = "Arena|Encounter")
	void StartEncounters();

	UFUNCTION(BlueprintCallable, Category = "Arena|Encounter")
	void StopEncounters();

	UFUNCTION(BlueprintCallable, Category = "Arena|Encounter")
	void SpawnWave();

protected:
	void TickWaveGate();

	FTimerHandle WaveTimerHandle;

	UPROPERTY(Replicated)
	int32 CurrentWave = 0;

	UPROPERTY()
	TArray<TWeakObjectPtr<APawn>> AliveEnemies;
};
