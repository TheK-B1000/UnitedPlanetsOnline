// White Tiger Studios 2024

#include "UI/Widgets/GOTAMultiplayerWidget.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

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
}

UGOTALobbyViewModel* UGOTAMultiplayerWidget::GetLobby() const
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		return Subsystem->GetLobbyViewModel();
	}
	return nullptr;
}

void UGOTAMultiplayerWidget::HandleHost() { RequestHostGame(); }
void UGOTAMultiplayerWidget::HandleJoin() { RequestJoinGame(); }
void UGOTAMultiplayerWidget::HandleFind() { RequestFindSessions(); }
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
