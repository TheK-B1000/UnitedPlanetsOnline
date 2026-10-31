// White Tiger Studios 2024

#include "UI/Widgets/GOTAMainMenuWidget.h"
#include "UI/GOTAMenuContentDefinition.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

UGOTAMainMenuWidget::UGOTAMainMenuWidget()
{
	ScreenId = EGOTAMenuScreen::MainMenu;
}

void UGOTAMainMenuWidget::ConfigureAction(UGOTAMenuButtonWidget* Button, EGOTAMenuAction Action, bool bCard)
{
	if (!Button)
	{
		return;
	}
	Button->MenuAction = Action;
	Button->bUseCardContent = bCard;
	Button->Style = bCard ? EGOTAButtonStyle::FeatureCard : EGOTAButtonStyle::NavRow;
	TrackFocus(Button);
}

void UGOTAMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ConfigureAction(Btn_Continue, EGOTAMenuAction::Continue, false);
	ConfigureAction(Btn_NewStory, EGOTAMenuAction::Story, false);
	ConfigureAction(Btn_PvEArena, EGOTAMenuAction::Arena, false);
	ConfigureAction(Btn_Multiplayer, EGOTAMenuAction::Multiplayer, false);
	ConfigureAction(Btn_Settings, EGOTAMenuAction::Settings, false);
	ConfigureAction(Btn_Extras, EGOTAMenuAction::Extras, false);
	ConfigureAction(Btn_Quit, EGOTAMenuAction::Quit, false);

	ConfigureAction(Card_Story, EGOTAMenuAction::Story, true);
	ConfigureAction(Card_Arena, EGOTAMenuAction::Arena, true);
	ConfigureAction(Card_Multiplayer, EGOTAMenuAction::Multiplayer, true);
	ConfigureAction(Card_Settings, EGOTAMenuAction::Settings, true);
	ConfigureAction(Card_Extras, EGOTAMenuAction::Extras, true);

	// Social controls: no friends / party backend exists (OSS Null) — shown, honestly disabled.
	const FText SocialUnavailable = NSLOCTEXT("GOTA", "SocialUnavailable",
		"Friends and party invites require an online platform service, which is not available in this build.");
	for (UGOTAMenuButtonWidget* Social : { Btn_Friends.Get(), Btn_Plus.Get() })
	{
		if (Social)
		{
			Social->SetButtonEnabled(false);
			Social->SetToolTipText(SocialUnavailable);
		}
	}

	if (const UGOTAMenuContentDefinition* Content = GetMenuContent())
	{
		if (Text_GameSubtitle && !Content->GameSubtitle.IsEmpty())
		{
			Text_GameSubtitle->SetText(Content->GameSubtitle);
		}
	}
}

void UGOTAMainMenuWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MessageTimer);
	}
	Super::NativeDestruct();
}

void UGOTAMainMenuWidget::OnScreenActivated()
{
	RefreshContinueAvailability();
	RefreshProfile();
	AdvanceInstitutionalMessage();
	if (UWorld* World = GetWorld())
	{
		const UGOTAMenuContentDefinition* Content = GetMenuContent();
		if (Content && Content->InstitutionalMessages.Num() > 1 && !MessageTimer.IsValid())
		{
			World->GetTimerManager().SetTimer(MessageTimer, this, &UGOTAMainMenuWidget::AdvanceInstitutionalMessage, 9.f, true);
		}
	}
}

UGOTAMenuButtonWidget* UGOTAMainMenuWidget::GetDefaultFocusButton() const
{
	if (Btn_Continue && Btn_Continue->IsButtonEnabled())
	{
		return Btn_Continue;
	}
	return Btn_NewStory ? Btn_NewStory.Get() : Btn_PvEArena.Get();
}

void UGOTAMainMenuWidget::RefreshContinueAvailability()
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	if (!Btn_Continue || !Subsystem)
	{
		return;
	}
	FText Reason;
	const bool bCanContinue = Subsystem->IsMenuActionAvailable(EGOTAMenuAction::Continue, Reason);
	Btn_Continue->SetButtonEnabled(bCanContinue);
	if (bCanContinue)
	{
		Btn_Continue->ApplyDestinationContent();
		const FText Summary = Subsystem->GetContinueSummary();
		if (!Summary.IsEmpty())
		{
			Btn_Continue->SetToolTipText(Summary);
		}
	}
	else
	{
		Btn_Continue->SetSubtitle(NSLOCTEXT("GOTA", "NoMissionSub", "NO MISSION TO RESUME"));
		Btn_Continue->SetToolTipText(Reason);
	}
}

void UGOTAMainMenuWidget::RefreshProfile()
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	if (!Subsystem)
	{
		return;
	}
	const FGOTAProfileView Profile = Subsystem->GetProfileView();
	if (Text_ProfileName)
	{
		Text_ProfileName->SetText(Profile.DisplayName);
	}
	if (Text_ProfileRank)
	{
		Text_ProfileRank->SetText(Profile.bHasProgression
			? FText::Format(NSLOCTEXT("GOTA", "LevelFmt", "LEVEL {0}"), FText::AsNumber(Profile.Level))
			: Profile.RankTitle);
	}
	if (Bar_ProfileProgress)
	{
		Bar_ProfileProgress->SetVisibility(Profile.bHasProgression ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Bar_ProfileProgress->SetPercent(Profile.LevelProgress);
	}
}

void UGOTAMainMenuWidget::AdvanceInstitutionalMessage()
{
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	if (!Text_Slogan || !Content || Content->InstitutionalMessages.Num() == 0)
	{
		return;
	}
	MessageIndex = MessageIndex % Content->InstitutionalMessages.Num();
	Text_Slogan->SetText(Content->InstitutionalMessages[MessageIndex]);
	++MessageIndex;
}
