// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arenas/Horde/HordeTypes.h"
#include "HordeEncounterDirector.generated.h"

class UHordeWaveDefinition;
class AHordeSpawnPoint;
class AHordeGameState;
class AHordePlayerState;

/**
 * Server-owned Wrath of the Gods encounter scheduler.
 * Tracks scheduled / spawned / alive / defeated separately to avoid stuck or double-complete waves.
 */
UCLASS()
class WAROFGODS_API AHordeEncounterDirector : public AActor
{
	GENERATED_BODY()

public:
	AHordeEncounterDirector();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde")
	TObjectPtr<UHordeWaveDefinition> ActiveWave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde")
	int32 ActivePlayerCount = 1;

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void ConfigureWave(UHordeWaveDefinition* WaveDef, int32 PlayerCount);

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void StartWaveSpawning();

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void StopWaveSpawning();

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void CleanupTrackedEnemies(bool bDestroyAlive);

	UFUNCTION(BlueprintPure, Category = "Horde")
	bool IsWaveFullyCleared() const;

	UFUNCTION(BlueprintPure, Category = "Horde")
	bool HasPendingSpawns() const { return RemainingToSpawn > 0 || bSpawnQueueActive; }

	UFUNCTION(BlueprintPure, Category = "Horde")
	int32 GetAliveCount() const { return AliveCount; }

	UFUNCTION(BlueprintPure, Category = "Horde")
	int32 GetRemainingToSpawn() const { return RemainingToSpawn; }

	UFUNCTION(BlueprintPure, Category = "Horde")
	int32 GetDefeatedCount() const { return DefeatedCount; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHordeWaveCleared);
	UPROPERTY(BlueprintAssignable, Category = "Horde")
	FOnHordeWaveCleared OnWaveCleared;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHordeEnemyDefeated, int32, ScoreValue);
	UPROPERTY(BlueprintAssignable, Category = "Horde")
	FOnHordeEnemyDefeated OnEnemyDefeated;

protected:
	void BuildSpawnQueue();
	void TickSpawnQueue();
	void TickAliveScrub();
	bool TrySpawnOne(const FHordeEnemySpawnEntry& Entry);
	AHordeSpawnPoint* PickSpawnPoint(EHordeEnemyRole EnemyRole) const;
	void RegisterEnemy(APawn* Enemy, EHordeEnemyRole EnemyRole, int32 ScoreValue);
	void HandleEnemyEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
	void NotifyEnemyDefeated(FHordeTrackedEnemy& Tracked, bool bLegitimateKill);
	void PushCountsToGameState() const;
	int32 ScaledCount(const FHordeEnemySpawnEntry& Entry) const;
	int32 ScaledConcurrentCap() const;
	AHordeGameState* GetHordeGS() const;

	UPROPERTY()
	TArray<FHordeTrackedEnemy> TrackedEnemies;

	TArray<FHordeEnemySpawnEntry> SpawnQueue;
	int32 SpawnQueueIndex = 0;
	int32 RemainingToSpawn = 0;
	int32 AliveCount = 0;
	int32 DefeatedCount = 0;
	int32 FailedSpawnCount = 0;
	int32 NextSpawnId = 1;
	bool bSpawnQueueActive = false;
	bool bWaveClearBroadcast = false;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle ScrubTimerHandle;
};
