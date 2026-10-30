// White Tiger Studios 2024

#include "Multiplayer/GotaSessionSubsystem.h"

#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "TimerManager.h"

void UGotaSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreateCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGotaSessionSubsystem::HandleCreateSessionComplete);
	FindCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGotaSessionSubsystem::HandleFindSessionsComplete);
	JoinCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGotaSessionSubsystem::HandleJoinSessionComplete);
	DestroyCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UGotaSessionSubsystem::HandleDestroySessionComplete);
}

void UGotaSessionSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteHandle);
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindCompleteHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinCompleteHandle);
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyCompleteHandle);
	}
	Super::Deinitialize();
}

IOnlineSessionPtr UGotaSessionSubsystem::GetSessions() const
{
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		return OSS->GetSessionInterface();
	}
	return nullptr;
}

bool UGotaSessionSubsystem::IsSessionBackendAvailable() const
{
	if (GetSessions().IsValid())
	{
		return true;
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->FindFunction(FName(TEXT("CreateMPSession"))) != nullptr;
	}
	return false;
}

bool UGotaSessionSubsystem::TryCallGameInstance(FName FunctionName) const
{
	if (FunctionName.IsNone() || !bUseBlueprintSessionEvents)
	{
		return false;
	}
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}
	if (UFunction* Function = GI->FindFunction(FunctionName))
	{
		GI->ProcessEvent(Function, nullptr);
		return true;
	}
	return false;
}

UGOTALobbyViewModel* UGotaSessionSubsystem::GetLobbyVM() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGOTAMenuSubsystem* Menu = GI->GetSubsystem<UGOTAMenuSubsystem>())
		{
			return Menu->GetLobbyViewModel();
		}
	}
	return nullptr;
}

void UGotaSessionSubsystem::PushStatus(const FText& Message)
{
	OnStatusMessage.Broadcast(Message);
	if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
	{
		Lobby->SetStatusMessage(Message);
		Lobby->SetErrorMessage(FText::GetEmpty());
	}
}

void UGotaSessionSubsystem::PushError(const FText& Message)
{
	OnStatusMessage.Broadcast(Message);
	if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
	{
		Lobby->SetErrorMessage(Message);
	}
}

bool UGotaSessionSubsystem::HostSession(int32 MaxPlayers, bool bLAN)
{
	SoftMaxPlayers = FMath::Clamp(MaxPlayers, 2, 4);
	bLANPreferred = bLAN;

	// Prefer existing BP session create when authored.
	if (TryCallGameInstance(FName(TEXT("CreateMPSession"))))
	{
		bSessionActive = true;
		bIsHost = true;
		bLocalReady = false;
		if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
		{
			Lobby->ResetLobby();
			Lobby->SetSessionActive(true, true);
			Lobby->SetSelectedArena(SelectedArena);
			Lobby->SetSelectedMode(SelectedMode);
			Lobby->UpsertPlayerSlot(0, FText::FromString(TEXT("Host")), EGOTALobbySlotState::Occupied, true, true, false);
		}
		PushStatus(NSLOCTEXT("GOTA", "HostBP", "Hosting via GameInstance CreateMPSession…"));
		OnHostComplete.Broadcast(true);
		RefreshLobbyProjection();
		return true;
	}

	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		PushError(NSLOCTEXT("GOTA", "NoOSS", "No Online Subsystem — Host unavailable. Use PIE Listen Server."));
		OnHostComplete.Broadcast(false);
		return false;
	}

	Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteHandle);
	CreateCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegate);

	LastHostSettings = MakeShared<FOnlineSessionSettings>();
	LastHostSettings->bIsLANMatch = bLAN;
	LastHostSettings->NumPublicConnections = SoftMaxPlayers;
	LastHostSettings->NumPrivateConnections = 0;
	LastHostSettings->bAllowInvites = false;
	LastHostSettings->bAllowJoinInProgress = true;
	LastHostSettings->bShouldAdvertise = true;
	LastHostSettings->bUsesPresence = false;
	LastHostSettings->bUseLobbiesIfAvailable = false;
	LastHostSettings->Set(FName(TEXT("GOTAArena")), static_cast<int32>(SelectedArena), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastHostSettings->Set(FName(TEXT("GOTAMode")), static_cast<int32>(SelectedMode), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const ULocalPlayer* LP = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	const FUniqueNetIdRepl UserId = LP ? LP->GetPreferredUniqueNetId() : FUniqueNetIdRepl();
	if (!UserId.IsValid() || !Sessions->CreateSession(*UserId, SessionName, *LastHostSettings))
	{
		// Null OSS sometimes accepts NAME_None host without UniqueNetId — retry host id path.
		if (!Sessions->CreateSession(0, SessionName, *LastHostSettings))
		{
			PushError(NSLOCTEXT("GOTA", "HostFail", "CreateSession failed."));
			OnHostComplete.Broadcast(false);
			return false;
		}
	}

	PushStatus(NSLOCTEXT("GOTA", "Hosting", "Creating LAN session…"));
	return true;
}

void UGotaSessionSubsystem::HandleCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteHandle);
	}

	bSessionActive = bWasSuccessful;
	bIsHost = bWasSuccessful;
	if (bWasSuccessful)
	{
		if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
		{
			Lobby->ResetLobby();
			Lobby->SetSessionActive(true, true);
			Lobby->SetSelectedArena(SelectedArena);
			Lobby->SetSelectedMode(SelectedMode);
			Lobby->UpsertPlayerSlot(0, FText::FromString(TEXT("Host")), EGOTALobbySlotState::Occupied, true, true, false);
		}
		PushStatus(NSLOCTEXT("GOTA", "HostOk", "LAN session ready — opening the Champion Hall…"));
		// Become a listen server in the frontend map so allies can connect; the menu presents
		// the Lobby screen on arrival because a session is active.
		const FString LobbyPackage = LobbyMenuMap.GetLongPackageName();
		UGameplayStatics::OpenLevel(this, FName(*LobbyPackage), true, TEXT("listen"));
	}
	else
	{
		PushError(NSLOCTEXT("GOTA", "HostFail2", "Session create failed."));
	}
	OnHostComplete.Broadcast(bWasSuccessful);
}

bool UGotaSessionSubsystem::FindSessions(bool bLAN)
{
	bLANPreferred = bLAN;

	// Legacy FindSessions lives on WBP_MainMenu, not GameInstance — do not reflection-call GI.
	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		PushError(NSLOCTEXT("GOTA", "FindNoOSS", "Find Sessions unavailable (no Online Subsystem). Join by address or use PIE."));
		OnFindComplete.Broadcast(0);
		return false;
	}

	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindCompleteHandle);
	FindCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FindCompleteDelegate);

	LastSearch = MakeShared<FOnlineSessionSearch>();
	LastSearch->bIsLanQuery = bLAN;
	LastSearch->MaxSearchResults = 20;

	const ULocalPlayer* LP = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	const FUniqueNetIdRepl UserId = LP ? LP->GetPreferredUniqueNetId() : FUniqueNetIdRepl();
	bool bStarted = false;
	if (UserId.IsValid())
	{
		bStarted = Sessions->FindSessions(*UserId, LastSearch.ToSharedRef());
	}
	if (!bStarted)
	{
		bStarted = Sessions->FindSessions(0, LastSearch.ToSharedRef());
	}
	if (!bStarted)
	{
		PushError(NSLOCTEXT("GOTA", "FindFail", "FindSessions failed to start."));
		OnFindComplete.Broadcast(0);
		return false;
	}

	PushStatus(NSLOCTEXT("GOTA", "Finding", "Searching LAN sessions…"));
	return true;
}

void UGotaSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindCompleteHandle);
	}

	const int32 Count = (bWasSuccessful && LastSearch.IsValid()) ? LastSearch->SearchResults.Num() : 0;
	if (Count <= 0)
	{
		PushStatus(NSLOCTEXT("GOTA", "FindEmpty", "No LAN sessions found."));
	}
	else
	{
		PushStatus(FText::Format(NSLOCTEXT("GOTA", "FindHits", "Found {0} session(s)."), FText::AsNumber(Count)));
	}
	OnFindComplete.Broadcast(Count);
}

bool UGotaSessionSubsystem::JoinFirstFoundSession()
{
	return JoinSessionAtIndex(0);
}

bool UGotaSessionSubsystem::JoinSessionAtIndex(int32 SearchResultIndex)
{
	// BP GI event is JoinMpSession (legacy); also accept JoinMPSession spelling.
	if (TryCallGameInstance(FName(TEXT("JoinMpSession")))
		|| TryCallGameInstance(FName(TEXT("JoinMPSession"))))
	{
		bSessionActive = true;
		bIsHost = false;
		PushStatus(NSLOCTEXT("GOTA", "JoinBP", "Joining via GameInstance JoinMpSession…"));
		OnJoinComplete.Broadcast(true);
		return true;
	}

	if (!LastSearch.IsValid() || !LastSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		PushError(NSLOCTEXT("GOTA", "JoinNoResult", "No search result selected — Find Sessions first."));
		OnJoinComplete.Broadcast(false);
		return false;
	}

	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		PushError(NSLOCTEXT("GOTA", "JoinNoOSS", "Join unavailable — no Online Subsystem."));
		OnJoinComplete.Broadcast(false);
		return false;
	}

	Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinCompleteHandle);
	JoinCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(JoinCompleteDelegate);

	const ULocalPlayer* LP = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	const FUniqueNetIdRepl UserId = LP ? LP->GetPreferredUniqueNetId() : FUniqueNetIdRepl();
	bool bStarted = false;
	if (UserId.IsValid())
	{
		bStarted = Sessions->JoinSession(*UserId, SessionName, LastSearch->SearchResults[SearchResultIndex]);
	}
	if (!bStarted)
	{
		bStarted = Sessions->JoinSession(0, SessionName, LastSearch->SearchResults[SearchResultIndex]);
	}
	if (!bStarted)
	{
		PushError(NSLOCTEXT("GOTA", "JoinFail", "JoinSession failed to start."));
		OnJoinComplete.Broadcast(false);
		return false;
	}
	PushStatus(NSLOCTEXT("GOTA", "Joining", "Joining session…"));
	return true;
}

void UGotaSessionSubsystem::HandleJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinCompleteHandle);
	}

	const bool bOk = (Result == EOnJoinSessionCompleteResult::Success);
	bSessionActive = bOk;
	bIsHost = false;
	if (bOk)
	{
		FString ConnectInfo;
		if (const IOnlineSessionPtr Sessions = GetSessions())
		{
			if (Sessions->GetResolvedConnectString(InSessionName, ConnectInfo) && !ConnectInfo.IsEmpty())
			{
				if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
				{
					PC->ClientTravel(ConnectInfo, TRAVEL_Absolute);
				}
			}
		}
		if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
		{
			Lobby->SetSessionActive(true, false);
		}
		PushStatus(NSLOCTEXT("GOTA", "JoinOk", "Joined session."));
	}
	else
	{
		FText Reason = NSLOCTEXT("GOTA", "JoinFail2", "Join session failed.");
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			Reason = NSLOCTEXT("GOTA", "JoinFull", "That session is full (4 Wardens maximum).");
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			Reason = NSLOCTEXT("GOTA", "JoinGone", "That session no longer exists. Refresh the list.");
			break;
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			Reason = NSLOCTEXT("GOTA", "JoinNoAddr", "Could not resolve the host address.");
			break;
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			Reason = NSLOCTEXT("GOTA", "JoinAlready", "You are already in a session. Leave it first.");
			break;
		default:
			break;
		}
		PushError(Reason);
	}
	OnJoinComplete.Broadcast(bOk);
}

bool UGotaSessionSubsystem::JoinByAddress(const FString& Address)
{
	if (Address.IsEmpty())
	{
		PushError(NSLOCTEXT("GOTA", "BadAddr", "Enter a host address (ip:port)."));
		return false;
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		bSessionActive = true;
		bIsHost = false;
		PC->ClientTravel(Address, TRAVEL_Absolute);
		PushStatus(FText::Format(NSLOCTEXT("GOTA", "TravelAddr", "Traveling to {0}…"), FText::FromString(Address)));
		OnJoinComplete.Broadcast(true);
		return true;
	}
	return false;
}

void UGotaSessionSubsystem::DestroyCurrentSession()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	// Legacy DestroySession is on BP_WarOfGodsMenuController, not GameInstance.
	if (APlayerController* PC = (bUseBlueprintSessionEvents && GetGameInstance()) ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
	{
		if (UFunction* Fn = PC->FindFunction(FName(TEXT("DestroySession"))))
		{
			PC->ProcessEvent(Fn, nullptr);
			bSessionActive = false;
			bIsHost = false;
			bLocalReady = false;
			if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
			{
				Lobby->ResetLobby();
			}
			PushStatus(NSLOCTEXT("GOTA", "DestroyedPC", "Session destroyed (MenuController)."));
			return;
		}
	}
	if (TryCallGameInstance(FName(TEXT("DestroySession"))))
	{
		bSessionActive = false;
		bIsHost = false;
		bLocalReady = false;
		if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
		{
			Lobby->ResetLobby();
		}
		PushStatus(NSLOCTEXT("GOTA", "DestroyedBP", "Session destroyed (GameInstance)."));
		return;
	}

	if (IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyCompleteHandle);
		DestroyCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(DestroyCompleteDelegate);
		if (!Sessions->DestroySession(SessionName))
		{
			HandleDestroySessionComplete(SessionName, true);
		}
	}
	else
	{
		HandleDestroySessionComplete(SessionName, true);
	}
}

void UGotaSessionSubsystem::HandleDestroySessionComplete(FName InSessionName, bool bWasSuccessful)
{
	if (const IOnlineSessionPtr Sessions = GetSessions())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyCompleteHandle);
	}
	bSessionActive = false;
	bIsHost = false;
	bLocalReady = false;
	if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
	{
		Lobby->ResetLobby();
	}
	PushStatus(NSLOCTEXT("GOTA", "Destroyed", "Left session."));
}

void UGotaSessionSubsystem::SetSelectedArena(EArenaId ArenaId)
{
	SelectedArena = ArenaId;
	if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
	{
		Lobby->SetSelectedArena(ArenaId);
	}
}

void UGotaSessionSubsystem::SetSelectedMode(EArenaMode Mode)
{
	SelectedMode = Mode;
	if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
	{
		Lobby->SetSelectedMode(Mode);
	}
}

void UGotaSessionSubsystem::SetLocalReady(bool bReady)
{
	bLocalReady = bReady;
	if (UGOTALobbyViewModel* Lobby = GetLobbyVM())
	{
		Lobby->SetPlayerReady(0, bReady);
	}
}

FString UGotaSessionSubsystem::BuildTravelOptions(bool bListen) const
{
	FString Options;
	if (SelectedMode == EArenaMode::PvPDuel)
	{
		Options = TEXT("game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3");
	}
	else if (SelectedMode == EArenaMode::PvEHorde)
	{
		Options = TEXT("game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry");
	}
	else if (SelectedMode == EArenaMode::Story)
	{
		Options = TEXT("game=/Script/WarOfGods.GOTAStoryGameMode?Mode=Story");
	}
	else
	{
		Options = TEXT("game=/Script/WarOfGods.ArenaGameMode?Mode=Skirmish?Arena=Quarry");
	}

	if (SelectedArena == EArenaId::AshfallCrucible) { Options = Options.Replace(TEXT("Arena=Quarry"), TEXT("Arena=Ashfall")); }
	else if (SelectedArena == EArenaId::FrostboundCitadel) { Options = Options.Replace(TEXT("Arena=Quarry"), TEXT("Arena=Frostbound")); }
	else if (SelectedArena == EArenaId::VerdantSanctuary) { Options = Options.Replace(TEXT("Arena=Quarry"), TEXT("Arena=Verdant")); }
	else if (SelectedArena == EArenaId::SunscarExpanse) { Options = Options.Replace(TEXT("Arena=Quarry"), TEXT("Arena=Sunscar")); }
	else if (SelectedArena == EArenaId::StormbreakCoast) { Options = Options.Replace(TEXT("Arena=Quarry"), TEXT("Arena=Stormbreak")); }
	else if (SelectedArena == EArenaId::ShatteredDominion) { Options = Options.Replace(TEXT("Arena=Quarry"), TEXT("Arena=Dominion")); }

	if (bListen)
	{
		Options += TEXT("?listen");
	}
	return Options;
}

void UGotaSessionSubsystem::TravelToMatch(bool bAsListenHost)
{
	const FString Options = BuildTravelOptions(bAsListenHost);
	FSoftObjectPath MapPath = MatchMap;
	if (SelectedMode == EArenaMode::Story)
	{
		const FString StoryPackage = TEXT("/Game/Story/Maps/L_GOTA_Story_Prologue");
		if (FPackageName::DoesPackageExist(StoryPackage))
		{
			MapPath = FSoftObjectPath(StoryPackage + TEXT(".L_GOTA_Story_Prologue"));
		}
	}
	const FString MapPackage = MapPath.GetLongPackageName();

	if (bAsListenHost)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(MapPackage + TEXT("?") + Options, true);
			return;
		}
	}
	UGameplayStatics::OpenLevel(this, FName(*MapPackage), true, Options);
}

bool UGotaSessionSubsystem::StartMatchFromLobby()
{
	if (!bIsHost)
	{
		PushError(NSLOCTEXT("GOTA", "StartNotHost", "Only the host can start the match."));
		return false;
	}

	// Prefer existing TravelToMap BP if present (may ignore our options).
	if (TryCallGameInstance(FName(TEXT("TravelToMap"))))
	{
		PushStatus(NSLOCTEXT("GOTA", "TravelBP", "Traveling via GameInstance TravelToMap…"));
		return true;
	}

	TravelToMatch(true);
	PushStatus(NSLOCTEXT("GOTA", "TravelMatch", "Starting match…"));
	return true;
}

void UGotaSessionSubsystem::RefreshLobbyProjection()
{
	UGOTALobbyViewModel* Lobby = GetLobbyVM();
	if (!Lobby)
	{
		return;
	}

	Lobby->SetSessionActive(bSessionActive, bIsHost);
	Lobby->SetSelectedArena(SelectedArena);
	Lobby->SetSelectedMode(SelectedMode);

	UWorld* World = GetWorld();
	AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	if (!GS)
	{
		// Menu-only host: keep slot 0 as local host projection.
		if (bIsHost && bSessionActive)
		{
			Lobby->UpsertPlayerSlot(0, FText::FromString(TEXT("Host")),
				bLocalReady ? EGOTALobbySlotState::Ready : EGOTALobbySlotState::Occupied,
				true, true, bLocalReady);
		}
		return;
	}

	const TArray<APlayerState*>& Players = GS->PlayerArray;
	for (int32 i = 0; i < UGOTALobbyViewModel::MaxLobbyPlayers; ++i)
	{
		if (!Players.IsValidIndex(i) || !Players[i])
		{
			Lobby->ClearPlayerSlot(i);
			continue;
		}
		const APlayerState* PS = Players[i];
		const bool bHostSlot = (i == 0);
		bool bLocal = false;
		if (const APlayerController* PC = PS->GetPlayerController())
		{
			bLocal = PC->IsLocalController();
		}
		Lobby->UpsertPlayerSlot(
			i,
			FText::FromString(PS->GetPlayerName()),
			EGOTALobbySlotState::Occupied,
			bHostSlot && bIsHost,
			bLocal,
			bLocal ? bLocalReady : false);
	}
}

TArray<FGOTASessionResultView> UGotaSessionSubsystem::GetSearchResultViews() const
{
	TArray<FGOTASessionResultView> Views;
	if (!LastSearch.IsValid())
	{
		return Views;
	}
	for (int32 Index = 0; Index < LastSearch->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& Result = LastSearch->SearchResults[Index];
		FGOTASessionResultView View;
		View.ResultIndex = Index;
		View.HostName = Result.Session.OwningUserName.IsEmpty() ? TEXT("LAN HOST") : Result.Session.OwningUserName;
		View.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		View.OpenSlots = Result.Session.NumOpenPublicConnections;
		View.PingMs = Result.PingInMs;
		View.bIsFull = View.OpenSlots <= 0;
		Views.Add(View);
	}
	return Views;
}
