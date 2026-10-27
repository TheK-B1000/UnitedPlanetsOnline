// White Tiger Studios 2024

#include "UI/Widgets/GOTAStoryMenuWidget.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTAStoryMenuWidget::UGOTAStoryMenuWidget()
{
	ScreenId = EGOTAMenuScreen::StoryMode;
}

void UGOTAStoryMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_NewGameSolo)
	{
		Btn_NewGameSolo->SetLabel(NSLOCTEXT("GOTA", "StorySolo", "START CHAPTER 1 — SOLO"));
		Btn_NewGameSolo->OnActivated.AddDynamic(this, &UGOTAStoryMenuWidget::HandleNewGameSolo);
	}
	if (Btn_NewGameCoop)
	{
		Btn_NewGameCoop->SetLabel(NSLOCTEXT("GOTA", "StoryCoop", "START CHAPTER 1 — HOST CO-OP"));
		Btn_NewGameCoop->OnActivated.AddDynamic(this, &UGOTAStoryMenuWidget::HandleNewGameCoop);
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
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->RequestStartStoryPrologue(true);
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
