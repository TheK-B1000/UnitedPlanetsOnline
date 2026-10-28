// White Tiger Studios 2024

#include "UI/Widgets/GOTALobbyWidget.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "Engine/GameInstance.h"

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
		Btn_StartMatch->SetLabel(NSLOCTEXT("GOTA", "StartMatch", "START DUEL"));
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
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			Sessions->SetSelectedMode(EArenaMode::PvPDuel);
			Sessions->SetSelectedArena(EArenaId::TitansQuarry);
			Sessions->RefreshLobbyProjection();
		}
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
	UGotaSessionSubsystem* Sessions = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		Sessions = GI->GetSubsystem<UGotaSessionSubsystem>();
	}

	const bool bCanStart = Sessions
		? (Sessions->IsHosting() && Sessions->IsInSession())
		: (GetLobby() && GetLobby()->CanHostStartMatch());

	if (Btn_StartMatch)
	{
		Btn_StartMatch->SetButtonEnabled(bCanStart);
	}
	OnLobbyModelUpdated();
}

void UGOTALobbyWidget::HandleReady()
{
	RequestToggleReady();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			Sessions->SetLocalReady(!Sessions->IsLocalReady());
			Sessions->RefreshLobbyProjection();
			return;
		}
	}
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		const TArray<FGOTALobbyPlayerSlot>& Slots = Lobby->GetSlots();
		const bool bCurrentlyReady = Slots.IsValidIndex(0) ? Slots[0].bIsReady : false;
		Lobby->SetPlayerReady(0, !bCurrentlyReady);
	}
}

void UGOTALobbyWidget::HandleStart()
{
	RequestStartMatch();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			if (!Sessions->IsHosting())
			{
				if (UGOTALobbyViewModel* Lobby = GetLobby())
				{
					Lobby->SetErrorMessage(NSLOCTEXT("GOTA", "OnlyHost", "Only the host can start."));
				}
				return;
			}
			Sessions->SetSelectedMode(EArenaMode::PvPDuel);
			Sessions->StartMatchFromLobby();
			return;
		}
	}
}

void UGOTALobbyWidget::HandleLeave()
{
	RequestLeaveLobby();
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			Sessions->DestroyCurrentSession();
		}
	}
	if (UGOTALobbyViewModel* Lobby = GetLobby())
	{
		Lobby->ResetLobby();
	}
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->NavigateTo(EGOTAMenuScreen::Multiplayer, false);
	}
}

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
