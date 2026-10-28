// White Tiger Studios 2024

#include "Arenas/Duel/DuelPlayerState.h"
#include "Net/UnrealNetwork.h"

ADuelPlayerState::ADuelPlayerState()
{
	bReplicates = true;
}

void ADuelPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADuelPlayerState, DuelSlot);
	DOREPLIFETIME(ADuelPlayerState, RoundsWon);
	DOREPLIFETIME(ADuelPlayerState, bIsReady);
	DOREPLIFETIME(ADuelPlayerState, bIsEliminatedThisRound);
}

void ADuelPlayerState::ServerSetReady(bool bReady)
{
	if (!HasAuthority())
	{
		return;
	}
	bIsReady = bReady;
}

void ADuelPlayerState::SetEliminatedThisRound(bool bEliminated)
{
	if (!HasAuthority())
	{
		return;
	}
	bIsEliminatedThisRound = bEliminated;
}

void ADuelPlayerState::AddRoundWin()
{
	if (!HasAuthority())
	{
		return;
	}
	++RoundsWon;
}
