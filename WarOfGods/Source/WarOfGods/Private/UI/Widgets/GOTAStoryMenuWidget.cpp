// White Tiger Studios 2024

#include "UI/Widgets/GOTAStoryMenuWidget.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "Engine/GameInstance.h"

UGOTAStoryMenuWidget::UGOTAStoryMenuWidget()
{
	ScreenId = EGOTAMenuScreen::StoryMode;
}

void UGOTAStoryMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_NewGameSolo)
	{
		Btn_NewGameSolo->SetLabel(NSLOCTEXT("GOTA", "StoryNewGame", "NEW GAME"));
		Btn_NewGameSolo->OnActivated.AddDynamic(this, &UGOTAStoryMenuWidget::HandleNewGameSolo);
	}
	if (Btn_NewGameCoop)
	{
		Btn_NewGameCoop->SetLabel(NSLOCTEXT("GOTA", "StoryHostCoop", "HOST COOPERATIVE STORY"));
		Btn_NewGameCoop->OnActivated.AddDynamic(this, &UGOTAStoryMenuWidget::HandleNewGameCoop);
	}
	if (Btn_JoinCoop)
	{
		Btn_JoinCoop->SetLabel(NSLOCTEXT("GOTA", "StoryJoinCoop", "JOIN COOPERATIVE STORY"));
		Btn_JoinCoop->OnActivated.AddDynamic(this, &UGOTAStoryMenuWidget::HandleJoinCoop);
	}
	if (Btn_Continue)
	{
		Btn_Continue->SetLabel(NSLOCTEXT("GOTA", "StoryContinue", "CONTINUE"));
		Btn_Continue->OnActivated.AddDynamic(this, &UGOTAStoryMenuWidget::HandleContinue);
	}
	if (Btn_Back)
	{
		Btn_Back->SetLabel(NSLOCTEXT("GOTA", "Back", "BACK"));
		Btn_Back->OnActivated.AddDynamic(this, &UGOTAStoryMenuWidget::HandleBack);
	}
}

void UGOTAStoryMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshContinueAvailability();
}

void UGOTAStoryMenuWidget::RefreshContinueAvailability()
{
	const bool bCanContinue = GetMenuSubsystem() && GetMenuSubsystem()->HasValidContinueSave();
	if (Btn_Continue)
	{
		Btn_Continue->SetButtonEnabled(bCanContinue);
		Btn_Continue->SetVisibility(bCanContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UGOTAStoryMenuWidget::HandleNewGameSolo()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->RequestStartStoryPrologue(false);
	}
}

void UGOTAStoryMenuWidget::HandleNewGameCoop()
{
	// Host LAN session then travel into Story GameMode with listen.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			Sessions->SetSelectedMode(EArenaMode::Story);
			Sessions->HostSession(4, true);
		}
	}
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->RequestStartStoryPrologue(true);
	}
}

void UGOTAStoryMenuWidget::HandleJoinCoop()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			if (Sessions->FindSessions(true))
			{
				Sessions->JoinFirstFoundSession();
				return;
			}
		}
	}
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->NavigateTo(EGOTAMenuScreen::Multiplayer, false);
	}
}

void UGOTAStoryMenuWidget::HandleContinue()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->RequestContinueStory();
	}
}

void UGOTAStoryMenuWidget::HandleBack()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->GoBack(false);
	}
}
