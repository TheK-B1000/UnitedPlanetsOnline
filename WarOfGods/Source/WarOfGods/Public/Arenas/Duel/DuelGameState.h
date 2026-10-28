// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Arenas/ArenaGameState.h"
#include "Arenas/Duel/DuelTypes.h"
#include "DuelGameState.generated.h"

UCLASS()
class WAROFGODS_API ADuelGameState : public AArenaGameState
{
	GENERATED_BODY()

public:
	ADuelGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DuelPhase, Category = "Duel")
	EDuelPhase DuelPhase = EDuelPhase::WaitingForPlayers;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 CurrentRound = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 RoundsToWin = 2;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 BestOfRounds = 3;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 ScorePlayer0 = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 ScorePlayer1 = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	float PhaseDeadlineServerTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	float RoundEndServerTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	bool bCombatEnabled = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MatchOutcome, Category = "Duel")
	EDuelMatchOutcome MatchOutcome = EDuelMatchOutcome::None;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 LastRoundWinnerSlot = INDEX_NONE;

	UPROPERTY(BlueprintAssignable, Category = "Duel")
	FOnDuelPhaseChanged OnDuelPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Duel")
	FOnDuelScoreChanged OnDuelScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "Duel")
	FOnDuelMatchOutcomeChanged OnDuelMatchOutcomeChanged;

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void SetDuelPhase(EDuelPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void BroadcastScore();

	UFUNCTION(BlueprintPure, Category = "Duel")
	float GetSecondsUntilPhaseDeadline() const;

	UFUNCTION(BlueprintPure, Category = "Duel")
	float GetSecondsUntilRoundEnd() const;

	UFUNCTION()
	void OnRep_DuelPhase();

	UFUNCTION()
	void OnRep_MatchOutcome();
};
