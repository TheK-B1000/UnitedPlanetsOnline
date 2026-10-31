// White Tiger Studios 2024

#include "UI/GOTAMenuSubsystem.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuCameraDirector.h"
#include "UI/GOTAMenuContentDefinition.h"
#include "UI/GOTAMenuSettingsAdapter.h"
#include "UI/GOTAGameUserSettings.h"
#include "Arenas/ArenaDefinition.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "Story/GOTAStorySaveGame.h"
#include "Characters/GOTAHeroDefinition.h"
#include "Characters/GOTAHeroRegistry.h"
#include "UI/Widgets/GOTALoadingScreenWidget.h"
#include "MoviePlayer.h"
#include "Engine/Texture2D.h"
#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTAMenu, Log, All);

namespace GOTAMenuText
{
	static FText QuitTitle() { return NSLOCTEXT("GOTA", "QuitTitle", "EXIT GODS OF THE ARENA?"); }
	static FText QuitBody() { return NSLOCTEXT("GOTA", "QuitBody", "Any match in progress will end. Campaign progress is kept at your last checkpoint."); }
}

void UGOTAMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SettingsAdapter = NewObject<UGOTAMenuSettingsAdapter>(this);
	LobbyViewModel = NewObject<UGOTALobbyViewModel>(this);
	CurrentScreen = EGOTAMenuScreen::MainMenu;
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UGOTAMenuSubsystem::HandlePostLoadMap);
	if (GEngine)
	{
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UGOTAMenuSubsystem::HandleTravelFailure);
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UGOTAMenuSubsystem::HandleNetworkFailure);
	}
}

void UGOTAMenuSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	if (GEngine)
	{
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}
	if (AGOTAMenuCameraDirector* Director = CameraDirector.Get())
	{
		Director->OnTransitionFinished.RemoveAll(this);
	}
	StopMenuAudio();
	RootMenuWidget = nullptr;
	SettingsAdapter = nullptr;
	LobbyViewModel = nullptr;
	CachedContent = nullptr;
	Super::Deinitialize();
}

// =====================================================================  Navigation

void UGOTAMenuSubsystem::NavigateTo(EGOTAMenuScreen Screen, bool bSkipCameraTransition)
{
	if (Screen == EGOTAMenuScreen::None || Screen == CurrentScreen)
	{
		if (bSkipCameraTransition)
		{
			SkipCameraTransition();
		}
		return;
	}

	const EGOTAMenuScreen Previous = CurrentScreen;
	if (Previous != EGOTAMenuScreen::None && Previous != EGOTAMenuScreen::Loading)
	{
		// Returning to Main always collapses history so Back never loops.
		if (Screen == EGOTAMenuScreen::MainMenu)
		{
			ScreenHistory.Reset();
		}
		else
		{
			ScreenHistory.Add(Previous);
		}
	}
	CurrentScreen = Screen;
	OnMenuScreenChanged.Broadcast(CurrentScreen, Previous);

	const EGOTAMenuScreen CameraScreen = ResolveCameraScreen(Screen);
	AGOTAMenuCameraDirector* Director = CameraDirector.Get();
	const bool bSkip = bSkipCameraTransition || IsReducedMotion();
	if (!Director || !Director->TransitionTo(CameraScreen, bSkip))
	{
		OnCameraTransitionFinished.Broadcast();
	}
}

void UGOTAMenuSubsystem::GoBack(bool bSkipCameraTransition)
{
	if (CurrentScreen == EGOTAMenuScreen::MainMenu)
	{
		return;
	}
	if (ScreenHistory.Num() == 0)
	{
		NavigateTo(EGOTAMenuScreen::MainMenu, bSkipCameraTransition);
		return;
	}

	const EGOTAMenuScreen Previous = ScreenHistory.Pop(EAllowShrinking::No);
	const EGOTAMenuScreen Leaving = CurrentScreen;
	CurrentScreen = Previous;
	OnMenuScreenChanged.Broadcast(CurrentScreen, Leaving);

	if (AGOTAMenuCameraDirector* Director = CameraDirector.Get())
	{
		Director->TransitionTo(ResolveCameraScreen(CurrentScreen), bSkipCameraTransition || IsReducedMotion());
	}
}

bool UGOTAMenuSubsystem::HandleBackInput()
{
	if (IsModalOpen())
	{
		// Busy is not cancellable: travel is already committed.
		if (ActiveModal.Modal == EGOTAMenuModal::Busy)
		{
			return true;
		}
		PlayUISound(EGOTAMenuSound::Back);
		ResolveModal(false);
		return true;
	}
	if (CurrentScreen == EGOTAMenuScreen::MainMenu)
	{
		return false;
	}
	PlayUISound(EGOTAMenuSound::Back);
	GoBack(false);
	return true;
}

bool UGOTAMenuSubsystem::IsMenuActionAvailable(EGOTAMenuAction Action, FText& OutReason) const
{
	OutReason = FText::GetEmpty();
	switch (Action)
	{
	case EGOTAMenuAction::Continue:
		if (!HasValidContinueSave())
		{
			OutReason = NSLOCTEXT("GOTA", "NoContinue", "No campaign progress yet. Start Story Mode to create a checkpoint.");
			return false;
		}
		return true;
	case EGOTAMenuAction::None:
		return false;
	default:
		return true;
	}
}

void UGOTAMenuSubsystem::ExecuteMenuAction(EGOTAMenuAction Action)
{
	if (bTravelInProgress || IsModalOpen())
	{
		return;
	}

	FText Reason;
	if (!IsMenuActionAvailable(Action, Reason))
	{
		PlayUISound(EGOTAMenuSound::Error);
		return;
	}

	PlayUISound(EGOTAMenuSound::Confirm);
	switch (Action)
	{
	case EGOTAMenuAction::Continue:
		RequestContinueStory();
		break;
	case EGOTAMenuAction::Story:
		NavigateTo(EGOTAMenuScreen::StoryMode);
		break;
	case EGOTAMenuAction::Arena:
		NavigateTo(EGOTAMenuScreen::ArenaSelection);
		break;
	case EGOTAMenuAction::Multiplayer:
	{
		const UGotaSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UGotaSessionSubsystem>();
		NavigateTo(Sessions && Sessions->IsInSession() ? EGOTAMenuScreen::Lobby : EGOTAMenuScreen::Multiplayer);
		break;
	}
	case EGOTAMenuAction::Settings:
		NavigateTo(EGOTAMenuScreen::Settings);
		break;
	case EGOTAMenuAction::Extras:
		NavigateTo(EGOTAMenuScreen::Extras);
		break;
	case EGOTAMenuAction::Quit:
		RequestQuitGame();
		break;
	default:
		break;
	}
}

// =====================================================================  Modal

void UGOTAMenuSubsystem::ShowModal(const FGOTAModalRequest& Request)
{
	ActiveModal = Request;
	if (ActiveModal.ConfirmLabel.IsEmpty())
	{
		ActiveModal.ConfirmLabel = NSLOCTEXT("GOTA", "Confirm", "CONFIRM");
	}
	if (ActiveModal.CancelLabel.IsEmpty())
	{
		ActiveModal.CancelLabel = NSLOCTEXT("GOTA", "Cancel", "CANCEL");
	}
	OnModalChanged.Broadcast(ActiveModal);
}

void UGOTAMenuSubsystem::ShowError(FText Title, FText Body)
{
	PlayUISound(EGOTAMenuSound::Error);
	FGOTAModalRequest Request;
	Request.Modal = EGOTAMenuModal::Error;
	Request.Title = Title.IsEmpty() ? NSLOCTEXT("GOTA", "ErrorTitle", "SOMETHING WENT WRONG") : Title;
	Request.Body = Body;
	Request.ConfirmLabel = NSLOCTEXT("GOTA", "ErrorOk", "RETURN");
	ShowModal(Request);
}

void UGOTAMenuSubsystem::ResolveModal(bool bConfirmed)
{
	const EGOTAMenuModal Resolved = ActiveModal.Modal;
	ActiveModal = FGOTAModalRequest();
	OnModalChanged.Broadcast(ActiveModal);

	switch (Resolved)
	{
	case EGOTAMenuModal::ConfirmQuit:
		if (bConfirmed)
		{
			if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
			{
				// In PIE this ends the play session (non-destructive); packaged builds exit.
				UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
			}
		}
		break;
	case EGOTAMenuModal::ConfirmDisplay:
		if (SettingsAdapter)
		{
			bConfirmed ? SettingsAdapter->ConfirmDisplayChanges() : SettingsAdapter->RevertDisplayChanges();
		}
		break;
	default:
		break;
	}
}

// =====================================================================  Camera

void UGOTAMenuSubsystem::BindCameraDirector(AGOTAMenuCameraDirector* Director)
{
	if (AGOTAMenuCameraDirector* Existing = CameraDirector.Get())
	{
		Existing->OnTransitionFinished.RemoveAll(this);
	}

	CameraDirector = Director;
	if (Director)
	{
		Director->OnTransitionFinished.AddDynamic(this, &UGOTAMenuSubsystem::HandleCameraTransitionFinished);
		Director->TransitionTo(ResolveCameraScreen(CurrentScreen), true);
	}
}

void UGOTAMenuSubsystem::SkipCameraTransition()
{
	if (AGOTAMenuCameraDirector* Director = CameraDirector.Get())
	{
		Director->SkipActiveTransition();
	}
}

void UGOTAMenuSubsystem::TryBindCameraDirectorInWorld()
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AGOTAMenuCameraDirector> It(World); It; ++It)
		{
			BindCameraDirector(*It);
			return;
		}
	}
}

void UGOTAMenuSubsystem::HandleCameraTransitionFinished()
{
	OnCameraTransitionFinished.Broadcast();
}

EGOTAMenuScreen UGOTAMenuSubsystem::ResolveCameraScreen(EGOTAMenuScreen Screen) const
{
	switch (Screen)
	{
	case EGOTAMenuScreen::Lobby:
		return EGOTAMenuScreen::Multiplayer;
	case EGOTAMenuScreen::StoryMode:
	case EGOTAMenuScreen::ArenaSelection:
	case EGOTAMenuScreen::Extras:
	case EGOTAMenuScreen::Loading:
		return EGOTAMenuScreen::MainMenu;
	default:
		return Screen;
	}
}

// =====================================================================  Accessors

UGOTAMenuContentDefinition* UGOTAMenuSubsystem::GetMenuContent() const
{
	if (!CachedContent && !MenuContentAsset.IsNull())
	{
		CachedContent = MenuContentAsset.LoadSynchronous();
		if (!CachedContent)
		{
			UE_LOG(LogGOTAMenu, Warning, TEXT("Menu content asset missing: %s"), *MenuContentAsset.ToString());
		}
	}
	return CachedContent;
}

FGOTAProfileView UGOTAMenuSubsystem::GetProfileView() const
{
	FGOTAProfileView View;
	// Local identity only: the OS account name (OSS Null nicknames are machine ids, not names).
	FString Name = FPlatformProcess::UserName();
	if (Name.IsEmpty())
	{
		if (const ULocalPlayer* LP = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr)
		{
			Name = LP->GetNickname();
		}
	}
	if (Name.Len() > 18)
	{
		Name = Name.Left(17) + TEXT("…");
	}
	View.DisplayName = FText::FromString(Name.IsEmpty() ? TEXT("WARDEN") : Name.ToUpper());
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	View.RankTitle = Content && !Content->DefaultRankTitle.IsEmpty()
		? Content->DefaultRankTitle
		: NSLOCTEXT("GOTA", "DefaultRank", "AUTHORITY WARDEN");
	// No persistent progression system exists yet — do not invent a level.
	View.bHasProgression = false;
	return View;
}

bool UGOTAMenuSubsystem::IsReducedMotion() const
{
	const UGOTAGameUserSettings* Settings = UGOTAGameUserSettings::GetGOTAGameUserSettings();
	return Settings && Settings->bReducedMotion;
}

// =====================================================================  Story / Continue

UGOTAStorySaveGame* UGOTAMenuSubsystem::LoadBestContinueSave(FString* OutSlot) const
{
	for (const FString& Slot : CampaignSaveSlots)
	{
		if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
		{
			continue;
		}
		UGOTAStorySaveGame* Save = Cast<UGOTAStorySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
		if (!Save)
		{
			UE_LOG(LogGOTAMenu, Warning, TEXT("Continue: slot %s is not a GOTA story save — ignored"), *Slot);
			continue;
		}
		if (Save->SaveVersion < 1 || Save->MissionId.IsNone() || Save->bMissionCompleted)
		{
			continue;
		}
		if (OutSlot)
		{
			*OutSlot = Slot;
		}
		return Save;
	}
	return nullptr;
}

bool UGOTAMenuSubsystem::HasValidContinueSave() const
{
	return LoadBestContinueSave() != nullptr;
}

FText UGOTAMenuSubsystem::GetContinueSummary() const
{
	if (const UGOTAStorySaveGame* Save = LoadBestContinueSave())
	{
		const FText Mission = Save->MissionId == TEXT("FallenSanctuary")
			? NSLOCTEXT("GOTA", "MissionFallen", "The Fallen Sanctuary")
			: FText::FromName(Save->MissionId);
		if (Save->CheckpointId.IsNone())
		{
			return Mission;
		}
		return FText::Format(NSLOCTEXT("GOTA", "ContinueFmt", "{0} — {1}"), Mission, FText::FromName(Save->CheckpointId));
	}
	return FText::GetEmpty();
}

void UGOTAMenuSubsystem::RequestStoryMode()
{
	NavigateTo(EGOTAMenuScreen::StoryMode, false);
}

void UGOTAMenuSubsystem::RequestStartStoryPrologue(bool bHostCoop)
{
	FSoftObjectPath MapPath = DefaultArenaMap;
	FString Options = TEXT("game=/Script/WarOfGods.GOTAStoryGameMode?Mode=Story");
	const FString StoryPackage = StoryPrologueMap.GetLongPackageName();
	if (!StoryPackage.IsEmpty() && FPackageName::DoesPackageExist(StoryPackage))
	{
		MapPath = StoryPrologueMap;
	}
	else
	{
		Options += TEXT("?Arena=Story");
		UE_LOG(LogGOTAMenu, Log, TEXT("L_GOTA_Story_Prologue not found — fallback %s?Arena=Story"), *DefaultArenaMap.ToString());
	}
	if (bHostCoop)
	{
		Options += TEXT("?listen");
	}
	BeginGuardedTravel(MapPath.GetLongPackageName(), Options, NSLOCTEXT("GOTA", "LoadingStory", "Deploying to The Fallen Sanctuary…"));
}

void UGOTAMenuSubsystem::RequestContinueStory()
{
	FString Slot;
	const UGOTAStorySaveGame* Save = LoadBestContinueSave(&Slot);
	if (!Save)
	{
		ShowError(NSLOCTEXT("GOTA", "ContinueErrTitle", "NO MISSION TO RESUME"),
			NSLOCTEXT("GOTA", "ContinueErrBody", "No valid campaign checkpoint was found. Start The Fallen Sanctuary from Story Mode."));
		return;
	}

	FSoftObjectPath MapPath = DefaultArenaMap;
	FString Options = TEXT("game=/Script/WarOfGods.GOTAStoryGameMode?Mode=Story");
	const FString StoryPackage = StoryPrologueMap.GetLongPackageName();
	if (!StoryPackage.IsEmpty() && FPackageName::DoesPackageExist(StoryPackage))
	{
		MapPath = StoryPrologueMap;
	}
	else
	{
		Options += TEXT("?Arena=Story");
	}
	if (!Save->CheckpointId.IsNone())
	{
		Options += FString::Printf(TEXT("?Checkpoint=%s"), *Save->CheckpointId.ToString());
	}
	UE_LOG(LogGOTAMenu, Log, TEXT("Continue from %s checkpoint=%s"), *Slot, *Save->CheckpointId.ToString());
	BeginGuardedTravel(MapPath.GetLongPackageName(), Options, NSLOCTEXT("GOTA", "LoadingContinue", "Resuming your mission…"));
}

void UGOTAMenuSubsystem::RequestSoloStory()
{
	RequestStartStoryPrologue(false);
}

// =====================================================================  Arena

void UGOTAMenuSubsystem::RequestPvEArena()
{
	NavigateTo(EGOTAMenuScreen::ArenaSelection, false);
}

FString UGOTAMenuSubsystem::GetArenaUrlToken(EArenaId ArenaId)
{
	switch (ArenaId)
	{
	case EArenaId::TitansQuarry: return TEXT("Quarry");
	case EArenaId::AshfallCrucible: return TEXT("Ashfall");
	case EArenaId::FrostboundCitadel: return TEXT("Frostbound");
	case EArenaId::VerdantSanctuary: return TEXT("Verdant");
	case EArenaId::SunscarExpanse: return TEXT("Sunscar");
	case EArenaId::StormbreakCoast: return TEXT("Stormbreak");
	case EArenaId::ShatteredDominion: return TEXT("Dominion");
	case EArenaId::FallenSanctuary: return TEXT("Story");
	default: return TEXT("Quarry");
	}
}

bool UGOTAMenuSubsystem::IsArenaRulesetAvailable(EGOTAArenaRuleset Ruleset, EArenaId Arena, FText& OutReason) const
{
	OutReason = FText::GetEmpty();
	const FString MapPackage = DefaultArenaMap.GetLongPackageName();
	if (!FPackageName::DoesPackageExist(MapPackage))
	{
		OutReason = NSLOCTEXT("GOTA", "ArenaMapMissing", "Arena map is missing from this build.");
		return false;
	}

	// Rulesets are validated per arena. Only Titan's Quarry has a verified Trial of Champions
	// and Horde layout (player starts + spawn points); other grayboxes are not enabled here.
	if (Arena != EArenaId::TitansQuarry)
	{
		OutReason = NSLOCTEXT("GOTA", "ArenaNotValidated", "This arena has not been validated for this ruleset yet.");
		return false;
	}

	if (Ruleset == EGOTAArenaRuleset::Horde)
	{
		const UGOTAMenuContentDefinition* Content = GetMenuContent();
		if (!Content || !Content->bHordeLaunchEnabled)
		{
			OutReason = Content && !Content->HordeUnavailableReason.IsEmpty()
				? Content->HordeUnavailableReason
				: NSLOCTEXT("GOTA", "HordeNotReady", "Wrath of the Gods is still in validation and cannot be launched from the menu yet.");
			return false;
		}
	}
	return true;
}

bool UGOTAMenuSubsystem::RequestLaunchArena(EGOTAArenaRuleset Ruleset, EArenaId Arena)
{
	FText Reason;
	if (!IsArenaRulesetAvailable(Ruleset, Arena, Reason))
	{
		ShowError(NSLOCTEXT("GOTA", "ArenaUnavailableTitle", "ARENA UNAVAILABLE"), Reason);
		return false;
	}

	const FString ArenaToken = GetArenaUrlToken(Arena);
	FString Options;
	FText Busy;
	FText ModeName;
	if (Ruleset == EGOTAArenaRuleset::FightAI)
	{
		static const TCHAR* DifficultyTokens[] = { TEXT("Recruit"), TEXT("Veteran"), TEXT("Ascendant") };
		Options = FString::Printf(TEXT("game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=%s?BestOf=3?Opponent=AI?Difficulty=%s"),
			*ArenaToken, DifficultyTokens[GetFightAIDifficulty()]);
		Busy = NSLOCTEXT("GOTA", "LoadingFightAI", "Entering Titan's Quarry — Trial of Champions…");
		ModeName = NSLOCTEXT("GOTA", "ModeTrial", "TRIAL OF CHAMPIONS");
	}
	else
	{
		Options = FString::Printf(TEXT("game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=%s"), *ArenaToken);
		Busy = NSLOCTEXT("GOTA", "LoadingHorde", "Entering Titan's Quarry — Wrath of the Gods…");
		ModeName = NSLOCTEXT("GOTA", "ModeHorde", "WRATH OF THE GODS");
	}
	// The hero is only a *request*; the server re-validates it (ADuelGameMode::InitNewPlayer).
	FName HeroId;
	if (const UGOTAHeroRegistry* Registry = GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>())
	{
		HeroId = Registry->GetLocalSelection();
		if (!HeroId.IsNone())
		{
			Options += FString::Printf(TEXT("?Hero=%s"), *HeroId.ToString());
		}
	}
	PrepareLoadingScreen(HeroId, Arena, ModeName);
	return BeginGuardedTravel(DefaultArenaMap.GetLongPackageName(), Options, Busy);
}

int32 UGOTAMenuSubsystem::GetFightAIDifficulty() const
{
	const UGOTAGameUserSettings* Settings = UGOTAGameUserSettings::GetGOTAGameUserSettings();
	return Settings ? FMath::Clamp(Settings->FightAIDifficulty, 0, 2) : 1;
}

int32 UGOTAMenuSubsystem::CycleFightAIDifficulty()
{
	UGOTAGameUserSettings* Settings = UGOTAGameUserSettings::GetGOTAGameUserSettings();
	if (!Settings)
	{
		return 1;
	}
	Settings->FightAIDifficulty = (FMath::Clamp(Settings->FightAIDifficulty, 0, 2) + 1) % 3;
	Settings->SaveSettings();
	return Settings->FightAIDifficulty;
}

void UGOTAMenuSubsystem::BeginArenaHeroSelection(EGOTAArenaRuleset Ruleset, EArenaId Arena)
{
	FText Reason;
	if (!IsArenaRulesetAvailable(Ruleset, Arena, Reason))
	{
		ShowError(NSLOCTEXT("GOTA", "ArenaUnavailableTitle", "ARENA UNAVAILABLE"), Reason);
		return;
	}
	PendingRuleset = Ruleset;
	PendingArena = Arena;
	NavigateTo(EGOTAMenuScreen::HeroSelect);
}

bool UGOTAMenuSubsystem::LaunchPendingArena()
{
	return RequestLaunchArena(PendingRuleset, PendingArena);
}

void UGOTAMenuSubsystem::PrepareLoadingScreen(FName HeroId, EArenaId Arena, const FText& ModeName)
{
	if (!IsMoviePlayerEnabled() || GIsEditor)
	{
		return; // PIE/editor: no MoviePlayer — the Busy modal covers the transition there.
	}
	APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	UClass* WidgetClass = LoadingScreenWidgetClass.LoadSynchronous();
	if (!PC || !WidgetClass)
	{
		return;
	}
	UGOTALoadingScreenWidget* Widget = CreateWidget<UGOTALoadingScreenWidget>(PC, WidgetClass);
	if (!Widget)
	{
		return;
	}
	const FGOTAArenaCard Card = BuildFallbackArenaCard(Arena);
	const FText Tip = NSLOCTEXT("GOTA", "LoadTip", "TIP  \u2022  A round ends when a fighter falls or the 90-second clock expires; on time, the healthier fighter takes the round.");
	const UGOTAHeroRegistry* Registry = GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>();
	if (const UGOTAHeroDefinition* Hero = Registry ? Registry->FindHero(HeroId) : nullptr)
	{
		Widget->ConfigureHero(*Hero, Card.DisplayName, ModeName, Tip);
	}
	else
	{
		UTexture2D* Art = nullptr;
		if (const UGOTAMenuContentDefinition* Content = GetMenuContent())
		{
			if (const TSoftObjectPtr<UTexture2D>* Preview = Content->ArenaPreviews.Find(Arena))
			{
				Art = Preview->LoadSynchronous();
			}
		}
		Widget->ConfigureArena(Art, Card.DisplayName, Card.Description, ModeName, Tip);
	}
	LoadingScreenWidget = Widget; // keep alive for the duration of the load
	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.bWaitForManualStop = false;
	Attributes.MinimumLoadingScreenDisplayTime = 0.f;
	Attributes.WidgetLoadingScreen = Widget->TakeWidget();
	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

TArray<FGOTAArenaCard> UGOTAMenuSubsystem::BuildArenaCards() const
{
	TArray<FGOTAArenaCard> Cards;

	// Order matches the frontend brief: launchable arena first.
	const TArray<EArenaId> Order = {
		EArenaId::TitansQuarry,
		EArenaId::AshfallCrucible,
		EArenaId::FrostboundCitadel,
		EArenaId::VerdantSanctuary,
		EArenaId::SunscarExpanse,
		EArenaId::StormbreakCoast,
		EArenaId::ShatteredDominion
	};

	TMap<EArenaId, FGOTAArenaCard> FromAssets;
	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> AssetIds;
		Manager->GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("ArenaDefinition")), AssetIds);
		for (const FPrimaryAssetId& Id : AssetIds)
		{
			if (UArenaDefinition* Def = Cast<UArenaDefinition>(Manager->GetPrimaryAssetObject(Id)))
			{
				FGOTAArenaCard Card;
				Card.ArenaId = Def->ArenaId;
				Card.DisplayName = Def->DisplayName.IsEmpty()
					? FText::FromString(StaticEnum<EArenaId>()->GetNameStringByValue(static_cast<int64>(Def->ArenaId)))
					: Def->DisplayName;
				Card.Description = Def->Description;
				Card.Tagline = Def->Tagline;
				Card.Biomes = Def->BiomeIds;
				Card.SupportedModes = Def->SupportedModes;
				Card.bIsSelectable = Def->bIsSelectable;
				Card.bComingSoon = Def->bComingSoon;
				Card.Thumbnail = Def->Thumbnail;
				Card.ArenaMap = Def->ArenaMap.ToSoftObjectPath();
				FromAssets.Add(Card.ArenaId, Card);
			}
		}
	}

	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	for (EArenaId Id : Order)
	{
		FGOTAArenaCard Card = FromAssets.Contains(Id) ? FromAssets[Id] : BuildFallbackArenaCard(Id);
		if (Card.Thumbnail.IsNull() && Content)
		{
			if (const TSoftObjectPtr<UTexture2D>* Preview = Content->ArenaPreviews.Find(Id))
			{
				Card.Thumbnail = *Preview;
			}
		}
		Cards.Add(Card);
	}
	return Cards;
}

FGOTAArenaCard UGOTAMenuSubsystem::BuildFallbackArenaCard(EArenaId ArenaId) const
{
	FGOTAArenaCard Card;
	Card.ArenaId = ArenaId;
	Card.bComingSoon = true;
	Card.bIsSelectable = false;

	switch (ArenaId)
	{
	case EArenaId::AshfallCrucible:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Ashfall", "Ashfall Crucible");
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Ashfall_Tag", "Volcanic forge arena");
		Card.Description = NSLOCTEXT("GOTA", "Arena_Ashfall_Desc", "Volcanic forges and molten causeways.");
		Card.Biomes = { EBiomeId::Volcanic };
		break;
	case EArenaId::FrostboundCitadel:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Frost", "Frostbound Citadel");
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Frost_Tag", "Frozen battlements");
		Card.Description = NSLOCTEXT("GOTA", "Arena_Frost_Desc", "Frozen battlements above the storm.");
		Card.Biomes = { EBiomeId::Frozen };
		break;
	case EArenaId::VerdantSanctuary:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Verdant", "Verdant Sanctuary");
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Verdant_Tag", "Ancient groves");
		Card.Description = NSLOCTEXT("GOTA", "Arena_Verdant_Desc", "Sacred forests and ruined temples.");
		Card.Biomes = { EBiomeId::Forest };
		break;
	case EArenaId::SunscarExpanse:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Sunscar", "Sunscar Expanse");
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Sunscar_Tag", "Burning dunes");
		Card.Description = NSLOCTEXT("GOTA", "Arena_Sunscar_Desc", "Sun-blasted desert ruins.");
		Card.Biomes = { EBiomeId::Desert };
		break;
	case EArenaId::StormbreakCoast:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Storm", "Stormbreak Coast");
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Storm_Tag", "Where gods meet the sea");
		Card.Description = NSLOCTEXT("GOTA", "Arena_Storm_Desc", "Nordic cliffs and thunderous shores.");
		Card.Biomes = { EBiomeId::Coast };
		break;
	case EArenaId::TitansQuarry:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Quarry", "Titan's Quarry");
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Quarry_Tag", "Stone and steel");
		Card.Description = NSLOCTEXT("GOTA", "Arena_Quarry_Desc", "A colossal stone excavation — terraced rock, raised causeways and open ground built for single combat.");
		Card.Biomes = { EBiomeId::StoneRuins };
		Card.bComingSoon = false;
		Card.bIsSelectable = true;
		Card.SupportedModes = { EArenaMode::PvPDuel, EArenaMode::PvEHorde };
		Card.ArenaMap = DefaultArenaMap;
		break;
	case EArenaId::ShatteredDominion:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Dominion", "Shattered Dominion");
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Dominion_Tag", "A world of gods awaits");
		Card.Description = NSLOCTEXT("GOTA", "Arena_Dominion_Desc", "Colossal multi-biome Pantheon Coliseum.");
		Card.Biomes = { EBiomeId::Coliseum, EBiomeId::Volcanic, EBiomeId::Frozen, EBiomeId::Forest, EBiomeId::Desert, EBiomeId::Coast };
		break;
	default:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Unknown", "Unknown Arena");
		break;
	}
	return Card;
}

// =====================================================================  Travel guard

bool UGOTAMenuSubsystem::BeginGuardedTravel(const FString& MapPackage, const FString& Options, const FText& BusyText)
{
	if (bTravelInProgress)
	{
		UE_LOG(LogGOTAMenu, Warning, TEXT("Travel ignored — already travelling"));
		return false;
	}
	if (!FPackageName::DoesPackageExist(MapPackage))
	{
		ShowError(NSLOCTEXT("GOTA", "MapMissingTitle", "UNABLE TO LOAD"),
			FText::Format(NSLOCTEXT("GOTA", "MapMissingBody", "The map {0} is not available in this build."), FText::FromString(MapPackage)));
		return false;
	}

	bTravelInProgress = true;
	FGOTAModalRequest Busy;
	Busy.Modal = EGOTAMenuModal::Busy;
	Busy.Title = NSLOCTEXT("GOTA", "BusyTitle", "PREPARING DEPLOYMENT");
	Busy.Body = BusyText;
	ShowModal(Busy);
	PlayUISound(EGOTAMenuSound::Transition);

	UE_LOG(LogGOTAMenu, Log, TEXT("Travel → %s?%s"), *MapPackage, *Options);
	// OpenLevel is deferred to the next engine tick; the busy overlay renders this frame.
	// Completion is observed by PostLoadMap (success) or OnTravelFailure (failure), never a timer.
	UGameplayStatics::OpenLevel(this, FName(*MapPackage), true, Options);
	return true;
}

void UGOTAMenuSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	bTravelInProgress = false;
	PendingErrorTitle = NSLOCTEXT("GOTA", "TravelFailTitle", "DEPLOYMENT FAILED");
	PendingErrorBody = FText::Format(NSLOCTEXT("GOTA", "TravelFailBody", "{0}\n\n{1}"),
		FText::FromString(ETravelFailure::ToString(FailureType)), FText::FromString(ErrorString));
	UE_LOG(LogGOTAMenu, Warning, TEXT("Travel failure %s: %s"), ETravelFailure::ToString(FailureType), *ErrorString);
	if (RootMenuWidget && RootMenuWidget->IsInViewport())
	{
		ShowError(PendingErrorTitle, PendingErrorBody);
		PendingErrorTitle = FText::GetEmpty();
		PendingErrorBody = FText::GetEmpty();
	}
}

void UGOTAMenuSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	bTravelInProgress = false;
	PendingErrorTitle = NSLOCTEXT("GOTA", "NetFailTitle", "CONNECTION LOST");
	PendingErrorBody = FText::Format(NSLOCTEXT("GOTA", "NetFailBody", "{0}\n\n{1}"),
		FText::FromString(ENetworkFailure::ToString(FailureType)), FText::FromString(ErrorString));
	UE_LOG(LogGOTAMenu, Warning, TEXT("Network failure %s: %s"), ENetworkFailure::ToString(FailureType), *ErrorString);
}

// =====================================================================  Lifecycle

void UGOTAMenuSubsystem::RequestQuitGame()
{
	FGOTAModalRequest Request;
	Request.Modal = EGOTAMenuModal::ConfirmQuit;
	Request.Title = GOTAMenuText::QuitTitle();
	Request.Body = GOTAMenuText::QuitBody();
	Request.ConfirmLabel = NSLOCTEXT("GOTA", "QuitConfirm", "QUIT GAME");
	Request.CancelLabel = NSLOCTEXT("GOTA", "QuitCancel", "CANCEL");
	ShowModal(Request);
}

void UGOTAMenuSubsystem::OpenExistingMainMenuFallback()
{
	const FSoftObjectPath& Path = CinematicMenuMap.IsValid() ? CinematicMenuMap : LegacyMainMenuMap;
	BeginGuardedTravel(Path.GetLongPackageName(), FString(), NSLOCTEXT("GOTA", "LoadingMenu", "Returning to the capital…"));
}

void UGOTAMenuSubsystem::SetRootMenuWidget(UUserWidget* Widget)
{
	RootMenuWidget = Widget;
}

void UGOTAMenuSubsystem::ClearFrontendState()
{
	// Release the previous world's root (its viewport is gone); never keep it alive across travel.
	if (RootMenuWidget)
	{
		RootMenuWidget->RemoveFromParent();
	}
	RootMenuWidget = nullptr;
	PresentedWorld.Reset();
	ScreenHistory.Reset();
	ActiveModal = FGOTAModalRequest();
	bTravelInProgress = false;
	CameraDirector.Reset();
	StopMenuAudio();
	SetWorldRenderingSuppressed(false);
}

void UGOTAMenuSubsystem::SetWorldRenderingSuppressed(bool bSuppressed)
{
	if (UGameViewportClient* Viewport = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
	{
		Viewport->bDisableWorldRendering = bSuppressed;
	}
}

void UGOTAMenuSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// The frontend GameMode may already have presented the UI during BeginPlay (which runs
	// before this broadcast). Never clear or re-present for the same world — no duplicates.
	// (Widgets are outered to the GameInstance, so their GetWorld() follows the *current* world —
	// track the presented world explicitly.)
	if (RootMenuWidget && PresentedWorld.Get() == LoadedWorld)
	{
		return;
	}

	ClearFrontendState();
	LoadingScreenWidget = nullptr; // MoviePlayer has finished with it once the map is loaded
	if (!IsFrontendMap(LoadedWorld))
	{
		CurrentScreen = EGOTAMenuScreen::None;
		return;
	}

	CurrentScreen = EGOTAMenuScreen::None;
	// The local PlayerController is spawned after PostLoadMap — present on the next tick.
	LoadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UGOTAMenuSubsystem::TryBindCameraDirectorInWorld));
	LoadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UGOTAMenuSubsystem::PresentFrontendUI));
}

bool UGOTAMenuSubsystem::IsFrontendMap(const UWorld* World) const
{
	if (!World)
	{
		return false;
	}
	// Only the cinematic frontend hosts this UI; the legacy MainMenu keeps its own BP widgets.
	return World->GetMapName().Contains(TEXT("L_GOTA_MainMenu"), ESearchCase::IgnoreCase);
}

void UGOTAMenuSubsystem::PresentFrontendUI()
{
	UWorld* World = GetWorld();
	if (!World || !IsFrontendMap(World))
	{
		return;
	}
	if (RootMenuWidget && RootMenuWidget->IsInViewport() && PresentedWorld.Get() == World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		// Retry next tick until the local controller exists (bounded by map lifetime).
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UGOTAMenuSubsystem::PresentFrontendUI));
		return;
	}

	UClass* WidgetClass = FrontendRootWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogGOTAMenu, Error, TEXT("GOTA frontend root widget missing: %s"), *FrontendRootWidgetClass.ToString());
		return;
	}

	UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
	if (!Widget)
	{
		return;
	}

	Widget->AddToViewport(100);
	RootMenuWidget = Widget;
	PresentedWorld = World;
	bTravelInProgress = false;
	ActiveModal = FGOTAModalRequest();
	// The concept clean plate is fully opaque — skip rendering the 3D world behind it.
	SetWorldRenderingSuppressed(bSuppressWorldBehindBackdrop);
	PC->SetShowMouseCursor(true);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);

	StartMenuAudio();

	const UGotaSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UGotaSessionSubsystem>();
	NavigateTo(Sessions && Sessions->IsInSession() ? EGOTAMenuScreen::Lobby : EGOTAMenuScreen::MainMenu, true);

	if (!PendingErrorBody.IsEmpty())
	{
		ShowError(PendingErrorTitle, PendingErrorBody);
		PendingErrorTitle = FText::GetEmpty();
		PendingErrorBody = FText::GetEmpty();
	}
	UE_LOG(LogGOTAMenu, Log, TEXT("Frontend presented (%s)"), *GetNameSafe(WidgetClass));
}

// =====================================================================  Audio

float UGOTAMenuSubsystem::GetUserVolume(bool bMusic) const
{
	const UGOTAGameUserSettings* Settings = UGOTAGameUserSettings::GetGOTAGameUserSettings();
	if (!Settings)
	{
		return 1.f;
	}
	return bMusic ? Settings->MusicVolume : Settings->UIVolume;
}

void UGOTAMenuSubsystem::StartMenuAudio()
{
	UWorld* World = GetWorld();
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	if (!World || !Content || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	USoundClass* MusicClass = Cast<USoundClass>(MusicSoundClass.TryLoad());
	auto StartLoop = [&](TWeakObjectPtr<UAudioComponent>& Slot, const TSoftObjectPtr<USoundBase>& SoundRef, float Base)
	{
		if (Slot.IsValid() && Slot->IsPlaying())
		{
			return; // never stack loops
		}
		USoundBase* Sound = SoundRef.LoadSynchronous();
		if (!Sound)
		{
			return;
		}
		UAudioComponent* Comp = UGameplayStatics::CreateSound2D(World, Sound, Base * GetUserVolume(true), 1.f, 0.f, nullptr, false, false);
		if (Comp)
		{
			Comp->SoundClassOverride = MusicClass;
			Comp->bIsUISound = true;
			Comp->FadeIn(IsReducedMotion() ? 0.1f : 2.5f, Base * GetUserVolume(true));
			Slot = Comp;
		}
	};
	StartLoop(MusicComponent, Content->MenuMusic, Content->MusicBaseVolume);
	StartLoop(AmbienceComponent, Content->MenuAmbience, Content->AmbienceBaseVolume);
}

void UGOTAMenuSubsystem::StopMenuAudio()
{
	for (TWeakObjectPtr<UAudioComponent>* Slot : { &MusicComponent, &AmbienceComponent })
	{
		if (UAudioComponent* Comp = Slot->Get())
		{
			Comp->Stop();
			Comp->DestroyComponent();
		}
		Slot->Reset();
	}
}

void UGOTAMenuSubsystem::RefreshMenuAudioVolumes()
{
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	if (!Content)
	{
		return;
	}
	if (UAudioComponent* Music = MusicComponent.Get())
	{
		Music->SetVolumeMultiplier(Content->MusicBaseVolume * GetUserVolume(true));
	}
	if (UAudioComponent* Amb = AmbienceComponent.Get())
	{
		Amb->SetVolumeMultiplier(Content->AmbienceBaseVolume * GetUserVolume(true));
	}
}

void UGOTAMenuSubsystem::PlayUISound(EGOTAMenuSound Sound)
{
	UWorld* World = GetWorld();
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	if (!World || !Content || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const TSoftObjectPtr<USoundBase>* Ref = nullptr;
	switch (Sound)
	{
	case EGOTAMenuSound::Focus: Ref = &Content->FocusCue; break;
	case EGOTAMenuSound::Confirm: Ref = &Content->ConfirmCue; break;
	case EGOTAMenuSound::Back: Ref = &Content->BackCue; break;
	case EGOTAMenuSound::Error: Ref = &Content->ErrorCue; break;
	case EGOTAMenuSound::Transition: Ref = &Content->TransitionCue; break;
	}
	USoundBase* Cue = Ref ? Ref->LoadSynchronous() : nullptr;
	if (!Cue)
	{
		return;
	}
	if (UAudioComponent* Comp = UGameplayStatics::CreateSound2D(World, Cue, Content->UIBaseVolume * GetUserVolume(false), 1.f, 0.f, nullptr, false, true))
	{
		Comp->SoundClassOverride = Cast<USoundClass>(UISoundClass.TryLoad());
		Comp->bIsUISound = true;
		Comp->Play();
	}
}
