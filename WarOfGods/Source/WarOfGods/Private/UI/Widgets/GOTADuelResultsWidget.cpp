// White Tiger Studios 2024

#include "UI/Widgets/GOTADuelResultsWidget.h"

#include "Arenas/Duel/DuelGameMode.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

void UGOTADuelResultsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Btn_Rematch)
	{
		Btn_Rematch->SetLabel(NSLOCTEXT("GOTA", "Rematch", "REMATCH"));
		Btn_Rematch->OnActivated.AddDynamic(this, &UGOTADuelResultsWidget::HandleRematch);
	}
	if (Btn_ReturnToLobby)
	{
		Btn_ReturnToLobby->SetLabel(NSLOCTEXT("GOTA", "ReturnLobby", "RETURN TO LOBBY"));
		Btn_ReturnToLobby->OnActivated.AddDynamic(this, &UGOTADuelResultsWidget::HandleReturnToLobby);
	}
	Refresh();
}

void UGOTADuelResultsWidget::Refresh()
{
	ADuelGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr;
	if (!GS)
	{
		return;
	}

	if (Text_Outcome)
	{
		FText OutcomeText = NSLOCTEXT("GOTA", "Draw", "DRAW");
		switch (GS->MatchOutcome)
		{
		case EDuelMatchOutcome::Player0Wins:
			OutcomeText = NSLOCTEXT("GOTA", "P0Wins", "CHAMPION — FIGHTER I");
			break;
		case EDuelMatchOutcome::Player1Wins:
			OutcomeText = NSLOCTEXT("GOTA", "P1Wins", "CHAMPION — FIGHTER II");
			break;
		case EDuelMatchOutcome::DrawByTimeout:
			OutcomeText = NSLOCTEXT("GOTA", "TimeoutDraw", "DRAW — TIME");
			break;
		default:
			break;
		}
		Text_Outcome->SetText(OutcomeText);
	}

	if (Text_Summary)
	{
		Text_Summary->SetText(FText::FromString(FString::Printf(
			TEXT("Trial of Champions  |  Best of %d  |  Final %d — %d  |  Rounds played %d"),
			GS->BestOfRounds,
			GS->ScorePlayer0,
			GS->ScorePlayer1,
			GS->CurrentRound)));
	}
}

void UGOTADuelResultsWidget::HandleRematch()
{
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
