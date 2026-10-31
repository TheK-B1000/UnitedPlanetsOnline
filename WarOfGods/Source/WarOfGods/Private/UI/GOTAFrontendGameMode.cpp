// White Tiger Studios 2024

#include "UI/GOTAFrontendGameMode.h"

#include "Engine/GameInstance.h"
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
