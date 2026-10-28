// White Tiger Studios 2024

#include "UI/Widgets/GOTAHordeResultsWidget.h"

#include "Arenas/Horde/HordeGameMode.h"
#include "Arenas/Horde/HordeGameState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

void UGOTAHordeResultsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureRuntimeLayout();

	if (Btn_Replay)
	{
		Btn_Replay->SetLabel(NSLOCTEXT("GOTA", "Replay", "FIGHT AGAIN"));
		Btn_Replay->OnActivated.AddDynamic(this, &UGOTAHordeResultsWidget::HandleReplay);
	}
	else if (Btn_ReplayNative)
	{
		Btn_ReplayNative->OnClicked.AddDynamic(this, &UGOTAHordeResultsWidget::HandleReplay);
	}

	if (Btn_ReturnToLobby)
	{
		Btn_ReturnToLobby->SetLabel(NSLOCTEXT("GOTA", "ReturnLobby", "RETURN TO LOBBY"));
		Btn_ReturnToLobby->OnActivated.AddDynamic(this, &UGOTAHordeResultsWidget::HandleReturnToLobby);
	}
	else if (Btn_ReturnNative)
	{
		Btn_ReturnNative->OnClicked.AddDynamic(this, &UGOTAHordeResultsWidget::HandleReturnToLobby);
	}

	Refresh();
}

void UGOTAHordeResultsWidget::EnsureRuntimeLayout()
{
	if (Text_Outcome && Text_Summary && (Btn_Replay || Btn_ReplayNative))
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
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HordeResultsRoot"));
		WidgetTree->RootWidget = Root;
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HordeResultsBox"));
	if (UCanvasPanelSlot* RootSlot = Root->AddChildToCanvas(Box))
	{
		RootSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		RootSlot->SetAutoSize(true);
	}

	auto MakeText = [this, Box](const TCHAR* Name, int32 Size) -> UTextBlock*
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Text))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
			Slot->SetHorizontalAlignment(HAlign_Center);
		}
		return Text;
	};

	if (!Text_Outcome) { Text_Outcome = MakeText(TEXT("Text_Outcome"), 42); }
	if (!Text_Summary) { Text_Summary = MakeText(TEXT("Text_Summary"), 20); }

	auto MakeButton = [this, Box](const TCHAR* Name, const FText& Label) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("%s_Label"), Name));
		LabelText->SetText(Label);
		Button->AddChild(LabelText);
		if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Button))
		{
			Slot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
			Slot->SetHorizontalAlignment(HAlign_Center);
		}
		return Button;
	};

	if (!Btn_Replay && !Btn_ReplayNative)
	{
		Btn_ReplayNative = MakeButton(TEXT("Btn_ReplayNative"), NSLOCTEXT("GOTA", "Replay", "FIGHT AGAIN"));
	}
	if (!Btn_ReturnToLobby && !Btn_ReturnNative)
	{
		Btn_ReturnNative = MakeButton(TEXT("Btn_ReturnNative"), NSLOCTEXT("GOTA", "ReturnLobby", "RETURN TO LOBBY"));
	}
}

void UGOTAHordeResultsWidget::Refresh()
{
	AHordeGameState* GS = GetWorld() ? GetWorld()->GetGameState<AHordeGameState>() : nullptr;
	if (!GS)
	{
		return;
	}

	if (Text_Outcome)
	{
		Text_Outcome->SetText(GS->MatchOutcome == EHordeMatchOutcome::Victory
			? NSLOCTEXT("GOTA", "Victory", "VICTORY")
			: NSLOCTEXT("GOTA", "Defeat", "DEFEAT"));
	}
	if (Text_Summary)
	{
		const float Elapsed = FMath::Max(0.f, GS->GetServerWorldTimeSeconds() - GS->MatchStartServerTime);
		Text_Summary->SetText(FText::FromString(FString::Printf(
			TEXT("Waves %d/%d  |  Score %d  |  Defeated %d  |  Time %.0fs"),
			GS->CurrentWaveIndex,
			GS->TotalRegularWaves,
			GS->TeamScore,
			GS->EnemiesDefeated,
			Elapsed)));
	}
}

void UGOTAHordeResultsWidget::HandleReplay()
{
	if (AHordeGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AHordeGameMode>() : nullptr)
	{
		GM->RequestRestartMatch();
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName()), true,
		TEXT("game=/Script/WarOfGods.HordeGameMode?Mode=Horde?listen"));
}

void UGOTAHordeResultsWidget::HandleReturnToLobby()
{
	if (AHordeGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AHordeGameMode>() : nullptr)
	{
		GM->RequestReturnToLobby();
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/MainMenu/L_GOTA_MainMenu")), true);
}
