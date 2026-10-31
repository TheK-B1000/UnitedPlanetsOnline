// White Tiger Studios 2024

#include "Arenas/Horde/HordePlayerState.h"

#include "Net/UnrealNetwork.h"

AHordePlayerState::AHordePlayerState()
{
	bReplicates = true;
}

void AHordePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHordePlayerState, Eliminations);
	DOREPLIFETIME(AHordePlayerState, EliteEliminations);
	DOREPLIFETIME(AHordePlayerState, PersonalScore);
	DOREPLIFETIME(AHordePlayerState, bIsEliminated);
	DOREPLIFETIME(AHordePlayerState, bIsSpectating);
}

void AHordePlayerState::ServerAddElimination(bool bElite, int32 ScoreDelta)
{
	if (!HasAuthority())
	{
		return;
	}
	++Eliminations;
	if (bElite)
	{
		++EliteEliminations;
	}
	PersonalScore = FMath::Max(0, PersonalScore + ScoreDelta);
}

void AHordePlayerState::SetEliminated(bool bEliminated)
{
	if (!HasAuthority())
	{
		return;
	}
	bIsEliminated = bEliminated;
	bIsSpectating = bEliminated;
}
