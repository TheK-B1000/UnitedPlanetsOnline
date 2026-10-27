// White Tiger Studios 2024

#include "UI/GOTAMenuSubsystem.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuCameraDirector.h"
#include "UI/GOTAMenuSettingsAdapter.h"
#include "Arenas/ArenaDefinition.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Blueprint/UserWidget.h"

void UGOTAMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SettingsAdapter = NewObject<UGOTAMenuSettingsAdapter>(this);
	LobbyViewModel = NewObject<UGOTALobbyViewModel>(this);
	CurrentScreen = EGOTAMenuScreen::MainMenu;
}

void UGOTAMenuSubsystem::Deinitialize()
{
	if (AGOTAMenuCameraDirector* Director = CameraDirector.Get())
	{
		Director->OnTransitionFinished.RemoveAll(this);
	}
	RootMenuWidget = nullptr;
	SettingsAdapter = nullptr;
	LobbyViewModel = nullptr;
	Super::Deinitialize();
}

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
	if (Previous != EGOTAMenuScreen::None)
	{
		ScreenHistory.Add(Previous);
	}
	CurrentScreen = Screen;
	OnMenuScreenChanged.Broadcast(CurrentScreen, Previous);

	const EGOTAMenuScreen CameraScreen = ResolveCameraScreen(Screen);
	if (AGOTAMenuCameraDirector* Director = CameraDirector.Get())
	{
		if (!Director->TransitionTo(CameraScreen, bSkipCameraTransition))
		{
			OnCameraTransitionFinished.Broadcast();
		}
	}
	else
	{
		OnCameraTransitionFinished.Broadcast();
	}
}

void UGOTAMenuSubsystem::GoBack(bool bSkipCameraTransition)
{
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
		Director->TransitionTo(ResolveCameraScreen(CurrentScreen), bSkipCameraTransition);
	}
}

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

bool UGOTAMenuSubsystem::HasValidContinueSave() const
{
	// Profile save alone is not a campaign continue point.
	const FString SlotFile = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SaveGames"),
		CampaignSaveSlot + TEXT(".sav"));
	return IFileManager::Get().FileExists(*SlotFile);
}

void UGOTAMenuSubsystem::RequestQuitGame()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}

void UGOTAMenuSubsystem::RequestStoryMode()
{
	NavigateTo(EGOTAMenuScreen::StoryMode, false);
}

void UGOTAMenuSubsystem::RequestStartStoryPrologue(bool bHostCoop)
{
	// Prefer dedicated prologue map; otherwise ThirdPersonMap + Fallen Sanctuary graybox.
	FSoftObjectPath MapPath = DefaultArenaMap;
	const FString StoryPackage = StoryPrologueMap.GetLongPackageName();
	if (!StoryPackage.IsEmpty() && FPackageName::DoesPackageExist(StoryPackage))
	{
		MapPath = StoryPrologueMap;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("L_GOTA_Story_Prologue not found — fallback %s?Arena=Story"),
			*DefaultArenaMap.ToString());
	}

	FString Options = TEXT("Arena=Story?Mode=Story");
	if (bHostCoop)
	{
		Options += TEXT("?listen");
	}

	const FString LongPackage = MapPath.GetLongPackageName();
	UGameplayStatics::OpenLevel(this, FName(*LongPackage), true, Options);
}

void UGOTAMenuSubsystem::RequestContinueStory()
{
	if (!HasValidContinueSave())
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestContinueStory: no campaign save (%s)"), *CampaignSaveSlot);
		return;
	}
	// Checkpoint restore not authored yet — reopen Chapter 1 sanctuary as honest path.
	RequestStartStoryPrologue(false);
}

void UGOTAMenuSubsystem::RequestSoloStory()
{
	RequestStartStoryPrologue(false);
}

void UGOTAMenuSubsystem::RequestPvEArena()
{
	NavigateTo(EGOTAMenuScreen::ArenaSelection, false);
}

void UGOTAMenuSubsystem::OpenExistingMainMenuFallback()
{
	const FSoftObjectPath& Path = CinematicMenuMap.IsValid() ? CinematicMenuMap : LegacyMainMenuMap;
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TSoftObjectPtr<UWorld>(Path));
}

TArray<FGOTAArenaCard> UGOTAMenuSubsystem::BuildArenaCards() const
{
	TArray<FGOTAArenaCard> Cards;

	const TArray<EArenaId> Order = {
		EArenaId::AshfallCrucible,
		EArenaId::FrostboundCitadel,
		EArenaId::VerdantSanctuary,
		EArenaId::SunscarExpanse,
		EArenaId::StormbreakCoast,
		EArenaId::TitansQuarry,
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

	for (EArenaId Id : Order)
	{
		if (const FGOTAArenaCard* Found = FromAssets.Find(Id))
		{
			Cards.Add(*Found);
		}
		else
		{
			Cards.Add(BuildFallbackArenaCard(Id));
		}
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
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Ashfall_Tag", "Prove your strength");
		Card.Description = NSLOCTEXT("GOTA", "Arena_Ashfall_Desc", "Volcanic forges and molten causeways.");
		Card.Biomes = { EBiomeId::Volcanic };
		break;
	case EArenaId::FrostboundCitadel:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Frost", "Frostbound Citadel");
		Card.Tagline = NSLOCTEXT("GOTA", "Arena_Frost_Tag", "Fight together");
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
		Card.Description = NSLOCTEXT("GOTA", "Arena_Quarry_Desc", "Graybox-ready quarry arena.");
		Card.Biomes = { EBiomeId::StoneRuins };
		// Only selectable path today: opens ThirdPerson graybox via ArenaGameMode.
		Card.bComingSoon = false;
		Card.bIsSelectable = true;
		Card.SupportedModes = { EArenaMode::Skirmish, EArenaMode::PvEHorde };
		Card.ArenaMap = DefaultArenaMap;
		break;
	case EArenaId::ShatteredDominion:
		Card.DisplayName = NSLOCTEXT("GOTA", "Arena_Dominion", "The Shattered Dominion");
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

void UGOTAMenuSubsystem::SetRootMenuWidget(UUserWidget* Widget)
{
	RootMenuWidget = Widget;
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
	case EGOTAMenuScreen::ArenaSelection:
		return EGOTAMenuScreen::Multiplayer;
	case EGOTAMenuScreen::StoryMode:
		return EGOTAMenuScreen::MainMenu;
	case EGOTAMenuScreen::Extras:
	case EGOTAMenuScreen::Loading:
		return EGOTAMenuScreen::MainMenu;
	default:
		return Screen;
	}
}
