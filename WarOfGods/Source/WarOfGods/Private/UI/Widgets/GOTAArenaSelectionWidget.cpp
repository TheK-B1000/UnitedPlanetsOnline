// White Tiger Studios 2024

#include "UI/Widgets/GOTAArenaSelectionWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTAArenaSelectionWidget::UGOTAArenaSelectionWidget()
{
	ScreenId = EGOTAMenuScreen::ArenaSelection;
}

void UGOTAArenaSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Btn_ModeFightAI)
	{
		Btn_ModeFightAI->Style = EGOTAButtonStyle::Tab;
		Btn_ModeFightAI->OnActivated.AddUniqueDynamic(this, &UGOTAArenaSelectionWidget::HandleFightAI);
		TrackFocus(Btn_ModeFightAI);
	}
	if (Btn_ModeHorde)
	{
		Btn_ModeHorde->Style = EGOTAButtonStyle::Tab;
		Btn_ModeHorde->OnActivated.AddUniqueDynamic(this, &UGOTAArenaSelectionWidget::HandleHorde);
		TrackFocus(Btn_ModeHorde);
	}
	if (Btn_Confirm)
	{
		Btn_Confirm->OnActivated.AddUniqueDynamic(this, &UGOTAArenaSelectionWidget::HandleConfirm);
		TrackFocus(Btn_Confirm);
	}
	if (Btn_Back)
	{
		Btn_Back->OnActivated.AddUniqueDynamic(this, &UGOTAArenaSelectionWidget::HandleBack);
		TrackFocus(Btn_Back);
	}
}

void UGOTAArenaSelectionWidget::OnScreenActivated()
{
	if (ArenaEntries.Num() == 0)
	{
		RebuildArenaList();
	}
	RefreshDetails();
}

UGOTAMenuButtonWidget* UGOTAArenaSelectionWidget::GetDefaultFocusButton() const
{
	if (Btn_Confirm && Btn_Confirm->IsButtonEnabled())
	{
		return Btn_Confirm;
	}
	return Btn_ModeFightAI;
}

void UGOTAArenaSelectionWidget::RebuildArenaList()
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	if (!Subsystem || !Box_Arenas || !EntryClass)
	{
		return;
	}
	Box_Arenas->ClearChildren();
	ArenaEntries.Reset();
	Cards = Subsystem->BuildArenaCards();

	for (const FGOTAArenaCard& Card : Cards)
	{
		UGOTAMenuButtonWidget* Entry = CreateWidget<UGOTAMenuButtonWidget>(this, EntryClass);
		if (!Entry)
		{
			continue;
		}
		Entry->Style = EGOTAButtonStyle::ListEntry;
		Entry->Payload = static_cast<int32>(Card.ArenaId);
		Entry->SetLabel(Card.DisplayName);
		Entry->SetSubtitle(Card.Tagline);
		Entry->OnFocused.AddUniqueDynamic(this, &UGOTAArenaSelectionWidget::HandleArenaFocused);
		TrackFocus(Entry);
		Box_Arenas->AddChild(Entry);
		ArenaEntries.Add(Entry);
	}
}

void UGOTAArenaSelectionWidget::SelectRuleset(EGOTAArenaRuleset Ruleset)
{
	SelectedRuleset = Ruleset;
	RefreshDetails();
}

void UGOTAArenaSelectionWidget::SelectArena(EArenaId ArenaId)
{
	SelectedArena = ArenaId;
	RefreshDetails();
}

void UGOTAArenaSelectionWidget::HandleFightAI()
{
	SelectRuleset(EGOTAArenaRuleset::FightAI);
}

void UGOTAArenaSelectionWidget::HandleHorde()
{
	SelectRuleset(EGOTAArenaRuleset::Horde);
}

void UGOTAArenaSelectionWidget::HandleArenaFocused(UGOTAMenuButtonWidget* Button)
{
	// Focus previews (gamepad friendly); the arena under focus is the one Confirm launches.
	if (Button && Button->Payload != INDEX_NONE)
	{
		SelectArena(static_cast<EArenaId>(Button->Payload));
	}
}

void UGOTAArenaSelectionWidget::RefreshDetails()
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();

	if (Btn_ModeFightAI) { Btn_ModeFightAI->SetSelected(SelectedRuleset == EGOTAArenaRuleset::FightAI); }
	if (Btn_ModeHorde) { Btn_ModeHorde->SetSelected(SelectedRuleset == EGOTAArenaRuleset::Horde); }
	for (UGOTAMenuButtonWidget* Entry : ArenaEntries)
	{
		if (Entry)
		{
			Entry->SetSelected(Entry->Payload == static_cast<int32>(SelectedArena));
		}
	}

	if (Text_ModeTitle)
	{
		Text_ModeTitle->SetText(SelectedRuleset == EGOTAArenaRuleset::FightAI
			? NSLOCTEXT("GOTA", "ModeTrialTitle", "TRIAL OF CHAMPIONS — FIGHT AI")
			: NSLOCTEXT("GOTA", "ModeHordeTitle", "WRATH OF THE GODS — HORDE"));
	}
	if (Text_ModeDescription)
	{
		Text_ModeDescription->SetText(SelectedRuleset == EGOTAArenaRuleset::FightAI
			? NSLOCTEXT("GOTA", "ModeTrialDesc", "Single combat against an Arena Champion. Best of three rounds — first to two victories claims the trial.")
			: NSLOCTEXT("GOTA", "ModeHordeDesc", "Survive five escalating waves and the champion that follows. Up to four Wardens."));
	}

	for (const FGOTAArenaCard& Card : Cards)
	{
		if (Card.ArenaId != SelectedArena)
		{
			continue;
		}
		if (Text_ArenaName) { Text_ArenaName->SetText(Card.DisplayName); }
		if (Text_ArenaDescription) { Text_ArenaDescription->SetText(Card.Description); }
		if (Image_ArenaPreview)
		{
			UTexture2D* Preview = Card.Thumbnail.LoadSynchronous();
			Image_ArenaPreview->SetVisibility(Preview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
			if (Preview)
			{
				Image_ArenaPreview->SetBrushFromTexture(Preview, false);
			}
		}
	}

	FText Reason;
	const bool bAvailable = Subsystem && Subsystem->IsArenaRulesetAvailable(SelectedRuleset, SelectedArena, Reason);
	if (Btn_Confirm)
	{
		Btn_Confirm->SetButtonEnabled(bAvailable);
	}
	if (Text_Availability)
	{
		Text_Availability->SetText(bAvailable
			? NSLOCTEXT("GOTA", "ArenaReady", "READY FOR DEPLOYMENT")
			: Reason);
	}
}

void UGOTAArenaSelectionWidget::HandleConfirm()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->PlayUISound(EGOTAMenuSound::Confirm);
		if (SelectedRuleset == EGOTAArenaRuleset::FightAI)
		{
			// Trial of Champions picks a hero first (validated again by the server on login).
			Subsystem->BeginArenaHeroSelection(SelectedRuleset, SelectedArena);
		}
		else
		{
			Subsystem->RequestLaunchArena(SelectedRuleset, SelectedArena);
		}
	}
}

void UGOTAArenaSelectionWidget::HandleBack()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->PlayUISound(EGOTAMenuSound::Back);
		Subsystem->GoBack(false);
	}
}
