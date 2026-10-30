// White Tiger Studios 2024

#include "UI/Widgets/GOTAStoryMenuWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "UI/GOTAMenuContentDefinition.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTAStoryMenuWidget::UGOTAStoryMenuWidget()
{
	ScreenId = EGOTAMenuScreen::StoryMode;
}

void UGOTAStoryMenuWidget::NativeOnInitialized()
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
	Bind(Btn_NewGameSolo, NSLOCTEXT("GOTA", "NewSolo", "NEW GAME"), NSLOCTEXT("GOTA", "NewSoloSub", "BEGIN THE PROLOGUE ALONE"));
	Bind(Btn_NewGameCoop, NSLOCTEXT("GOTA", "NewCoop", "NEW CO-OP CAMPAIGN"), NSLOCTEXT("GOTA", "NewCoopSub", "HOST ON LAN  •  UP TO 3 ALLIES"));
	Bind(Btn_JoinCoop, NSLOCTEXT("GOTA", "JoinCoop", "JOIN A CAMPAIGN"), NSLOCTEXT("GOTA", "JoinCoopSub", "FIND AN ALLY ON YOUR NETWORK"));
	Bind(Btn_Continue, NSLOCTEXT("GOTA", "StoryContinue", "CONTINUE"), FText::GetEmpty());
	Bind(Btn_Back, NSLOCTEXT("GOTA", "Back", "BACK"), FText::GetEmpty());

	if (Btn_NewGameSolo) { Btn_NewGameSolo->OnActivated.AddUniqueDynamic(this, &UGOTAStoryMenuWidget::HandleNewGameSolo); }
	if (Btn_NewGameCoop) { Btn_NewGameCoop->OnActivated.AddUniqueDynamic(this, &UGOTAStoryMenuWidget::HandleNewGameCoop); }
	if (Btn_JoinCoop) { Btn_JoinCoop->OnActivated.AddUniqueDynamic(this, &UGOTAStoryMenuWidget::HandleJoinCoop); }
	if (Btn_Continue) { Btn_Continue->OnActivated.AddUniqueDynamic(this, &UGOTAStoryMenuWidget::HandleContinue); }
	if (Btn_Back) { Btn_Back->OnActivated.AddUniqueDynamic(this, &UGOTAStoryMenuWidget::HandleBack); }

	if (const UGOTAMenuContentDefinition* Content = GetMenuContent())
	{
		if (Text_CampaignTitle) { Text_CampaignTitle->SetText(Content->CampaignTitle); }
		if (Text_Synopsis) { Text_Synopsis->SetText(Content->CampaignSynopsis); }
	}
}

void UGOTAStoryMenuWidget::OnScreenActivated()
{
	if (const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		if (Btn_Continue)
		{
			const bool bCanContinue = Subsystem->HasValidContinueSave();
			Btn_Continue->SetButtonEnabled(bCanContinue);
			Btn_Continue->SetSubtitle(bCanContinue
				? Subsystem->GetContinueSummary()
				: NSLOCTEXT("GOTA", "StoryNoSave", "No checkpoint saved yet"));
		}
	}
	RebuildChapters();
}

UGOTAMenuButtonWidget* UGOTAStoryMenuWidget::GetDefaultFocusButton() const
{
	if (Btn_Continue && Btn_Continue->IsButtonEnabled())
	{
		return Btn_Continue;
	}
	return Btn_NewGameSolo;
}

void UGOTAStoryMenuWidget::RebuildChapters()
{
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	if (!Box_Chapters || !EntryClass || !Content)
	{
		return;
	}
	Box_Chapters->ClearChildren();
	int32 Index = 0;
	for (const FGOTAStoryChapterInfo& Chapter : Content->Chapters)
	{
		UGOTAMenuButtonWidget* Entry = CreateWidget<UGOTAMenuButtonWidget>(this, EntryClass);
		if (!Entry)
		{
			continue;
		}
		Entry->Style = EGOTAButtonStyle::ListEntry;
		Entry->Payload = Index++;
		Entry->SetLabel(Chapter.Title);
		Entry->SetSubtitle(Chapter.bPlayable ? Chapter.Summary : NSLOCTEXT("GOTA", "ChapterLocked", "LOCKED — in development"));
		Entry->SetButtonEnabled(Chapter.bPlayable);
		if (Chapter.bPlayable)
		{
			Entry->OnActivated.AddUniqueDynamic(this, &UGOTAStoryMenuWidget::HandleChapterActivated);
			TrackFocus(Entry);
		}
		Box_Chapters->AddChild(Entry);
	}
}

void UGOTAStoryMenuWidget::HandleChapterActivated()
{
	// Chapter One is the only playable chapter; it launches the same validated prologue path.
	HandleNewGameSolo();
}

void UGOTAStoryMenuWidget::HandleNewGameSolo()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->PlayUISound(EGOTAMenuSound::Confirm);
		Subsystem->RequestStartStoryPrologue(false);
	}
}

void UGOTAStoryMenuWidget::HandleNewGameCoop()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->PlayUISound(EGOTAMenuSound::Confirm);
		Subsystem->RequestStartStoryPrologue(true);
	}
}

void UGOTAStoryMenuWidget::HandleJoinCoop()
{
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
		Subsystem->PlayUISound(EGOTAMenuSound::Back);
		Subsystem->GoBack(false);
	}
}
