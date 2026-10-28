// White Tiger Studios 2024

#include "UI/Widgets/GOTAHordeHUDWidget.h"

#include "Arenas/Horde/HordeGameState.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UGOTAHordeHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->OnHordePhaseChanged.AddDynamic(this, &UGOTAHordeHUDWidget::HandlePhaseChanged);
		GS->OnHordeWaveStatusChanged.AddDynamic(this, &UGOTAHordeHUDWidget::HandleWaveStatus);
		GS->OnHordeScoreChanged.AddDynamic(this, &UGOTAHordeHUDWidget::HandleScoreChanged);
	}
	RefreshFromGameState();
}

void UGOTAHordeHUDWidget::NativeDestruct()
{
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->OnHordePhaseChanged.RemoveAll(this);
		GS->OnHordeWaveStatusChanged.RemoveAll(this);
		GS->OnHordeScoreChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UGOTAHordeHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (AHordeGameState* GS = GetHordeGS())
	{
		if (Text_Countdown)
		{
			const float Remain = GS->GetSecondsUntilPhaseDeadline();
			if (Remain > 0.f && (GS->HordePhase == EHordeMatchPhase::Preparing || GS->HordePhase == EHordeMatchPhase::Intermission))
			{
				Text_Countdown->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), Remain)));
				Text_Countdown->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				Text_Countdown->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		if (Bar_BossHealth)
		{
			Bar_BossHealth->SetVisibility(GS->bBossEncounterActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			Bar_BossHealth->SetPercent(GS->BossHealthFraction);
		}
	}
}

void UGOTAHordeHUDWidget::HandlePhaseChanged(EHordeMatchPhase NewPhase)
{
	RefreshFromGameState();
}

void UGOTAHordeHUDWidget::HandleWaveStatus(int32 WaveIndex, int32 TotalWaves, int32 EnemiesRemaining)
{
	if (Text_Wave)
	{
		Text_Wave->SetText(FText::FromString(FString::Printf(TEXT("WAVE %d / %d"), WaveIndex, TotalWaves)));
	}
	if (Text_Enemies)
	{
		Text_Enemies->SetText(FText::FromString(FString::Printf(TEXT("ENEMIES %d"), EnemiesRemaining)));
	}
}

void UGOTAHordeHUDWidget::HandleScoreChanged(int32 TeamScore)
{
	if (Text_Score)
	{
		Text_Score->SetText(FText::FromString(FString::Printf(TEXT("SCORE %d"), TeamScore)));
	}
}

void UGOTAHordeHUDWidget::RefreshFromGameState()
{
	AHordeGameState* GS = GetHordeGS();
	if (!GS)
	{
		return;
	}

	HandleWaveStatus(GS->CurrentWaveIndex, GS->TotalRegularWaves, GS->EnemiesAlive + GS->EnemiesRemainingToSpawn);
	HandleScoreChanged(GS->TeamScore);

	if (Text_Phase)
	{
		const UEnum* Enum = StaticEnum<EHordeMatchPhase>();
		Text_Phase->SetText(Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(GS->HordePhase)) : FText::GetEmpty());
	}
}

AHordeGameState* UGOTAHordeHUDWidget::GetHordeGS() const
{
	return GetWorld() ? GetWorld()->GetGameState<AHordeGameState>() : nullptr;
}
