// White Tiger Studios 2024

#include "UI/Widgets/GOTAMultiplayerWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTAMultiplayerWidget::UGOTAMultiplayerWidget()
{
	ScreenId = EGOTAMenuScreen::Multiplayer;
}

UGotaSessionSubsystem* UGOTAMultiplayerWidget::GetSessions() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UGotaSessionSubsystem>() : nullptr;
}

UGOTALobbyViewModel* UGOTAMultiplayerWidget::GetLobby() const
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	return Subsystem ? Subsystem->GetLobbyViewModel() : nullptr;
}

void UGOTAMultiplayerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	auto Bind = [this](UGOTAMenuButtonWidget* Btn, FText Label, FText Sub)
	{
		if (Btn)
		{
			Btn->SetLabel(Label);
			Btn->SetSubtitle(Sub);
			TrackFocus(Btn);
		}
	};
	Bind(Btn_HostGame, NSLOCTEXT("GOTA", "HostGame", "HOST GAME"), NSLOCTEXT("GOTA", "HostSub", "LAN SESSION  •  UP TO 4 WARDENS"));
	Bind(Btn_FindSessions, NSLOCTEXT("GOTA", "FindGames", "FIND GAMES"), NSLOCTEXT("GOTA", "FindSub", "SEARCH YOUR LOCAL NETWORK"));
	Bind(Btn_JoinGame, NSLOCTEXT("GOTA", "JoinGame", "JOIN GAME"), NSLOCTEXT("GOTA", "JoinSub", "JOIN THE SELECTED SESSION"));
	Bind(Btn_OpenLobby, NSLOCTEXT("GOTA", "OpenLobby", "LOBBY"), NSLOCTEXT("GOTA", "LobbySub", "RETURN TO YOUR CHAMPION HALL"));
	Bind(Btn_Back, NSLOCTEXT("GOTA", "Back", "BACK"), FText::GetEmpty());

	if (Btn_HostGame) { Btn_HostGame->OnActivated.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleHost); }
	if (Btn_FindSessions) { Btn_FindSessions->OnActivated.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleFind); }
	if (Btn_JoinGame) { Btn_JoinGame->OnActivated.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleJoin); }
	if (Btn_OpenLobby) { Btn_OpenLobby->OnActivated.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleOpenLobby); }
	if (Btn_Back) { Btn_Back->OnActivated.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleBack); }
	if (Btn_InviteFriends)
	{
		// No platform friends service — the control is not offered at all.
		Btn_InviteFriends->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGOTAMultiplayerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGotaSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->OnHostComplete.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleHostComplete);
		Sessions->OnFindComplete.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleFindComplete);
		Sessions->OnJoinComplete.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleJoinComplete);
		Sessions->OnStatusMessage.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleStatus);
	}
}

void UGOTAMultiplayerWidget::NativeDestruct()
{
	if (UGotaSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->OnHostComplete.RemoveAll(this);
		Sessions->OnFindComplete.RemoveAll(this);
		Sessions->OnJoinComplete.RemoveAll(this);
		Sessions->OnStatusMessage.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UGOTAMultiplayerWidget::OnScreenActivated()
{
	bOperationPending = false;
	RebuildResults();
	RefreshButtons();
	if (Text_Status)
	{
		const UGotaSessionSubsystem* Sessions = GetSessions();
		Text_Status->SetText(Sessions && Sessions->IsSessionBackendAvailable()
			? NSLOCTEXT("GOTA", "MPIdle", "LAN play — host a session or search your local network. Maximum party: 4 Wardens.")
			: NSLOCTEXT("GOTA", "MPNoBackend", "No session backend is available in this build."));
	}
}

UGOTAMenuButtonWidget* UGOTAMultiplayerWidget::GetDefaultFocusButton() const
{
	return Btn_HostGame;
}

void UGOTAMultiplayerWidget::SetBusy(bool bBusy)
{
	bOperationPending = bBusy;
	RefreshButtons();
}

void UGOTAMultiplayerWidget::RefreshButtons()
{
	const UGotaSessionSubsystem* Sessions = GetSessions();
	const bool bBackend = Sessions && Sessions->IsSessionBackendAvailable();
	const bool bInSession = Sessions && Sessions->IsInSession();
	if (Btn_HostGame) { Btn_HostGame->SetButtonEnabled(bBackend && !bInSession && !bOperationPending); }
	if (Btn_FindSessions) { Btn_FindSessions->SetButtonEnabled(bBackend && !bInSession && !bOperationPending); }
	if (Btn_JoinGame) { Btn_JoinGame->SetButtonEnabled(bBackend && !bInSession && !bOperationPending && SelectedResult != INDEX_NONE); }
	if (Btn_OpenLobby) { Btn_OpenLobby->SetButtonEnabled(bInSession); }
}

void UGOTAMultiplayerWidget::RebuildResults()
{
	if (!Box_Sessions)
	{
		return;
	}
	Box_Sessions->ClearChildren();
	SelectedResult = INDEX_NONE;
	const UGotaSessionSubsystem* Sessions = GetSessions();
	if (!Sessions || !EntryClass)
	{
		return;
	}
	for (const FGOTASessionResultView& View : Sessions->GetSearchResultViews())
	{
		UGOTAMenuButtonWidget* Entry = CreateWidget<UGOTAMenuButtonWidget>(this, EntryClass);
		if (!Entry)
		{
			continue;
		}
		Entry->Style = EGOTAButtonStyle::ListEntry;
		Entry->Payload = View.ResultIndex;
		Entry->SetLabel(FText::FromString(View.HostName.ToUpper()));
		const int32 Occupied = FMath::Max(0, View.MaxPlayers - View.OpenSlots);
		Entry->SetSubtitle(View.bIsFull
			? FText::Format(NSLOCTEXT("GOTA", "SessionFullFmt", "FULL  {0}/{1}  •  {2} ms"), FText::AsNumber(Occupied), FText::AsNumber(View.MaxPlayers), FText::AsNumber(View.PingMs))
			: FText::Format(NSLOCTEXT("GOTA", "SessionOpenFmt", "{0}/{1} WARDENS  •  {2} ms"), FText::AsNumber(Occupied), FText::AsNumber(View.MaxPlayers), FText::AsNumber(View.PingMs)));
		Entry->SetButtonEnabled(!View.bIsFull);
		Entry->OnFocused.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleResultFocused);
		Entry->OnActivated.AddUniqueDynamic(this, &UGOTAMultiplayerWidget::HandleResultActivated);
		TrackFocus(Entry);
		Box_Sessions->AddChild(Entry);
	}
}

void UGOTAMultiplayerWidget::HandleResultFocused(UGOTAMenuButtonWidget* Button)
{
	SelectedResult = Button ? Button->Payload : INDEX_NONE;
	if (Box_Sessions)
	{
		for (UWidget* Child : Box_Sessions->GetAllChildren())
		{
			if (UGOTAMenuButtonWidget* Entry = Cast<UGOTAMenuButtonWidget>(Child))
			{
				Entry->SetSelected(Entry == Button);
			}
		}
	}
	RefreshButtons();
}

void UGOTAMultiplayerWidget::HandleResultActivated()
{
	HandleJoin();
}

void UGOTAMultiplayerWidget::HandleHost()
{
	RequestHostGame();
	UGotaSessionSubsystem* Sessions = GetSessions();
	if (!Sessions)
	{
		return;
	}
	Sessions->SetSelectedMode(EArenaMode::PvPDuel);
	Sessions->SetSelectedArena(EArenaId::TitansQuarry);
	SetBusy(true);
	if (!Sessions->HostSession(4, true))
	{
		SetBusy(false);
	}
}

void UGOTAMultiplayerWidget::HandleFind()
{
	RequestFindSessions();
	UGotaSessionSubsystem* Sessions = GetSessions();
	if (!Sessions)
	{
		return;
	}
	SetBusy(true);
	if (!Sessions->FindSessions(true))
	{
		SetBusy(false);
	}
}

void UGOTAMultiplayerWidget::HandleJoin()
{
	RequestJoinGame();
	UGotaSessionSubsystem* Sessions = GetSessions();
	if (!Sessions || SelectedResult == INDEX_NONE)
	{
		return;
	}
	SetBusy(true);
	if (!Sessions->JoinSessionAtIndex(SelectedResult))
	{
		SetBusy(false);
	}
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
		Subsystem->PlayUISound(EGOTAMenuSound::Back);
		Subsystem->GoBack(false);
	}
}

void UGOTAMultiplayerWidget::HandleHostComplete(bool bSuccess)
{
	// Success travels to the listen lobby (session subsystem); failure keeps us here.
	if (!bSuccess)
	{
		SetBusy(false);
		if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
		{
			Subsystem->PlayUISound(EGOTAMenuSound::Error);
		}
	}
}

void UGOTAMultiplayerWidget::HandleFindComplete(int32 ResultCount)
{
	SetBusy(false);
	RebuildResults();
	RefreshButtons();
}

void UGOTAMultiplayerWidget::HandleJoinComplete(bool bSuccess)
{
	if (!bSuccess)
	{
		SetBusy(false);
		if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
		{
			Subsystem->PlayUISound(EGOTAMenuSound::Error);
		}
	}
}

void UGOTAMultiplayerWidget::HandleStatus(FText Message)
{
	if (Text_Status)
	{
		Text_Status->SetText(Message);
	}
}
