// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "StoryTypes.generated.h"

UENUM(BlueprintType)
enum class EStoryMissionPhase : uint8
{
	NotStarted,
	Intro,
	Active,
	CheckpointReached,
	Completed,
	Failed
};

UENUM(BlueprintType)
enum class EStoryObjectiveId : uint8
{
	None,
	ReachSanctuaryGates,
	SurviveFirstEncounter,
	ActivateDivineShrine,
	DefeatSanctuaryGuardian,
	MissionComplete
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FStoryObjectiveProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Story")
	EStoryObjectiveId ObjectiveId = EStoryObjectiveId::None;

	UPROPERTY(BlueprintReadOnly, Category = "Story")
	FText DisplayText;

	UPROPERTY(BlueprintReadOnly, Category = "Story")
	int32 CurrentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Story")
	int32 RequiredCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Story")
	bool bCompleted = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryMissionStarted, FName, MissionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStoryObjectiveUpdated, EStoryObjectiveId, ObjectiveId, FStoryObjectiveProgress, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryCheckpointActivated, FName, CheckpointId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryMissionCompleted, FName, MissionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryMissionPhaseChanged, EStoryMissionPhase, NewPhase);
