// White Tiger Studios 2024

#include "UI/Widgets/GOTAHordeHUDWidget.h"

#include "Arenas/Horde/HordeGameState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

void UGOTAHordeHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureRuntimeLayout();
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

void UGOTAHordeHUDWidget::EnsureRuntimeLayout()
{
	if (Text_Wave && Text_Phase && Text_Enemies && Text_Score)
	{
		return;
	}
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HordeHudRoot"));
		WidgetTree->RootWidget = Root;
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HordeHudBox"));
	if (UCanvasPanelSlot* RootSlot = Root->AddChildToCanvas(Box))
	{
		RootSlot->SetAutoSize(true);
		RootSlot->SetAnchors(FAnchors(0.f, 0.f));
		RootSlot->SetPosition(FVector2D(48.f, 48.f));
	}

	auto MakeLine = [this, Box](const TCHAR* Name) -> UTextBlock*
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 22;
		Text->SetFont(Font);
		if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Text))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
		return Text;
	};

	if (!Text_Wave) { Text_Wave = MakeLine(TEXT("Text_Wave")); }
	if (!Text_Phase) { Text_Phase = MakeLine(TEXT("Text_Phase")); }
	if (!Text_Enemies) { Text_Enemies = MakeLine(TEXT("Text_Enemies")); }
	if (!Text_Score) { Text_Score = MakeLine(TEXT("Text_Score")); }
	if (!Text_Countdown) { Text_Countdown = MakeLine(TEXT("Text_Countdown")); }

	if (!Bar_BossHealth)
	{
		Bar_BossHealth = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Bar_BossHealth"));
		Bar_BossHealth->SetPercent(1.f);
		Bar_BossHealth->SetFillColorAndOpacity(FLinearColor(0.85f, 0.15f, 0.1f));
		if (UVerticalBoxSlot* BarSlot = Box->AddChildToVerticalBox(Bar_BossHealth))
		{
			BarSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
			BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		Bar_BossHealth->SetVisibility(ESlateVisibility::Collapsed);
	}
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
