// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Arenas/ArenaTypes.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuSubsystem.generated.h"

class UGOTALobbyViewModel;
class UGOTAMenuSettingsAdapter;
class AGOTAMenuCameraDirector;
class UArenaDefinition;
class UUserWidget;

/**
 * Frontend lifecycle: screen navigation, camera requests, settings/lobby accessors,
 * Continue eligibility, and arena card catalog for menu widgets.
 */
UCLASS()
class WAROFGODS_API UGOTAMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void NavigateTo(EGOTAMenuScreen Screen, bool bSkipCameraTransition = false);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void GoBack(bool bSkipCameraTransition = false);

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	EGOTAMenuScreen GetCurrentScreen() const { return CurrentScreen; }

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void BindCameraDirector(AGOTAMenuCameraDirector* Director);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SkipCameraTransition();

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UGOTAMenuSettingsAdapter* GetSettingsAdapter() const { return SettingsAdapter; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UGOTALobbyViewModel* GetLobbyViewModel() const { return LobbyViewModel; }

	/** Continue requires a campaign save — profile-only save is not enough. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool HasValidContinueSave() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool IsExtrasAvailable() const { return false; }

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RequestQuitGame();

	/** Opens Story Mode hub (does not travel). */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RequestStoryMode();

	/**
	 * Starts Story Chapter 1 — The Fallen Sanctuary.
	 * @param bHostCoop If true, travels with ?listen for host + friends (up to SoftMaxPlayers).
	 */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RequestStartStoryPrologue(bool bHostCoop = false);

	/** Continue campaign — no-op until GOTA_Campaign_Slot exists. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RequestContinueStory();

	/** @deprecated Use RequestStoryMode / RequestStartStoryPrologue. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu", meta = (DeprecatedFunction, DeprecationMessage = "Use RequestStartStoryPrologue"))
	void RequestSoloStory();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RequestPvEArena();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void OpenExistingMainMenuFallback();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Arena")
	TArray<FGOTAArenaCard> BuildArenaCards() const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Arena")
	FGOTAArenaCard BuildFallbackArenaCard(EArenaId ArenaId) const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetRootMenuWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UUserWidget* GetRootMenuWidget() const { return RootMenuWidget; }

	/** Creates and shows the cinematic frontend root widget when on a menu map. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void PresentFrontendUI();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void TryBindCameraDirectorInWorld();

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Menu")
	FOnGOTAMenuScreenChanged OnMenuScreenChanged;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Menu")
	FOnGOTAMenuTransitionFinished OnCameraTransitionFinished;

	/** Soft path to cinematic frontend. Falls back to legacy MainMenu if missing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FSoftObjectPath CinematicMenuMap = FSoftObjectPath(TEXT("/Game/MainMenu/L_GOTA_MainMenu.L_GOTA_MainMenu"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FSoftObjectPath LegacyMainMenuMap = FSoftObjectPath(TEXT("/Game/MainMenu/MainMenu.MainMenu"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FSoftObjectPath DefaultArenaMap = FSoftObjectPath(TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap.ThirdPersonMap"));

	/**
	 * Dedicated Story Chapter 1 map. Until authored in editor, travel falls back to
	 * DefaultArenaMap with ?Arena=Story (Fallen Sanctuary graybox).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FSoftObjectPath StoryPrologueMap = FSoftObjectPath(TEXT("/Game/Story/Maps/L_GOTA_Story_Prologue.L_GOTA_Story_Prologue"));

	/** Campaign SaveGame slot — none authored yet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FString CampaignSaveSlot = TEXT("GOTA_Campaign_Slot");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FName CampaignSaveKey = TEXT("Campaign");

	/** Root shell that hosts all GOTA menu screens (created by Editor Python). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	TSoftClassPtr<UUserWidget> FrontendRootWidgetClass =
		TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/MainMenu/UI/WBP_GOTA_FrontendRoot.WBP_GOTA_FrontendRoot_C")));

protected:
	UFUNCTION()
	void HandleCameraTransitionFinished();

	UFUNCTION()
	void HandlePostLoadMap(UWorld* LoadedWorld);

	bool IsFrontendMap(const UWorld* World) const;
	EGOTAMenuScreen ResolveCameraScreen(EGOTAMenuScreen Screen) const;

	UPROPERTY()
	TObjectPtr<UGOTAMenuSettingsAdapter> SettingsAdapter;

	UPROPERTY()
	TObjectPtr<UGOTALobbyViewModel> LobbyViewModel;

	UPROPERTY()
	TWeakObjectPtr<AGOTAMenuCameraDirector> CameraDirector;

	UPROPERTY()
	TObjectPtr<UUserWidget> RootMenuWidget;

	EGOTAMenuScreen CurrentScreen = EGOTAMenuScreen::None;
	TArray<EGOTAMenuScreen> ScreenHistory;
};
