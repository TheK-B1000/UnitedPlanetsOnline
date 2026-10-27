// White Tiger Studios 2024

#include "UI/Widgets/GOTAMainMenuWidget.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTAMainMenuWidget::UGOTAMainMenuWidget()
{
	ScreenId = EGOTAMenuScreen::MainMenu;
}

void UGOTAMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_Continue)
	{
		Btn_Continue->SetLabel(NSLOCTEXT("GOTA", "Continue", "CONTINUE"));
		Btn_Continue->OnActivated.AddDynamic(this, &UGOTAMainMenuWidget::HandleContinue);
	}
	if (Btn_NewStory)
	{
		Btn_NewStory->SetLabel(NSLOCTEXT("GOTA", "StoryMode", "STORY MODE"));
		Btn_NewStory->OnActivated.AddDynamic(this, &UGOTAMainMenuWidget::HandleNewStory);
	}
	if (Btn_PvEArena)
	{
		Btn_PvEArena->SetLabel(NSLOCTEXT("GOTA", "PvEArena", "PVE ARENA"));
		Btn_PvEArena->OnActivated.AddDynamic(this, &UGOTAMainMenuWidget::HandlePvEArena);
	}
	if (Btn_Multiplayer)
	{
		Btn_Multiplayer->SetLabel(NSLOCTEXT("GOTA", "Multiplayer", "MULTIPLAYER"));
		Btn_Multiplayer->OnActivated.AddDynamic(this, &UGOTAMainMenuWidget::HandleMultiplayer);
	}
	if (Btn_Settings)
	{
		Btn_Settings->SetLabel(NSLOCTEXT("GOTA", "Settings", "SETTINGS"));
		Btn_Settings->OnActivated.AddDynamic(this, &UGOTAMainMenuWidget::HandleSettings);
	}
	if (Btn_Extras)
	{
		Btn_Extras->SetLabel(NSLOCTEXT("GOTA", "Extras", "EXTRAS"));
		Btn_Extras->OnActivated.AddDynamic(this, &UGOTAMainMenuWidget::HandleExtras);
	}
	if (Btn_Quit)
	{
		Btn_Quit->SetLabel(NSLOCTEXT("GOTA", "Quit", "QUIT"));
		Btn_Quit->OnActivated.AddDynamic(this, &UGOTAMainMenuWidget::HandleQuit);
	}
}

void UGOTAMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshContinueAvailability();
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->SetRootMenuWidget(this);
		if (Btn_Extras)
		{
			const bool bExtras = Subsystem->IsExtrasAvailable();
			Btn_Extras->SetButtonEnabled(bExtras);
			Btn_Extras->SetVisibility(bExtras ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void UGOTAMainMenuWidget::RefreshContinueAvailability()
{
	const bool bCanContinue = GetMenuSubsystem() && GetMenuSubsystem()->HasValidContinueSave();
	if (Btn_Continue)
	{
		Btn_Continue->SetButtonEnabled(bCanContinue);
		Btn_Continue->SetVisibility(bCanContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UGOTAMainMenuWidget::HandleContinue()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->RequestContinueStory();
	}
}

void UGOTAMainMenuWidget::HandleNewStory()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->RequestStoryMode();
	}
}

void UGOTAMainMenuWidget::HandlePvEArena()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->RequestPvEArena();
	}
}

void UGOTAMainMenuWidget::HandleMultiplayer()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->NavigateTo(EGOTAMenuScreen::Multiplayer, false);
	}
}

void UGOTAMainMenuWidget::HandleSettings()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->NavigateTo(EGOTAMenuScreen::Settings, false);
	}
}

void UGOTAMainMenuWidget::HandleExtras()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		if (!Subsystem->IsExtrasAvailable())
		{
			return;
		}
		Subsystem->NavigateTo(EGOTAMenuScreen::Extras, false);
	}
}

void UGOTAMainMenuWidget::HandleQuit()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->RequestQuitGame();
	}
}
