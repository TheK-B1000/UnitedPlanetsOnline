// White Tiger Studios 2024

#include "Story/GOTAStoryGameState.h"
#include "Net/UnrealNetwork.h"

AGOTAStoryGameState::AGOTAStoryGameState()
{
	bReplicates = true;
}

void AGOTAStoryGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGOTAStoryGameState, MissionId);
	DOREPLIFETIME(AGOTAStoryGameState, ChapterId);
	DOREPLIFETIME(AGOTAStoryGameState, MissionPhase);
	DOREPLIFETIME(AGOTAStoryGameState, CurrentObjectiveId);
	DOREPLIFETIME(AGOTAStoryGameState, CurrentObjective);
	DOREPLIFETIME(AGOTAStoryGameState, ActiveCheckpointId);
	DOREPLIFETIME(AGOTAStoryGameState, bMissionCompleted);
	DOREPLIFETIME(AGOTAStoryGameState, EncounterEnemiesRemaining);
}

void AGOTAStoryGameState::SetMissionPhase(EStoryMissionPhase NewPhase)
{
	if (!HasAuthority() || MissionPhase == NewPhase)
	{
		return;
	}
	MissionPhase = NewPhase;
	OnStoryMissionPhaseChanged.Broadcast(MissionPhase);
}

void AGOTAStoryGameState::SetCurrentObjective(const FStoryObjectiveProgress& Progress)
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentObjective = Progress;
	CurrentObjectiveId = Progress.ObjectiveId;
	OnStoryObjectiveUpdated.Broadcast(CurrentObjectiveId, CurrentObjective);
}

void AGOTAStoryGameState::SetCheckpoint(FName CheckpointId)
{
	if (!HasAuthority())
	{
		return;
	}
	ActiveCheckpointId = CheckpointId;
	OnStoryCheckpointActivated.Broadcast(ActiveCheckpointId);
}

void AGOTAStoryGameState::OnRep_MissionPhase()
{
	OnStoryMissionPhaseChanged.Broadcast(MissionPhase);
}

void AGOTAStoryGameState::OnRep_Objective()
{
	OnStoryObjectiveUpdated.Broadcast(CurrentObjectiveId, CurrentObjective);
}

void AGOTAStoryGameState::OnRep_Checkpoint()
{
	OnStoryCheckpointActivated.Broadcast(ActiveCheckpointId);
}
