// White Tiger Studios 2024

#include "Arenas/Duel/DuelGameState.h"
#include "Net/UnrealNetwork.h"

ADuelGameState::ADuelGameState()
{
	bReplicates = true;
	ActiveMode = EArenaMode::PvPDuel;
}

void ADuelGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADuelGameState, DuelPhase);
	DOREPLIFETIME(ADuelGameState, CurrentRound);
	DOREPLIFETIME(ADuelGameState, RoundsToWin);
	DOREPLIFETIME(ADuelGameState, ScorePlayer0);
	DOREPLIFETIME(ADuelGameState, ScorePlayer1);
	DOREPLIFETIME(ADuelGameState, PhaseDeadlineServerTime);
	DOREPLIFETIME(ADuelGameState, MatchOutcome);
}

void ADuelGameState::SetDuelPhase(EDuelPhase NewPhase)
{
	if (!HasAuthority() || DuelPhase == NewPhase)
	{
		return;
	}
	DuelPhase = NewPhase;
	OnDuelPhaseChanged.Broadcast(DuelPhase);

	switch (NewPhase)
	{
	case EDuelPhase::WaitingForPlayers: SetArenaPhase(EArenaPhase::WaitingForPlayers); break;
	case EDuelPhase::Countdown: SetArenaPhase(EArenaPhase::Countdown); break;
	case EDuelPhase::InRound: SetArenaPhase(EArenaPhase::Active); break;
	case EDuelPhase::RoundEnd: SetArenaPhase(EArenaPhase::WaveBreak); break;
	case EDuelPhase::MatchEnd: SetArenaPhase(EArenaPhase::Completed); break;
	default: break;
	}
}

void ADuelGameState::BroadcastScore()
{
	OnDuelScoreChanged.Broadcast(ScorePlayer0, ScorePlayer1, CurrentRound);
}

float ADuelGameState::GetSecondsUntilPhaseDeadline() const
{
	return FMath::Max(0.f, PhaseDeadlineServerTime - GetServerWorldTimeSeconds());
}

void ADuelGameState::OnRep_DuelPhase()
{
	OnDuelPhaseChanged.Broadcast(DuelPhase);
}

void ADuelGameState::OnRep_MatchOutcome()
{
	OnDuelMatchOutcomeChanged.Broadcast(MatchOutcome);
}
