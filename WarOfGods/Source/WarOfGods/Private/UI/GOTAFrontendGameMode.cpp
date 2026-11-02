// White Tiger Studios 2024

#include "UI/GOTAFrontendGameMode.h"

#include "Characters/GOTAHeroRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/GOTAPlayerCameraManager.h"

AGOTAFrontendPlayerController::AGOTAFrontendPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	PlayerCameraManagerClass = AGOTAPlayerCameraManager::StaticClass();
}

void AGOTAFrontendPlayerController::ServerSetLobbyHero_Implementation(const FString& HeroName)
{
	UGameInstance* GI = GetGameInstance();
	UGotaSessionSubsystem* Sessions = GI ? GI->GetSubsystem<UGotaSessionSubsystem>() : nullptr;
	const UGOTAHeroRegistry* Registry = GI ? GI->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	if (!Sessions || !Registry)
	{
		return;
	}
	const FName HeroId(*HeroName);
	FText Reason;
	if (!Registry->ValidateSelection(HeroId, Sessions->GetSelectedMode(), Reason))
	{
		UE_LOG(LogTemp, Warning, TEXT("Lobby hero '%s' rejected (%s)"), *HeroName, *Reason.ToString());
		return;
	}
	Sessions->RememberLobbyHero(this, HeroId);
	UE_LOG(LogTemp, Log, TEXT("Lobby hero %s stored for %s"), *HeroId.ToString(), *GetNameSafe(this));
}

void AGOTAFrontendPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		SetShowMouseCursor(true);
		// Covers the PIE path where PostLoadMapWithWorld is not broadcast for the play world.
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UGOTAMenuSubsystem* Menu = GI->GetSubsystem<UGOTAMenuSubsystem>())
			{
				Menu->PresentFrontendUI();
			}
		}
	}
}

AGOTAFrontendGameMode::AGOTAFrontendGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AGOTAFrontendPlayerController::StaticClass();
	bStartPlayersAsSpectators = true;
}

void AGOTAFrontendGameMode::BeginPlay()
{
	Super::BeginPlay();
	RefreshLobby();
}

void AGOTAFrontendGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= MaxLobbyPlayers)
	{
		ErrorMessage = TEXT("Champion Hall is full (4 Wardens maximum).");
	}
}

void AGOTAFrontendGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	RefreshLobby();
}

void AGOTAFrontendGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	RefreshLobby();
}

void AGOTAFrontendGameMode::RefreshLobby() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			if (Sessions->IsInSession())
			{
				Sessions->RefreshLobbyProjection();
			}
		}
	}
}

static void StripUrlOption(FString& Url, const TCHAR* Key)
{
	const FString Assigned = FString::Printf(TEXT("?%s="), Key);
	int32 At = Url.Find(Assigned, ESearchCase::IgnoreCase);
	if (At != INDEX_NONE)
	{
		int32 End = Url.Find(TEXT("?"), ESearchCase::IgnoreCase, ESearchDir::FromStart, At + Assigned.Len());
		if (End == INDEX_NONE)
		{
			End = Url.Len();
		}
		Url.RemoveAt(At, End - At);
		return;
	}
	const FString Flag = FString::Printf(TEXT("?%s"), Key);
	At = Url.Find(Flag, ESearchCase::IgnoreCase);
	if (At != INDEX_NONE)
	{
		const int32 Next = At + Flag.Len();
		if (Next >= Url.Len() || Url[Next] == TEXT('?') || Url[Next] == TEXT('#'))
		{
			Url.RemoveAt(At, Flag.Len());
		}
	}
}

APlayerController* AGOTAFrontendGameMode::ProcessClientTravel(FString& URL, bool bSeamless, bool bAbsolute)
{
	UGotaSessionSubsystem* Sessions = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGotaSessionSubsystem>() : nullptr;
	APlayerController* LocalPlayer = nullptr;
	// Remote URLs are copied before the host's own hero is written onto the shared URL.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !Cast<UNetConnection>(PC->Player))
		{
			if (PC)
			{
				LocalPlayer = PC;
			}
			continue;
		}
		FString ClientUrl = URL;
		StripUrlOption(ClientUrl, TEXT("listen"));
		StripUrlOption(ClientUrl, TEXT("Hero"));
		const FName Hero = Sessions ? Sessions->GetLobbyHero(PC) : NAME_None;
		if (!Hero.IsNone())
		{
			ClientUrl += FString::Printf(TEXT("?Hero=%s"), *Hero.ToString());
		}
		UE_LOG(LogTemp, Log, TEXT("Menu travel: %s -> %s"), *GetNameSafe(PC), *ClientUrl);
		PC->ClientTravel(ClientUrl, TRAVEL_Relative, bSeamless);
	}
	if (LocalPlayer)
	{
		StripUrlOption(URL, TEXT("Hero"));
		const FName Hero = Sessions ? Sessions->GetLobbyHero(LocalPlayer) : NAME_None;
		if (!Hero.IsNone())
		{
			URL += FString::Printf(TEXT("?Hero=%s"), *Hero.ToString());
		}
		UE_LOG(LogTemp, Log, TEXT("Menu travel: host -> %s"), *URL);
		LocalPlayer->PreClientTravel(URL, bAbsolute ? TRAVEL_Absolute : TRAVEL_Relative, bSeamless);
	}
	return LocalPlayer;
}
