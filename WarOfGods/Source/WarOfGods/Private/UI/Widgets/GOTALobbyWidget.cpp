// White Tiger Studios 2024

#include "UI/Widgets/GOTALobbyWidget.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTALobbyWidget::UGOTALobbyWidget()
{
	ScreenId = EGOTAMenuScreen::Lobby;
}

void UGOTALobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Btn_Ready)
	{
		Btn_Ready->SetLabel(NSLOCTEXT("GOTA", "Ready", "READY"));
		Btn_Ready->OnActivated.AddDynamic(this, &UGOTALobbyWidget::HandleReady);
	}
	if (Btn_StartMatch)
	{
		Btn_StartMatch->SetLabel(NSLOCTEXT("GOTA", "StartMatch", "START MATCH"));
		Btn_StartMatch->OnActivated.AddDynamic(this, &UGOTALobbyWidget::HandleStart);
	}
	if (Btn_Leave)
	{
		Btn_Leave->SetLabel(NSLOCTEXT("GOTA", "LeaveLobby", "LEAVE LOBBY"));
		Btn_Leave->OnActivated.AddDynamic(this, &UGOTALobbyWidget::HandleLeave);
	}
	if (Btn_SelectArena)
	{
		Btn_SelectArena->SetLabel(NSLOCTEXT("GOTA", "SelectArena", "SELECT ARENA"));
		Btn_SelectArena->OnActivated.AddDynamic(this, &UGOTALobbyWidget::HandleSelectArena);
	}
	if (Btn_Back)
	{
		Btn_Back->SetLabel(NSLOCTEXT("GOTA", "Back", "BACK"));
		Btn_Back->OnActivated.AddDynamic(this, &UGOTALobbyWidget::HandleBack);
	}
}

void UGOTALobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		Lobby->OnLobbyUpdated.AddDynamic(this, &UGOTALobbyWidget::HandleLobbyUpdated);
		HandleLobbyUpdated();
	}
}

void UGOTALobbyWidget::NativeDestruct()
{
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		Lobby->OnLobbyUpdated.RemoveAll(this);
	}
	Super::NativeDestruct();
}

UGOTALobbyViewModel* UGOTALobbyWidget::GetLobby() const
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		return Subsystem->GetLobbyViewModel();
	}
	return nullptr;
}

void UGOTALobbyWidget::HandleLobbyUpdated()
{
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		if (Btn_StartMatch)
		{
			Btn_StartMatch->SetButtonEnabled(Lobby->CanHostStartMatch());
		}
	}
	OnLobbyModelUpdated();
}

void UGOTALobbyWidget::HandleReady() { RequestToggleReady(); }
void UGOTALobbyWidget::HandleStart()
{
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		if (!Lobby->CanHostStartMatch())
		{
			return;
		}
	}
	RequestStartMatch();
}
void UGOTALobbyWidget::HandleLeave() { RequestLeaveLobby(); }

void UGOTALobbyWidget::HandleSelectArena()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->NavigateTo(EGOTAMenuScreen::ArenaSelection, false);
	}
}

void UGOTALobbyWidget::HandleBack()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->GoBack(false);
	}
}
