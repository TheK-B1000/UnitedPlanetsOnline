// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "DuelTypes.generated.h"

UENUM(BlueprintType)
enum class EDuelPhase : uint8
{
	WaitingForPlayers,
	Countdown,
	InRound,
	RoundEnd,
	MatchEnd,
	ReturningToLobby
};

UENUM(BlueprintType)
enum class EDuelMatchOutcome : uint8
{
	None,
	Player0Wins,
	Player1Wins,
	Draw
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDuelPhaseChanged, EDuelPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDuelScoreChanged, int32, Score0, int32, Score1, int32, RoundIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDuelMatchOutcomeChanged, EDuelMatchOutcome, Outcome);
