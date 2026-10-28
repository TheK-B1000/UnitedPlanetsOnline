// White Tiger Studios 2024

#include "Arenas/Horde/HordeGameState.h"

#include "Net/UnrealNetwork.h"

AHordeGameState::AHordeGameState()
{
	ActiveMode = EArenaMode::PvEHorde;
}

void AHordeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHordeGameState, HordePhase);
	DOREPLIFETIME(AHordeGameState, CurrentWaveIndex);
	DOREPLIFETIME(AHordeGameState, TotalRegularWaves);
	DOREPLIFETIME(AHordeGameState, EnemiesAlive);
	DOREPLIFETIME(AHordeGameState, EnemiesRemainingToSpawn);
	DOREPLIFETIME(AHordeGameState, EnemiesDefeated);
	DOREPLIFETIME(AHordeGameState, TeamScore);
	DOREPLIFETIME(AHordeGameState, MatchOutcome);
	DOREPLIFETIME(AHordeGameState, PhaseDeadlineServerTime);
	DOREPLIFETIME(AHordeGameState, MatchStartServerTime);
	DOREPLIFETIME(AHordeGameState, BossHealthFraction);
	DOREPLIFETIME(AHordeGameState, bBossEncounterActive);
}

void AHordeGameState::SetHordePhase(EHordeMatchPhase NewPhase)
{
	if (!HasAuthority() || HordePhase == NewPhase)
	{
		return;
	}

	HordePhase = NewPhase;
	SetArenaPhase(MapToArenaPhase(NewPhase));
	OnHordePhaseChanged.Broadcast(HordePhase);
}

void AHordeGameState::AddTeamScore(int32 Delta)
{
	if (!HasAuthority() || Delta == 0)
	{
		return;
	}
	TeamScore = FMath::Max(0, TeamScore + Delta);
	OnHordeScoreChanged.Broadcast(TeamScore);
}

void AHordeGameState::BroadcastWaveStatus()
{
	OnHordeWaveStatusChanged.Broadcast(CurrentWaveIndex, TotalRegularWaves, EnemiesAlive + EnemiesRemainingToSpawn);
	AliveEnemyCount = EnemiesAlive;
	WaveIndex = CurrentWaveIndex;
	OnArenaWaveChanged.Broadcast(WaveIndex, AliveEnemyCount);
}

float AHordeGameState::GetSecondsUntilPhaseDeadline() const
{
	if (PhaseDeadlineServerTime <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, PhaseDeadlineServerTime - GetServerWorldTimeSeconds());
}

void AHordeGameState::OnRep_HordePhase()
{
	OnHordePhaseChanged.Broadcast(HordePhase);
}

void AHordeGameState::OnRep_TeamScore()
{
	OnHordeScoreChanged.Broadcast(TeamScore);
}

void AHordeGameState::OnRep_MatchOutcome()
{
	OnHordeMatchOutcomeChanged.Broadcast(MatchOutcome);
}

EArenaPhase AHordeGameState::MapToArenaPhase(EHordeMatchPhase Phase) const
{
	switch (Phase)
	{
	case EHordeMatchPhase::WaitingForPlayers:
		return EArenaPhase::WaitingForPlayers;
	case EHordeMatchPhase::Preparing:
		return EArenaPhase::Countdown;
	case EHordeMatchPhase::WaveStarting:
	case EHordeMatchPhase::WaveActive:
	case EHordeMatchPhase::BossStarting:
	case EHordeMatchPhase::BossActive:
		return Phase == EHordeMatchPhase::BossActive || Phase == EHordeMatchPhase::BossStarting
			? EArenaPhase::Boss
			: EArenaPhase::Active;
	case EHordeMatchPhase::WaveCompleted:
	case EHordeMatchPhase::Intermission:
		return EArenaPhase::WaveBreak;
	case EHordeMatchPhase::Victory:
		return EArenaPhase::Completed;
	case EHordeMatchPhase::Defeat:
	case EHordeMatchPhase::ReturningToLobby:
		return EArenaPhase::Failed;
	default:
		return EArenaPhase::Active;
	}
}
