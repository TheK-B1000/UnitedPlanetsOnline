// White Tiger Studios 2024

#include "Arenas/ArenaGameState.h"

#include "Net/UnrealNetwork.h"

AArenaGameState::AArenaGameState()
{
	bReplicates = true;
}

void AArenaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AArenaGameState, ArenaPhase);
	DOREPLIFETIME(AArenaGameState, ActiveArenaId);
	DOREPLIFETIME(AArenaGameState, ActiveMode);
	DOREPLIFETIME(AArenaGameState, WaveIndex);
	DOREPLIFETIME(AArenaGameState, AliveEnemyCount);
}

void AArenaGameState::SetArenaPhase(EArenaPhase NewPhase)
{
	if (!HasAuthority() || ArenaPhase == NewPhase)
	{
		return;
	}
	ArenaPhase = NewPhase;
	OnArenaPhaseChanged.Broadcast(ArenaPhase);
}

void AArenaGameState::OnRep_ArenaPhase()
{
	OnArenaPhaseChanged.Broadcast(ArenaPhase);
}

void AArenaGameState::OnRep_WaveIndex()
{
	OnArenaWaveChanged.Broadcast(WaveIndex, AliveEnemyCount);
}
