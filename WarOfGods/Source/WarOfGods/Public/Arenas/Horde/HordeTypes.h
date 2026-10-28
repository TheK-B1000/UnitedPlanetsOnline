// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Arenas/ArenaTypes.h"
#include "HordeTypes.generated.h"

UENUM(BlueprintType)
enum class EHordeMatchPhase : uint8
{
	WaitingForPlayers,
	Preparing,
	WaveStarting,
	WaveActive,
	WaveCompleted,
	Intermission,
	BossStarting,
	BossActive,
	Victory,
	Defeat,
	ReturningToLobby
};

UENUM(BlueprintType)
enum class EHordeEnemyRole : uint8
{
	Grunt,
	Ranged,
	Elite,
	Boss
};

UENUM(BlueprintType)
enum class EHordeMatchOutcome : uint8
{
	None,
	Victory,
	Defeat
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FHordeEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde")
	TSoftClassPtr<APawn> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde")
	EHordeEnemyRole Role = EHordeEnemyRole::Grunt;

	/** Base count before player-count scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde", meta = (ClampMin = "0"))
	int32 Count = 3;

	/** Extra enemies per additional player beyond the first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde", meta = (ClampMin = "0"))
	int32 CountPerExtraPlayer = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde", meta = (ClampMin = "0.0"))
	float SpawnInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Horde")
	int32 ScoreValue = 100;
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FHordeTrackedEnemy
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SpawnId = 0;

	UPROPERTY()
	TWeakObjectPtr<APawn> Pawn;

	UPROPERTY()
	EHordeEnemyRole Role = EHordeEnemyRole::Grunt;

	UPROPERTY()
	int32 ScoreValue = 0;

	UPROPERTY()
	bool bCountedDead = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHordeMatchPhaseChanged, EHordeMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHordeWaveStatusChanged, int32, WaveIndex, int32, TotalWaves, int32, EnemiesRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHordeScoreChanged, int32, TeamScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHordeMatchOutcomeChanged, EHordeMatchOutcome, Outcome);
