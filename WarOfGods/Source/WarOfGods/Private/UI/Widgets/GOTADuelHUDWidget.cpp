// White Tiger Studios 2024

#include "UI/Widgets/GOTADuelHUDWidget.h"

#include "Arenas/Duel/DuelGameState.h"
#include "Characters/StatsComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void UGOTADuelHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->OnDuelPhaseChanged.AddDynamic(this, &UGOTADuelHUDWidget::HandlePhaseChanged);
		GS->OnDuelScoreChanged.AddDynamic(this, &UGOTADuelHUDWidget::HandleScoreChanged);
	}
	RefreshFromGameState();
}

void UGOTADuelHUDWidget::NativeDestruct()
{
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->OnDuelPhaseChanged.RemoveDynamic(this, &UGOTADuelHUDWidget::HandlePhaseChanged);
		GS->OnDuelScoreChanged.RemoveDynamic(this, &UGOTADuelHUDWidget::HandleScoreChanged);
	}
	Super::NativeDestruct();
}

void UGOTADuelHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshFromGameState();
}

void UGOTADuelHUDWidget::HandlePhaseChanged(EDuelPhase NewPhase)
{
	RefreshFromGameState();
}

void UGOTADuelHUDWidget::HandleScoreChanged(int32 Score0, int32 Score1, int32 RoundIndex)
{
	RefreshFromGameState();
}

void UGOTADuelHUDWidget::RefreshFromGameState()
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS)
	{
		return;
	}

	if (Text_Round)
	{
		Text_Round->SetText(FText::FromString(FString::Printf(
			TEXT("ROUND %d  (first to %d)"), GS->CurrentRound, GS->RoundsToWin)));
	}
	if (Text_Score)
	{
		Text_Score->SetText(FText::FromString(FString::Printf(
			TEXT("%d  —  %d"), GS->ScorePlayer0, GS->ScorePlayer1)));
	}
	if (Text_Phase)
	{
		const UEnum* Enum = StaticEnum<EDuelPhase>();
		Text_Phase->SetText(Enum
			? Enum->GetDisplayNameTextByValue(static_cast<int64>(GS->DuelPhase))
			: FText::GetEmpty());
	}
	if (Text_Countdown)
	{
		const bool bShowCountdown = GS->DuelPhase == EDuelPhase::Countdown
			|| GS->DuelPhase == EDuelPhase::Intermission
			|| GS->DuelPhase == EDuelPhase::PreparingRound;
		Text_Countdown->SetVisibility(bShowCountdown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowCountdown)
		{
			Text_Countdown->SetText(FText::AsNumber(FMath::CeilToInt(GS->GetSecondsUntilPhaseDeadline())));
		}
	}
	if (Text_Timer)
	{
		const bool bShowTimer = GS->DuelPhase == EDuelPhase::InRound;
		Text_Timer->SetVisibility(bShowTimer ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowTimer)
		{
			Text_Timer->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), GS->GetSecondsUntilRoundEnd())));
		}
	}

	APlayerController* PC = GetOwningPlayer();
	APawn* LocalPawn = PC ? PC->GetPawn() : nullptr;
	APawn* OpponentPawn = nullptr;
	if (GS)
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (!PS || PS == (PC ? PC->PlayerState : nullptr))
			{
				continue;
			}
			OpponentPawn = PS->GetPawn();
			break;
		}
	}

	if (Bar_LocalHealth && LocalPawn)
	{
		if (UStatsComponent* Stats = LocalPawn->FindComponentByClass<UStatsComponent>())
		{
			const float MaxH = FMath::Max(1.f, Stats->GetMaxHealth());
			Bar_LocalHealth->SetPercent(Stats->GetHealth() / MaxH);
		}
	}
	if (Bar_OpponentHealth && OpponentPawn)
	{
		if (UStatsComponent* Stats = OpponentPawn->FindComponentByClass<UStatsComponent>())
		{
			const float MaxH = FMath::Max(1.f, Stats->GetMaxHealth());
			Bar_OpponentHealth->SetPercent(Stats->GetHealth() / MaxH);
		}
	}
}

ADuelGameState* UGOTADuelHUDWidget::GetDuelGS() const
{
	return GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr;
}
