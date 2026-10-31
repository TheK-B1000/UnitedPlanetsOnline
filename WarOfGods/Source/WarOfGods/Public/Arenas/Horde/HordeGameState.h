// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Arenas/ArenaGameState.h"
#include "Arenas/Horde/HordeTypes.h"
#include "HordeGameState.generated.h"

UCLASS()
class WAROFGODS_API AHordeGameState : public AArenaGameState
{
	GENERATED_BODY()

public:
	AHordeGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HordePhase, Category = "Horde")
	EHordeMatchPhase HordePhase = EHordeMatchPhase::WaitingForPlayers;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	int32 CurrentWaveIndex = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	int32 TotalRegularWaves = 5;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	int32 EnemiesAlive = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	int32 EnemiesRemainingToSpawn = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	int32 EnemiesDefeated = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TeamScore, Category = "Horde")
	int32 TeamScore = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MatchOutcome, Category = "Horde")
	EHordeMatchOutcome MatchOutcome = EHordeMatchOutcome::None;

	/** Server world time when intermission / prepare ends. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	float PhaseDeadlineServerTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	float MatchStartServerTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	float BossHealthFraction = 1.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	bool bBossEncounterActive = false;

	UPROPERTY(BlueprintAssignable, Category = "Horde")
	FOnHordeMatchPhaseChanged OnHordePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Horde")
	FOnHordeWaveStatusChanged OnHordeWaveStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "Horde")
	FOnHordeScoreChanged OnHordeScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "Horde")
	FOnHordeMatchOutcomeChanged OnHordeMatchOutcomeChanged;

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void SetHordePhase(EHordeMatchPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void AddTeamScore(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void BroadcastWaveStatus();

	UFUNCTION(BlueprintPure, Category = "Horde")
	float GetSecondsUntilPhaseDeadline() const;

	UFUNCTION()
	void OnRep_HordePhase();

	UFUNCTION()
	void OnRep_TeamScore();

	UFUNCTION()
	void OnRep_MatchOutcome();

protected:
	EArenaPhase MapToArenaPhase(EHordeMatchPhase Phase) const;
};
