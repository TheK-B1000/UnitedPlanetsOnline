// White Tiger Studios 2024

#include "UI/Widgets/GOTALobbyWidget.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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
		// Only Titan's Quarry is validated for Trial of Champions; no arena picker in lobby yet.
		Btn_SelectArena->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UGOTAMenuButtonWidget* Btn : { Btn_Ready.Get(), Btn_StartMatch.Get(), Btn_Leave.Get(), Btn_Back.Get() })
	{
		TrackFocus(Btn);
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RosterTimer);
	}
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

	if (const UGOTALobbyViewModel* Lobby = GetLobby())
	{
		if (Text_Roster)
		{
			FString Roster;
			for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
			{
				const FGOTALobbyPlayerSlot* Found = Lobby->GetSlots().FindByPredicate(
					[SlotIndex](const FGOTALobbyPlayerSlot& S) { return S.SlotIndex == SlotIndex; });
				if (Found && Found->State != EGOTALobbySlotState::Empty)
				{
					Roster += FString::Printf(TEXT("%d   %s%s%s\n"), SlotIndex + 1, *Found->PlayerName.ToString().ToUpper(),
						Found->bIsHost ? TEXT("   [HOST]") : TEXT(""), Found->bIsReady ? TEXT("   READY") : TEXT(""));
				}
				else
				{
					Roster += FString::Printf(TEXT("%d   OPEN SLOT\n"), SlotIndex + 1);
				}
			}
			Text_Roster->SetText(FText::FromString(Roster));
		}
		if (Text_Status)
		{
			const FText Error = Lobby->GetErrorMessage();
			Text_Status->SetText(Error.IsEmpty() ? Lobby->GetStatusMessage() : Error);
		}
	}
	OnLobbyModelUpdated();
}

UGOTAMenuButtonWidget* UGOTALobbyWidget::GetDefaultFocusButton() const
{
	return (Btn_StartMatch && Btn_StartMatch->IsButtonEnabled()) ? Btn_StartMatch.Get() : Btn_Ready.Get();
}

void UGOTALobbyWidget::OnScreenActivated()
{
	TickRoster();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RosterTimer, this, &UGOTALobbyWidget::TickRoster, 1.f, true);
	}
}

void UGOTALobbyWidget::HideScreen(bool bAnimate)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RosterTimer);
	}
	Super::HideScreen(bAnimate);
}

void UGOTALobbyWidget::TickRoster()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			Sessions->RefreshLobbyProjection();
		}
	}
	HandleLobbyUpdated();
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
	// Leaving a networked lobby must drop the connection / listen socket, not just the UI.
	if (UWorld* World = GetWorld(); World && World->GetNetMode() != NM_Standalone)
	{
		UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/MainMenu/L_GOTA_MainMenu")), true);
		return;
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
