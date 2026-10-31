// White Tiger Studios 2024

#include "UI/Widgets/GOTAExtrasWidget.h"

#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "UI/GOTAMenuContentDefinition.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTAExtrasWidget::UGOTAExtrasWidget()
{
	ScreenId = EGOTAMenuScreen::Extras;
}

void UGOTAExtrasWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Btn_Back)
	{
		Btn_Back->SetLabel(NSLOCTEXT("GOTA", "Back", "BACK"));
		Btn_Back->OnActivated.AddUniqueDynamic(this, &UGOTAExtrasWidget::HandleBack);
		TrackFocus(Btn_Back);
	}
}

void UGOTAExtrasWidget::OnScreenActivated()
{
	if (Entries.Num() == 0)
	{
		RebuildEntries();
		ShowEntry(0);
	}
}

UGOTAMenuButtonWidget* UGOTAExtrasWidget::GetDefaultFocusButton() const
{
	return Entries.Num() > 0 ? Entries[0].Get() : Btn_Back.Get();
}

void UGOTAExtrasWidget::RebuildEntries()
{
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	if (!Box_Entries || !EntryClass || !Content)
	{
		return;
	}
	Box_Entries->ClearChildren();
	Entries.Reset();
	for (int32 Index = 0; Index < Content->LoreEntries.Num(); ++Index)
	{
		const FGOTALoreEntry& Lore = Content->LoreEntries[Index];
		UGOTAMenuButtonWidget* Entry = CreateWidget<UGOTAMenuButtonWidget>(this, EntryClass);
		if (!Entry)
		{
			continue;
		}
		Entry->Style = EGOTAButtonStyle::ListEntry;
		Entry->Payload = Index;
		Entry->SetLabel(Lore.Title);
		Entry->SetSubtitle(Lore.Category);
		Entry->OnFocused.AddUniqueDynamic(this, &UGOTAExtrasWidget::HandleEntryFocused);
		TrackFocus(Entry);
		Box_Entries->AddChild(Entry);
		Entries.Add(Entry);
	}
}

void UGOTAExtrasWidget::HandleEntryFocused(UGOTAMenuButtonWidget* Button)
{
	if (Button)
	{
		ShowEntry(Button->Payload);
	}
}

void UGOTAExtrasWidget::ShowEntry(int32 Index)
{
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	if (!Content || !Content->LoreEntries.IsValidIndex(Index))
	{
		return;
	}
	const FGOTALoreEntry& Lore = Content->LoreEntries[Index];
	if (Text_EntryCategory) { Text_EntryCategory->SetText(Lore.Category); }
	if (Text_EntryTitle) { Text_EntryTitle->SetText(Lore.Title); }
	if (Text_EntryBody) { Text_EntryBody->SetText(Lore.Body); }
	if (Scroll_Body) { Scroll_Body->ScrollToStart(); }
	for (UGOTAMenuButtonWidget* Entry : Entries)
	{
		if (Entry)
		{
			Entry->SetSelected(Entry->Payload == Index);
		}
	}
}

void UGOTAExtrasWidget::HandleBack()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->PlayUISound(EGOTAMenuSound::Back);
		Subsystem->GoBack(false);
	}
}
