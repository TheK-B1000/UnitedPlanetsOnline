// White Tiger Studios 2024

#include "UI/Widgets/GOTADuelResultsWidget.h"

#include "Arenas/Duel/DuelGameMode.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Characters/GOTAHeroDefinition.h"
#include "Characters/GOTAHeroRegistry.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

void UGOTADuelResultsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Btn_Rematch)
	{
		Btn_Rematch->SetLabel(NSLOCTEXT("GOTA", "Rematch", "REMATCH"));
		Btn_Rematch->OnActivated.AddUniqueDynamic(this, &UGOTADuelResultsWidget::HandleRematch);
		Btn_Rematch->SetButtonEnabled(true);
	}
	if (Btn_ReturnToLobby)
	{
		Btn_ReturnToLobby->SetLabel(NSLOCTEXT("GOTA", "ReturnMenu", "RETURN TO MAIN MENU"));
		Btn_ReturnToLobby->OnActivated.AddUniqueDynamic(this, &UGOTADuelResultsWidget::HandleReturnToLobby);
		Btn_ReturnToLobby->SetButtonEnabled(true);
	}
	Refresh();
	FocusDefault();
}

void UGOTADuelResultsWidget::FocusDefault()
{
	if (Btn_Rematch && Btn_Rematch->IsButtonEnabled())
	{
		Btn_Rematch->FocusButton();
	}
	else if (Btn_ReturnToLobby)
	{
		Btn_ReturnToLobby->FocusButton();
	}
}

void UGOTADuelResultsWidget::Refresh()
{
	ADuelGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr;
	if (!GS)
	{
		return;
	}

	const APlayerController* PC = GetOwningPlayer();
	const ADuelPlayerState* LocalPS = PC ? PC->GetPlayerState<ADuelPlayerState>() : nullptr;
	const int32 LocalSlot = LocalPS && LocalPS->DuelSlot != INDEX_NONE ? LocalPS->DuelSlot : 0;
	const int32 LocalScore = LocalSlot == 0 ? GS->ScorePlayer0 : GS->ScorePlayer1;
	const int32 OpponentScore = LocalSlot == 0 ? GS->ScorePlayer1 : GS->ScorePlayer0;

	if (Text_Outcome)
	{
		FText OutcomeText = NSLOCTEXT("GOTA", "Draw", "DRAW");
		const bool bLocalWon = (GS->MatchOutcome == EDuelMatchOutcome::Player0Wins && LocalSlot == 0)
			|| (GS->MatchOutcome == EDuelMatchOutcome::Player1Wins && LocalSlot == 1);
		const bool bLocalLost = (GS->MatchOutcome == EDuelMatchOutcome::Player0Wins && LocalSlot == 1)
			|| (GS->MatchOutcome == EDuelMatchOutcome::Player1Wins && LocalSlot == 0);
		if (bLocalWon)
		{
			OutcomeText = NSLOCTEXT("GOTA", "Victory", "VICTORY");
		}
		else if (bLocalLost)
		{
			OutcomeText = NSLOCTEXT("GOTA", "Defeat", "DEFEAT");
		}
		else if (GS->MatchOutcome == EDuelMatchOutcome::DrawByTimeout)
		{
			OutcomeText = NSLOCTEXT("GOTA", "TimeoutDraw", "DRAW — TIME");
		}
		Text_Outcome->SetText(OutcomeText);
	}

	// The local fighter's accepted hero (replicated id -> local definition).
	const UGOTAHeroRegistry* Heroes = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	const UGOTAHeroDefinition* Hero = (LocalPS && Heroes) ? Heroes->FindHero(LocalPS->HeroId) : nullptr;
	if (Image_Hero)
	{
		UTexture2D* Portrait = Hero ? Hero->Portrait.LoadSynchronous() : nullptr;
		Image_Hero->SetVisibility(Portrait ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Portrait)
		{
			Image_Hero->SetBrushFromTexture(Portrait, false);
		}
	}
	if (Text_HeroName)
	{
		Text_HeroName->SetText(Hero ? FText::Format(NSLOCTEXT("GOTA", "ResultHeroFmt", "{0}  \u2022  {1}"), Hero->DisplayName.ToUpper(), Hero->Title) : FText::GetEmpty());
	}

	if (Text_Summary)
	{
		Text_Summary->SetText(FText::Format(
			NSLOCTEXT("GOTA", "DuelSummaryFmt", "TRIAL OF CHAMPIONS  •  BEST OF {0}  •  FINAL {1} – {2}  •  ROUNDS PLAYED {3}"),
			FText::AsNumber(GS->BestOfRounds), FText::AsNumber(LocalScore), FText::AsNumber(OpponentScore),
			FText::AsNumber(GS->CurrentRound)));
	}
}

void UGOTADuelResultsWidget::HandleRematch()
{
	// Lock both choices until the server acts — no double travel / double restart.
	if (Btn_Rematch) { Btn_Rematch->SetButtonEnabled(false); }
	if (Btn_ReturnToLobby) { Btn_ReturnToLobby->SetButtonEnabled(false); }

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ADuelPlayerState* PS = PC->GetPlayerState<ADuelPlayerState>())
		{
			PS->ServerRequestRematch();
			return;
		}
	}
	if (ADuelGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADuelGameMode>() : nullptr)
	{
		GM->RequestRematch();
	}
}

void UGOTADuelResultsWidget::HandleReturnToLobby()
{
	if (Btn_Rematch) { Btn_Rematch->SetButtonEnabled(false); }
	if (Btn_ReturnToLobby) { Btn_ReturnToLobby->SetButtonEnabled(false); }

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ADuelPlayerState* PS = PC->GetPlayerState<ADuelPlayerState>())
		{
			PS->ServerRequestReturnToLobby();
			return;
		}
	}
	if (ADuelGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADuelGameMode>() : nullptr)
	{
		GM->RequestReturnToLobby();
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/MainMenu/L_GOTA_MainMenu")), true);
}
