// White Tiger Studios 2024

#include "UI/Widgets/GOTAArenaSelectionWidget.h"
#include "UI/GOTALobbyViewModel.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UGOTAArenaSelectionWidget::UGOTAArenaSelectionWidget()
{
	ScreenId = EGOTAMenuScreen::ArenaSelection;
}

void UGOTAArenaSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Btn_Confirm)
	{
		Btn_Confirm->SetLabel(NSLOCTEXT("GOTA", "EnterArena", "ENTER ARENA"));
		Btn_Confirm->OnActivated.AddDynamic(this, &UGOTAArenaSelectionWidget::HandleConfirm);
	}
	if (Btn_Back)
	{
		Btn_Back->SetLabel(NSLOCTEXT("GOTA", "Back", "BACK"));
		Btn_Back->OnActivated.AddDynamic(this, &UGOTAArenaSelectionWidget::HandleBack);
	}
}

void UGOTAArenaSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshArenaCards();
}

void UGOTAArenaSelectionWidget::RefreshArenaCards()
{
	ArenaCards.Reset();
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		ArenaCards = Subsystem->BuildArenaCards();
	}
	OnArenaCardsUpdated(ArenaCards);
	if (ArenaCards.Num() > 0)
	{
		SelectArena(ArenaCards[0].ArenaId);
	}
}

void UGOTAArenaSelectionWidget::SelectArena(EArenaId ArenaId)
{
	HighlightedArena = ArenaId;
	bool bCanConfirm = false;
	for (const FGOTAArenaCard& Card : ArenaCards)
	{
		if (Card.ArenaId == ArenaId)
		{
			bCanConfirm = Card.bIsSelectable && !Card.bComingSoon;
			break;
		}
	}
	if (Btn_Confirm)
	{
		Btn_Confirm->SetButtonEnabled(bCanConfirm);
	}

	OnArenaHighlighted(ArenaId);

	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		if (UGOTALobbyViewModel* Lobby = Subsystem->GetLobbyViewModel())
		{
			Lobby->SetSelectedArena(ArenaId);
		}
	}
}

void UGOTAArenaSelectionWidget::ConfirmSelection()
{
	FGOTAArenaCard Selected;
	bool bFound = false;
	for (const FGOTAArenaCard& Card : ArenaCards)
	{
		if (Card.ArenaId == HighlightedArena)
		{
			Selected = Card;
			bFound = true;
			break;
		}
	}

	if (!bFound || !Selected.bIsSelectable || Selected.bComingSoon)
	{
		return;
	}

	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		if (UGOTALobbyViewModel* Lobby = Subsystem->GetLobbyViewModel())
		{
			Lobby->SetSelectedArena(Selected.ArenaId);
		}
	}

	// Multiplayer lobby path: store selection and return to Champion Hall.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			if (Sessions->IsInSession())
			{
				Sessions->SetSelectedArena(Selected.ArenaId);
				if (Selected.ArenaId == EArenaId::TitansQuarry)
				{
					Sessions->SetSelectedMode(EArenaMode::PvPDuel);
				}
				if (UGOTAMenuSubsystem* Menu = GetMenuSubsystem())
				{
					Menu->NavigateTo(EGOTAMenuScreen::Lobby, false);
				}
				return;
			}
		}
	}

	if (Selected.ArenaMap.IsValid())
	{
		// Solo PvE path: Titan's Quarry launches Horde GameMode on the graybox map.
		const FString Options = TEXT("game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry?listen");
		const FString LongPackage = Selected.ArenaMap.GetLongPackageName();
		UGameplayStatics::OpenLevel(this, FName(*LongPackage), true, Options);
		return;
	}

	RequestTravelToSelectedArena();
}

void UGOTAArenaSelectionWidget::HandleConfirm() { ConfirmSelection(); }

void UGOTAArenaSelectionWidget::HandleBack()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->GoBack(false);
	}
}
