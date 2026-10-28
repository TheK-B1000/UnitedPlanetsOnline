// White Tiger Studios 2024

#include "Arenas/Duel/DuelPlayerState.h"
#include "Arenas/Duel/DuelGameMode.h"
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
	DOREPLIFETIME(ADuelPlayerState, Eliminations);
	DOREPLIFETIME(ADuelPlayerState, Deaths);
	DOREPLIFETIME(ADuelPlayerState, bIsReady);
	DOREPLIFETIME(ADuelPlayerState, bWantsRematch);
	DOREPLIFETIME(ADuelPlayerState, bIsEliminatedThisRound);
}

void ADuelPlayerState::ServerSetReady_Implementation(bool bReady)
{
	bIsReady = bReady;
}

void ADuelPlayerState::ServerRequestRematch_Implementation()
{
	bWantsRematch = true;
	if (ADuelGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADuelGameMode>() : nullptr)
	{
		GM->RequestRematch();
	}
}

void ADuelPlayerState::ServerRequestReturnToLobby_Implementation()
{
	if (ADuelGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADuelGameMode>() : nullptr)
	{
		GM->RequestReturnToLobby();
	}
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

void ADuelPlayerState::AddElimination()
{
	if (!HasAuthority())
	{
		return;
	}
	++Eliminations;
}

void ADuelPlayerState::AddDeath()
{
	if (!HasAuthority())
	{
		return;
	}
	++Deaths;
}

void ADuelPlayerState::ResetMatchStats()
{
	if (!HasAuthority())
	{
		return;
	}
	RoundsWon = 0;
	Eliminations = 0;
	Deaths = 0;
	bIsReady = false;
	bWantsRematch = false;
	bIsEliminatedThisRound = false;
}
