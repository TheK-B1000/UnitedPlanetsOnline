// White Tiger Studios 2024

#include "UI/Widgets/GOTAHeroSelectWidget.h"

#include "Characters/GOTAHeroDefinition.h"
#include "Characters/GOTAHeroRegistry.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

namespace GOTAHeroUI
{
	static UGOTAHeroRegistry* Registry(const UUserWidget* W)
	{
		const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	}

	static FText Upper(const FText& T)
	{
		return T.ToUpper();
	}
}

UGOTAHeroSelectWidget::UGOTAHeroSelectWidget()
{
	ScreenId = EGOTAMenuScreen::HeroSelect;
}

void UGOTAHeroSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Btn_Confirm)
	{
		Btn_Confirm->SetLabel(NSLOCTEXT("GOTA", "HeroConfirm", "SELECT HERO"));
		Btn_Confirm->OnActivated.AddUniqueDynamic(this, &UGOTAHeroSelectWidget::HandleConfirm);
		TrackFocus(Btn_Confirm);
	}
	if (Btn_Back)
	{
		Btn_Back->SetLabel(NSLOCTEXT("GOTA", "Back", "BACK"));
		Btn_Back->OnActivated.AddUniqueDynamic(this, &UGOTAHeroSelectWidget::HandleBack);
		TrackFocus(Btn_Back);
	}
}

void UGOTAHeroSelectWidget::OnScreenActivated()
{
	if (Tiles.Num() == 0)
	{
		RebuildRoster();
	}
	const UGOTAHeroRegistry* Registry = GOTAHeroUI::Registry(this);
	FName Initial = Registry ? Registry->GetLocalSelection() : NAME_None;
	if (Initial.IsNone() && TileHeroIds.Num() > 0)
	{
		// Ayla leads the roster (approved card order); playable defaults are offered by CONFIRM state.
		Initial = TileHeroIds[0];
	}
	ShowHero(Initial);
}

UGOTAMenuButtonWidget* UGOTAHeroSelectWidget::GetDefaultFocusButton() const
{
	const int32 Index = TileHeroIds.IndexOfByKey(PreviewHero);
	return Tiles.IsValidIndex(Index) ? Tiles[Index].Get() : (Tiles.Num() > 0 ? Tiles[0].Get() : Btn_Back.Get());
}

void UGOTAHeroSelectWidget::RebuildRoster()
{
	const UGOTAHeroRegistry* Registry = GOTAHeroUI::Registry(this);
	if (!Registry || !Box_Roster || !TileClass)
	{
		return;
	}
	Box_Roster->ClearChildren();
	Tiles.Reset();
	TileHeroIds.Reset();
	for (const UGOTAHeroDefinition* Hero : Registry->GetHeroes())
	{
		UGOTAMenuButtonWidget* Tile = CreateWidget<UGOTAMenuButtonWidget>(this, TileClass);
		if (!Tile)
		{
			continue;
		}
		Tile->Style = EGOTAButtonStyle::FeatureCard;
		Tile->Payload = Tiles.Num();
		Tile->SetLabel(GOTAHeroUI::Upper(Hero->DisplayName));
		Tile->SetSubtitle(Hero->IsPlayable() ? NSLOCTEXT("GOTA", "HeroPlayable", "PLAYABLE") : NSLOCTEXT("GOTA", "HeroPreview", "PREVIEW"));
		// Thumbnails are small (≈300 px) — loading all of them is cheap; portraits stay soft.
		if (UTexture2D* Thumb = Hero->Thumbnail.LoadSynchronous())
		{
			Tile->SetArt(Thumb);
		}
		Tile->OnFocused.AddUniqueDynamic(this, &UGOTAHeroSelectWidget::HandleTileFocused);
		Tile->OnActivated.AddUniqueDynamic(this, &UGOTAHeroSelectWidget::HandleTileActivated);
		TrackFocus(Tile);
		Box_Roster->AddChild(Tile);
		Tiles.Add(Tile);
		TileHeroIds.Add(Hero->HeroId);
	}
}

void UGOTAHeroSelectWidget::SetStat(UProgressBar* Bar, UTextBlock* Value, int32 Stat)
{
	if (Bar) { Bar->SetPercent(FMath::Clamp(Stat / 10.f, 0.f, 1.f)); }
	if (Value) { Value->SetText(FText::AsNumber(Stat)); }
}

void UGOTAHeroSelectWidget::ShowHero(FName HeroId)
{
	UGOTAHeroRegistry* Registry = GOTAHeroUI::Registry(this);
	const UGOTAHeroDefinition* Hero = Registry ? Registry->FindHero(HeroId) : nullptr;
	if (!Hero)
	{
		return;
	}
	PreviewHero = HeroId;

	if (Image_Portrait)
	{
		if (UTexture2D* Portrait = Hero->Portrait.LoadSynchronous())
		{
			Image_Portrait->SetBrushFromTexture(Portrait, false);
		}
	}
	const FText Name = GOTAHeroUI::Upper(Hero->DisplayName);
	if (Text_HeroName) { Text_HeroName->SetText(Name); }
	if (Text_PanelName) { Text_PanelName->SetText(Name); }
	if (Text_HeroTitle) { Text_HeroTitle->SetText(Hero->Title); }
	if (Text_PanelTitle) { Text_PanelTitle->SetText(Hero->Title); }
	if (Text_Quote) { Text_Quote->SetText(Hero->Quote.IsEmpty() ? FText::GetEmpty() : FText::Format(NSLOCTEXT("GOTA", "QuoteFmt", "“{0}”"), Hero->Quote)); }
	if (Text_Faction)
	{
		const FText Faction = Hero->Faction.IsEmpty() ? NSLOCTEXT("GOTA", "FactionUndisclosed", "AFFILIATION UNDISCLOSED") : Hero->Faction;
		FString Lines = Faction.ToString();
		if (!Hero->ClassLabel.IsEmpty()) { Lines += TEXT("\n") + Hero->ClassLabel.ToString(); }
		if (!Hero->Specialization.IsEmpty()) { Lines += TEXT("\n") + Hero->Specialization.ToString(); }
		Text_Faction->SetText(FText::FromString(Lines));
	}
	SetStat(Bar_Attack, Text_Attack, Hero->CardStats.Attack);
	SetStat(Bar_Defense, Text_Defense, Hero->CardStats.Defense);
	SetStat(Bar_Mobility, Text_Mobility, Hero->CardStats.Mobility);
	SetStat(Bar_Technique, Text_Technique, Hero->CardStats.Technique);
	if (Text_Biography) { Text_Biography->SetText(Hero->Biography); }
	if (Text_Abilities)
	{
		FString Lines;
		for (const FGOTAHeroAbilityInfo& Ability : Hero->Abilities)
		{
			Lines += FString::Printf(TEXT("%s  —  %s\n"), *Ability.Name.ToString().ToUpper(), *Ability.Description.ToString());
		}
		Text_Abilities->SetText(FText::FromString(Lines.TrimEnd()));
	}
	if (Text_Weapon) { Text_Weapon->SetText(Hero->WeaponStyle); }

	FText Reason;
	const bool bValid = Registry->ValidateSelection(HeroId, EArenaMode::PvPDuel, Reason);
	if (Btn_Confirm) { Btn_Confirm->SetButtonEnabled(bValid); }
	if (Text_Availability)
	{
		Text_Availability->SetText(bValid ? NSLOCTEXT("GOTA", "HeroReady", "READY FOR THE ARENA") : Reason);
	}
	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		if (Tiles[i]) { Tiles[i]->SetSelected(TileHeroIds[i] == HeroId); }
	}
}

void UGOTAHeroSelectWidget::HandleTileFocused(UGOTAMenuButtonWidget* Tile)
{
	if (Tile && TileHeroIds.IsValidIndex(Tile->Payload))
	{
		ShowHero(TileHeroIds[Tile->Payload]);
	}
}

void UGOTAHeroSelectWidget::HandleTileActivated()
{
	// Activating a tile = choose it: jump to CONFIRM if the hero is valid.
	if (Btn_Confirm && Btn_Confirm->IsButtonEnabled())
	{
		Btn_Confirm->FocusButton();
	}
	else if (UGOTAMenuSubsystem* Menu = GetMenuSubsystem())
	{
		Menu->PlayUISound(EGOTAMenuSound::Error);
	}
}

void UGOTAHeroSelectWidget::HandleConfirm()
{
	UGOTAHeroRegistry* Registry = GOTAHeroUI::Registry(this);
	UGOTAMenuSubsystem* Menu = GetMenuSubsystem();
	FText Reason;
	if (!Registry || !Menu || !Registry->ValidateSelection(PreviewHero, EArenaMode::PvPDuel, Reason))
	{
		if (Menu) { Menu->ShowError(NSLOCTEXT("GOTA", "HeroInvalidTitle", "HERO UNAVAILABLE"), Reason); }
		return;
	}
	Registry->SetLocalSelection(PreviewHero);
	Menu->PlayUISound(EGOTAMenuSound::Confirm);
	Menu->LaunchPendingArena();
}

void UGOTAHeroSelectWidget::HandleBack()
{
	if (UGOTAMenuSubsystem* Menu = GetMenuSubsystem())
	{
		Menu->PlayUISound(EGOTAMenuSound::Back);
		Menu->GoBack(false);
	}
}
