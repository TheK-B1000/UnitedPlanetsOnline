// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Story/StoryTypes.h"
#include "GOTAStoryGameMode.generated.h"

class AGOTAStoryGameState;
class AGOTAStoryObjectiveActor;
class UGOTAStoryMissionDefinition;
class UGOTAStorySaveGame;
class AArenaGrayboxBuilder;
class AArenaPlayerStart;
class APawn;
class UUserWidget;

/**
 * Chapter 1 — The Fallen Sanctuary.
 * Launch: ?game=/Script/WarOfGods.GOTAStoryGameMode?Mode=Story
 * Continue: add ?Checkpoint=<Id>
 *
 * Enemy defeat uses server-side pawn validity polling (same pattern as Horde/Duel).
 * BP damage remains the health authority — do not treat client Notify* as mission proof.
 */
UCLASS()
class WAROFGODS_API AGOTAStoryGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Story uses the default prototype hero's body (cosmetic; same gameplay pawn). */
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

public:
	AGOTAStoryGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UFUNCTION(BlueprintCallable, Category = "Story")
	void NotifyObjectiveActorCompleted(AGOTAStoryObjectiveActor* ObjectiveActor, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void RequestReturnToMenu();

	UFUNCTION(BlueprintCallable, Category = "Story")
	void RequestRestartMission();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	TSoftObjectPtr<UGOTAStoryMissionDefinition> MissionDefinitionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bAutoBuildGraybox = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bAutoPlaceRuntimeObjectives = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	int32 SoftMaxPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Classes")
	TSoftClassPtr<APawn> SoftPlayerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Classes")
	TSoftClassPtr<APawn> SoftEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Classes")
	TSoftClassPtr<APawn> SoftGuardianClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|UI")
	TSoftClassPtr<UUserWidget> StoryHudWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|UI")
	TSoftClassPtr<UUserWidget> StoryCompleteWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Save")
	FString SoloSaveSlot = TEXT("GOTA_Campaign_Solo");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Save")
	FString HostSaveSlot = TEXT("GOTA_Campaign_Host");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FSoftObjectPath MainMenuMap = FSoftObjectPath(TEXT("/Game/MainMenu/L_GOTA_MainMenu.L_GOTA_MainMenu"));

protected:
	void ResolveClasses();
	void EnsureWorldInfrastructure();
	void EnsurePlayerStarts();
	void EnsureObjectiveActors();
	void EnsureGraybox();
	void StartOrResumeMission();
	void AdvanceToObjective(EStoryObjectiveId NextId);
	void CompleteCurrentObjective();
	bool CanAdvanceObjective(EStoryObjectiveId Id) const;
	void SpawnFirstEncounter();
	void SpawnGuardian();
	void WatchEncounters();
	void ActivateCheckpoint(FName CheckpointId);
	void CompleteMission();
	void PersistProgress(bool bAsync);
	UGOTAStorySaveGame* LoadCampaignSave() const;
	FString GetActiveSaveSlot() const;
	FStoryObjectiveProgress MakeProgress(EStoryObjectiveId Id, int32 Current, int32 Required, bool bDone) const;
	FText GetObjectiveText(EStoryObjectiveId Id) const;
	AGOTAStoryGameState* GetStoryGS() const;
	void PresentHud(APlayerController* PC);
	void PresentCompleteUI();

	UPROPERTY()
	TObjectPtr<UGOTAStoryMissionDefinition> RuntimeMission;

	UPROPERTY()
	TObjectPtr<AArenaGrayboxBuilder> SpawnedGraybox;

	UPROPERTY()
	TArray<TObjectPtr<APawn>> EncounterEnemies;

	UPROPERTY()
	TObjectPtr<APawn> GuardianPawn;

	UPROPERTY()
	TSet<EStoryObjectiveId> CompletedObjectives;

	FName ResumeCheckpointId = NAME_None;
	bool bHostCoopSave = false;
	bool bMissionFlowComplete = false;
	int32 NextPlayerSlot = 0;
	FTimerHandle EncounterWatchHandle;
};
