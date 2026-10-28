// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/StoryTypes.h"
#include "GOTAStoryMissionDefinition.generated.h"

USTRUCT(BlueprintType)
struct WAROFGODS_API FStoryObjectiveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	EStoryObjectiveId ObjectiveId = EStoryObjectiveId::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	int32 RequiredCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName CheckpointOnComplete = NAME_None;
};

UCLASS(BlueprintType)
class WAROFGODS_API UGOTAStoryMissionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName MissionId = TEXT("FallenSanctuary");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName ChapterId = TEXT("Chapter1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FText MissionTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FStoryObjectiveDefinition> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story|Combat")
	int32 FirstEncounterEnemyCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story|Combat")
	int32 GuardianHealthMultiplierHint = 2;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("StoryMission"), MissionId);
	}
};
