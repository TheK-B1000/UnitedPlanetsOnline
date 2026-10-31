// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "DuelTypes.generated.h"

UENUM(BlueprintType)
enum class EDuelPhase : uint8
{
	WaitingForPlayers,
	WaitingForReady,
	PreparingRound,
	Countdown,
	InRound UMETA(DisplayName = "RoundActive"),
	RoundEnd UMETA(DisplayName = "RoundResolving"),
	Intermission,
	MatchEnd UMETA(DisplayName = "MatchCompleted"),
	ReturningToLobby
};

UENUM(BlueprintType)
enum class EDuelMatchOutcome : uint8
{
	None,
	Player0Wins,
	Player1Wins,
	Draw,
	DrawByTimeout
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDuelPhaseChanged, EDuelPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDuelScoreChanged, int32, Score0, int32, Score1, int32, RoundIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDuelMatchOutcomeChanged, EDuelMatchOutcome, Outcome);

/** Fight AI difficulty (Trial of Champions: Sovereign Champion). */
UENUM(BlueprintType)
enum class EGOTADuelDifficulty : uint8
{
	Recruit,
	Veteran,
	Ascendant
};

/** Cosmetic combat cues (multicast). They never decide outcomes; the Blueprint damage system already has. */
UENUM(BlueprintType)
enum class EGOTADuelCue : uint8
{
	None,
	Blocked,
	PerfectParry,
	PerfectDodge,
	GuardBreak,
	ChampionDefended,
	UnblockableTelegraph,
	PhaseTwo,
	SurgeTelegraph,
	DivineCounter,
	CounterDenied,
	/** A hero divine ability was cast (Source = caster; HUD reads the name from its ability component). */
	AbilityUsed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDuelCombatCue, EGOTADuelCue, Cue, AActor*, Source, AActor*, Target);
