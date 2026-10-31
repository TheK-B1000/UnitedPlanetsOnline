// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Story/StoryTypes.h"
#include "GOTAStorySaveGame.generated.h"

UCLASS(BlueprintType)
class WAROFGODS_API UGOTAStorySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Story|Save")
	int32 SaveVersion = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Story|Save")
	FName ChapterId = TEXT("Chapter1");

	UPROPERTY(BlueprintReadWrite, Category = "Story|Save")
	FName MissionId = TEXT("FallenSanctuary");

	UPROPERTY(BlueprintReadWrite, Category = "Story|Save")
	FName CheckpointId = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "Story|Save")
	EStoryObjectiveId LastObjectiveId = EStoryObjectiveId::ReachSanctuaryGates;

	UPROPERTY(BlueprintReadWrite, Category = "Story|Save")
	bool bMissionCompleted = false;

	UPROPERTY(BlueprintReadWrite, Category = "Story|Save")
	FString ProfileTag = TEXT("Solo");
};
