// White Tiger Studios 2024

#include "UI/Widgets/GOTAMultiplayerWidget.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UGOTAMultiplayerWidget::UGOTAMultiplayerWidget()
{
	ScreenId = EGOTAMenuScreen::Multiplayer;
}

void UGOTAMultiplayerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Btn_HostGame)
	{
		Btn_HostGame->SetLabel(NSLOCTEXT("GOTA", "HostGame", "HOST GAME"));
		Btn_HostGame->OnActivated.AddDynamic(this, &UGOTAMultiplayerWidget::HandleHost);
	}
	if (Btn_JoinGame)
	{
		Btn_JoinGame->SetLabel(NSLOCTEXT("GOTA", "JoinGame", "JOIN GAME"));
		Btn_JoinGame->OnActivated.AddDynamic(this, &UGOTAMultiplayerWidget::HandleJoin);
	}
	if (Btn_FindSessions)
	{
		Btn_FindSessions->SetLabel(NSLOCTEXT("GOTA", "FindSessions", "FIND SESSIONS"));
		Btn_FindSessions->OnActivated.AddDynamic(this, &UGOTAMultiplayerWidget::HandleFind);
	}
	if (Btn_InviteFriends)
	{
		Btn_InviteFriends->SetLabel(NSLOCTEXT("GOTA", "InviteFriends", "INVITE FRIENDS"));
		Btn_InviteFriends->OnActivated.AddDynamic(this, &UGOTAMultiplayerWidget::HandleInvite);
	}
	if (Btn_OpenLobby)
	{
		Btn_OpenLobby->SetLabel(NSLOCTEXT("GOTA", "OpenLobby", "CHAMPION HALL"));
		Btn_OpenLobby->OnActivated.AddDynamic(this, &UGOTAMultiplayerWidget::HandleOpenLobby);
	}
	if (Btn_Back)
	{
		Btn_Back->SetLabel(NSLOCTEXT("GOTA", "Back", "BACK"));
		Btn_Back->OnActivated.AddDynamic(this, &UGOTAMultiplayerWidget::HandleBack);
	}
}

void UGOTAMultiplayerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UGotaSessionSubsystem* Sessions = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		Sessions = GI->GetSubsystem<UGotaSessionSubsystem>();
	}

	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		const bool bInvites = Lobby->AreFriendInvitesSupported();
		if (Btn_InviteFriends)
		{
			Btn_InviteFriends->SetButtonEnabled(bInvites);
			if (!bInvites)
			{
				Btn_InviteFriends->SetLabel(NSLOCTEXT("GOTA", "InviteUnavailable", "INVITE (UNAVAILABLE)"));
			}
		}
	}

	const bool bFind = Sessions && Sessions->IsFindSessionsSupported();
	if (Btn_FindSessions)
	{
		Btn_FindSessions->SetButtonEnabled(bFind);
		if (!bFind)
		{
			Btn_FindSessions->SetLabel(NSLOCTEXT("GOTA", "FindUnavailable", "FIND (UNAVAILABLE)"));
		}
	}
}

UGOTALobbyViewModel* UGOTAMultiplayerWidget::GetLobby() const
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		return Subsystem->GetLobbyViewModel();
	}
	return nullptr;
}

void UGOTAMultiplayerWidget::HandleHost()
{
	RequestHostGame();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			Sessions->SetSelectedMode(EArenaMode::PvPDuel);
			Sessions->SetSelectedArena(EArenaId::TitansQuarry);
			if (Sessions->HostSession(4, true))
			{
				if (UGOTAMenuSubsystem* Menu = GetMenuSubsystem())
				{
					Menu->NavigateTo(EGOTAMenuScreen::Lobby, false);
				}
				return;
			}
		}
	}
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		Lobby->SetErrorMessage(NSLOCTEXT("GOTA", "HostFailed", "Host failed — check Online Subsystem Null / PIE Listen Server."));
	}
}

void UGOTAMultiplayerWidget::HandleJoin()
{
	RequestJoinGame();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			if (Sessions->JoinFirstFoundSession())
			{
				if (UGOTAMenuSubsystem* Menu = GetMenuSubsystem())
				{
					Menu->NavigateTo(EGOTAMenuScreen::Lobby, false);
				}
				return;
			}
		}
	}
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		Lobby->SetErrorMessage(NSLOCTEXT("GOTA", "JoinNeedFind", "Find Sessions first, or join via console: open <ip>"));
	}
}

void UGOTAMultiplayerWidget::HandleFind()
{
	RequestFindSessions();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			if (!Sessions->IsFindSessionsSupported())
			{
				if (UGOTALobbyViewModel* Lobby = GetLobby())
				{
					Lobby->SetErrorMessage(NSLOCTEXT("GOTA", "FindUnsupported", "Session discovery unavailable — use PIE multiplayer or Join by address."));
				}
				return;
			}
			Sessions->FindSessions(true);
			return;
		}
	}
}

void UGOTAMultiplayerWidget::HandleInvite()
{
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		if (!Lobby->AreFriendInvitesSupported())
		{
			Lobby->SetErrorMessage(NSLOCTEXT("GOTA", "NoInvites", "Friend invites require Online Subsystem (Steam/EOS) — not enabled."));
			return;
		}
	}
	RequestInviteFriends();
}

void UGOTAMultiplayerWidget::HandleOpenLobby()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->NavigateTo(EGOTAMenuScreen::Lobby, false);
	}
}

void UGOTAMultiplayerWidget::HandleBack()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->GoBack(false);
	}
}
