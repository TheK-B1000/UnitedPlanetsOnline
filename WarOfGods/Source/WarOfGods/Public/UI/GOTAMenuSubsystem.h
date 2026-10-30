// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/EngineBaseTypes.h"
#include "Arenas/ArenaTypes.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuSubsystem.generated.h"

class UGOTALobbyViewModel;
class UGOTAMenuSettingsAdapter;
class UGOTAMenuContentDefinition;
class UGOTAStorySaveGame;
class AGOTAMenuCameraDirector;
class UArenaDefinition;
class UAudioComponent;
class UUserWidget;
class UNetDriver;

UENUM(BlueprintType)
enum class EGOTAMenuSound : uint8
{
	Focus,
	Confirm,
	Back,
	Error,
	Transition
};

/**
 * The single frontend coordinator (Layer B).
 * Owns: current screen + history, the modal layer, guarded travel (busy/error),
 * menu-action routing, Continue eligibility, Arena launch requests, menu audio.
 * Does NOT own: story progression, sessions, match rules, health/combat.
 * Those live in their feature systems; this class only requests and reports outcomes.
 */
UCLASS()
class WAROFGODS_API UGOTAMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---------------------------------------------------------------- Navigation
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void NavigateTo(EGOTAMenuScreen Screen, bool bSkipCameraTransition = false);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void GoBack(bool bSkipCameraTransition = false);

	/** Back input from the root: closes the modal first, otherwise steps back a screen. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	bool HandleBackInput();

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	EGOTAMenuScreen GetCurrentScreen() const { return CurrentScreen; }

	/** Shared entry point for nav rows AND feature cards. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void ExecuteMenuAction(EGOTAMenuAction Action);

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool IsMenuActionAvailable(EGOTAMenuAction Action, FText& OutReason) const;

	// ---------------------------------------------------------------- Modal layer
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Modal")
	void ShowModal(const FGOTAModalRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Modal")
	void ShowError(FText Title, FText Body);

	/** Resolve the active modal. bConfirmed selects the primary action. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Modal")
	void ResolveModal(bool bConfirmed);

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu|Modal")
	const FGOTAModalRequest& GetActiveModal() const { return ActiveModal; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu|Modal")
	bool IsModalOpen() const { return ActiveModal.Modal != EGOTAMenuModal::None; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool IsTravelInProgress() const { return bTravelInProgress; }

	// ---------------------------------------------------------------- Camera
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void BindCameraDirector(AGOTAMenuCameraDirector* Director);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SkipCameraTransition();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void TryBindCameraDirectorInWorld();

	// ---------------------------------------------------------------- Accessors
	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UGOTAMenuSettingsAdapter* GetSettingsAdapter() const { return SettingsAdapter; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UGOTALobbyViewModel* GetLobbyViewModel() const { return LobbyViewModel; }

	/** Loaded once and kept alive for the session. May be null if the asset is missing. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UGOTAMenuContentDefinition* GetMenuContent() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	FGOTAProfileView GetProfileView() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool IsReducedMotion() const;

	// ---------------------------------------------------------------- Story / Continue
	/** True only for a loadable, versioned, unfinished campaign save. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Menu|Story")
	bool HasValidContinueSave() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu|Story")
	FText GetContinueSummary() const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Story")
	void RequestStoryMode();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Story")
	void RequestStartStoryPrologue(bool bHostCoop = false);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Story")
	void RequestContinueStory();

	/** @deprecated Use RequestStoryMode / RequestStartStoryPrologue. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu", meta = (DeprecatedFunction, DeprecationMessage = "Use RequestStartStoryPrologue"))
	void RequestSoloStory();

	// ---------------------------------------------------------------- Arena
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Arena")
	void RequestPvEArena();

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu|Arena")
	bool IsArenaRulesetAvailable(EGOTAArenaRuleset Ruleset, EArenaId Arena, FText& OutReason) const;

	/** Validates and travels: Fight AI → DuelGameMode ?Opponent=AI; Horde → HordeGameMode. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Arena")
	bool RequestLaunchArena(EGOTAArenaRuleset Ruleset, EArenaId Arena);

	/** Arena -> Hero Select: remember what to launch once a hero is confirmed. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Arena")
	void BeginArenaHeroSelection(EGOTAArenaRuleset Ruleset, EArenaId Arena);

	/** Called by Hero Select CONFIRM: launches the pending ruleset/arena with the selected hero. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Arena")
	bool LaunchPendingArena();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Arena")
	TArray<FGOTAArenaCard> BuildArenaCards() const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Arena")
	FGOTAArenaCard BuildFallbackArenaCard(EArenaId ArenaId) const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu|Arena")
	static FString GetArenaUrlToken(EArenaId ArenaId);

	// ---------------------------------------------------------------- Lifecycle
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RequestQuitGame();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void OpenExistingMainMenuFallback();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetRootMenuWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UUserWidget* GetRootMenuWidget() const { return RootMenuWidget; }

	/** Creates and shows the frontend root widget exactly once per menu world. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void PresentFrontendUI();

	// ---------------------------------------------------------------- Audio
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Audio")
	void PlayUISound(EGOTAMenuSound Sound);

	/** Re-applies Music/UI volume to the live menu audio after a settings change. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu|Audio")
	void RefreshMenuAudioVolumes();

	// ---------------------------------------------------------------- Events
	UPROPERTY(BlueprintAssignable, Category = "GOTA|Menu")
	FOnGOTAMenuScreenChanged OnMenuScreenChanged;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Menu")
	FOnGOTAMenuTransitionFinished OnCameraTransitionFinished;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Menu")
	FOnGOTAMenuModalChanged OnModalChanged;

	// ---------------------------------------------------------------- Config
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FSoftObjectPath CinematicMenuMap = FSoftObjectPath(TEXT("/Game/MainMenu/L_GOTA_MainMenu.L_GOTA_MainMenu"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FSoftObjectPath LegacyMainMenuMap = FSoftObjectPath(TEXT("/Game/MainMenu/MainMenu.MainMenu"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FSoftObjectPath DefaultArenaMap = FSoftObjectPath(TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap.ThirdPersonMap"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	FSoftObjectPath StoryPrologueMap = FSoftObjectPath(TEXT("/Game/Story/Maps/L_GOTA_Story_Prologue.L_GOTA_Story_Prologue"));

	/** Slots written by AGOTAStoryGameMode (solo, host) plus the legacy slot. Read-only here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	TArray<FString> CampaignSaveSlots = { TEXT("GOTA_Campaign_Solo"), TEXT("GOTA_Campaign_Host"), TEXT("GOTA_Campaign_Slot") };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	TSoftClassPtr<UUserWidget> FrontendRootWidgetClass =
		TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/MainMenu/UI/WBP_GOTA_FrontendRoot.WBP_GOTA_FrontendRoot_C")));

	/** Shown by the MoviePlayer during blocking map loads (hero / arena spotlight). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	TSoftClassPtr<UUserWidget> LoadingScreenWidgetClass =
		TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/MainMenu/UI/WBP_GOTA_LoadingScreen.WBP_GOTA_LoadingScreen_C")));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	TSoftObjectPtr<UGOTAMenuContentDefinition> MenuContentAsset =
		TSoftObjectPtr<UGOTAMenuContentDefinition>(FSoftObjectPath(TEXT("/Game/MainMenu/Data/DA_GOTA_MenuContent.DA_GOTA_MenuContent")));

	/** Soft refs: Music + UI sound classes so the Settings sliders have real routing. */
	/**
	 * True while the frontend uses the opaque 2D clean-plate backdrop: the 3D menu world is not
	 * rendered (pure GPU saving). Set false once the cinematic 3D scene replaces the backdrop.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu")
	bool bSuppressWorldBehindBackdrop = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu|Audio")
	FSoftObjectPath MusicSoundClass = FSoftObjectPath(TEXT("/Game/Audio/Mix/SC_GOTA_Music.SC_GOTA_Music"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GOTA|Menu|Audio")
	FSoftObjectPath UISoundClass = FSoftObjectPath(TEXT("/Game/Audio/Mix/SC_GOTA_UI.SC_GOTA_UI"));

protected:
	UFUNCTION()
	void HandleCameraTransitionFinished();

	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	bool IsFrontendMap(const UWorld* World) const;
	EGOTAMenuScreen ResolveCameraScreen(EGOTAMenuScreen Screen) const;
	bool BeginGuardedTravel(const FString& MapPackage, const FString& Options, const FText& BusyText);
	/** Prepares the MoviePlayer loading screen for the next map load (no-op in PIE / editor). */
	void PrepareLoadingScreen(FName HeroId, EArenaId Arena, const FText& ModeName);
	UGOTAStorySaveGame* LoadBestContinueSave(FString* OutSlot = nullptr) const;
	void StartMenuAudio();
	void StopMenuAudio();
	void ClearFrontendState();
	void SetWorldRenderingSuppressed(bool bSuppressed);
	float GetUserVolume(bool bMusic) const;

	UPROPERTY()
	TObjectPtr<UGOTAMenuSettingsAdapter> SettingsAdapter;

	UPROPERTY()
	TObjectPtr<UGOTALobbyViewModel> LobbyViewModel;

	UPROPERTY()
	mutable TObjectPtr<UGOTAMenuContentDefinition> CachedContent;

	UPROPERTY()
	TWeakObjectPtr<AGOTAMenuCameraDirector> CameraDirector;

	UPROPERTY()
	TObjectPtr<UUserWidget> RootMenuWidget;

	/** World the root widget was presented in (widget GetWorld() follows the current world). */
	TWeakObjectPtr<UWorld> PresentedWorld;

	UPROPERTY()
	TWeakObjectPtr<UAudioComponent> MusicComponent;

	UPROPERTY()
	TWeakObjectPtr<UAudioComponent> AmbienceComponent;

	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingScreenWidget;

	EGOTAArenaRuleset PendingRuleset = EGOTAArenaRuleset::FightAI;
	EArenaId PendingArena = EArenaId::TitansQuarry;

	EGOTAMenuScreen CurrentScreen = EGOTAMenuScreen::None;
	TArray<EGOTAMenuScreen> ScreenHistory;
	FGOTAModalRequest ActiveModal;
	bool bTravelInProgress = false;

	/** Error raised while travelling; shown when the menu next appears. */
	FText PendingErrorTitle;
	FText PendingErrorBody;

	FDelegateHandle TravelFailureHandle;
	FDelegateHandle NetworkFailureHandle;
};
