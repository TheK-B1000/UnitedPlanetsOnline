// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Story/StoryTypes.h"
#include "GOTAStoryGameState.generated.h"

UCLASS()
class WAROFGODS_API AGOTAStoryGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGOTAStoryGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Story")
	FName MissionId = TEXT("FallenSanctuary");

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Story")
	FName ChapterId = TEXT("Chapter1");

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MissionPhase, Category = "Story")
	EStoryMissionPhase MissionPhase = EStoryMissionPhase::NotStarted;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Objective, Category = "Story")
	EStoryObjectiveId CurrentObjectiveId = EStoryObjectiveId::None;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Objective, Category = "Story")
	FStoryObjectiveProgress CurrentObjective;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Checkpoint, Category = "Story")
	FName ActiveCheckpointId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Story")
	bool bMissionCompleted = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Story")
	int32 EncounterEnemiesRemaining = 0;

	UPROPERTY(BlueprintAssignable, Category = "Story")
	FOnStoryMissionStarted OnStoryMissionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Story")
	FOnStoryObjectiveUpdated OnStoryObjectiveUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Story")
	FOnStoryCheckpointActivated OnStoryCheckpointActivated;

	UPROPERTY(BlueprintAssignable, Category = "Story")
	FOnStoryMissionCompleted OnStoryMissionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Story")
	FOnStoryMissionPhaseChanged OnStoryMissionPhaseChanged;

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetMissionPhase(EStoryMissionPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetCurrentObjective(const FStoryObjectiveProgress& Progress);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetCheckpoint(FName CheckpointId);

	UFUNCTION()
	void OnRep_MissionPhase();

	UFUNCTION()
	void OnRep_Objective();

	UFUNCTION()
	void OnRep_Checkpoint();
};
