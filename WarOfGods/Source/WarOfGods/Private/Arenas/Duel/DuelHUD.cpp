// White Tiger Studios 2024

#include "Arenas/Duel/DuelHUD.h"

#include "Arenas/Duel/DuelGameState.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/Widgets/GOTADuelHUDWidget.h"
#include "UI/Widgets/GOTADuelResultsWidget.h"

void ADuelHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!HudWidgetClass.ToSoftObjectPath().IsValid())
	{
		HudWidgetClass = TSoftClassPtr<UUserWidget>(
			FSoftObjectPath(TEXT("/Game/UI/Duel/WBP_GOTA_DuelHUD.WBP_GOTA_DuelHUD_C")));
	}
	if (!ResultsWidgetClass.ToSoftObjectPath().IsValid())
	{
		ResultsWidgetClass = TSoftClassPtr<UUserWidget>(
			FSoftObjectPath(TEXT("/Game/UI/Duel/WBP_GOTA_DuelResults.WBP_GOTA_DuelResults_C")));
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	UClass* HudClass = HudWidgetClass.LoadSynchronous();
	if (!HudClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DuelHUD: %s missing — using C++ base (no layout)"), *HudWidgetClass.ToString());
		HudClass = UGOTADuelHUDWidget::StaticClass();
	}
	ActiveHud = CreateWidget<UUserWidget>(PC, HudClass);
	if (ActiveHud)
	{
		ActiveHud->AddToViewport(50);
	}

	if (ADuelGameState* GS = GetWorld()->GetGameState<ADuelGameState>())
	{
		GS->OnDuelPhaseChanged.AddDynamic(this, &ADuelHUD::HandlePhaseChanged);
		GS->OnDuelMatchOutcomeChanged.AddDynamic(this, &ADuelHUD::HandleMatchOutcome);
		if (GS->MatchOutcome != EDuelMatchOutcome::None)
		{
			ShowResults();
		}
	}
}

void ADuelHUD::HandleMatchOutcome(EDuelMatchOutcome Outcome)
{
	if (Outcome != EDuelMatchOutcome::None)
	{
		ShowResults();
	}
}

void ADuelHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ADuelGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr)
	{
		GS->OnDuelPhaseChanged.RemoveAll(this);
		GS->OnDuelMatchOutcomeChanged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ADuelHUD::HandlePhaseChanged(EDuelPhase NewPhase)
{
	// Rematch moves the match out of MatchEnd — drop the results and give control back.
	if (NewPhase != EDuelPhase::MatchEnd && NewPhase != EDuelPhase::ReturningToLobby && ActiveResults)
	{
		HideResults();
	}
}

void ADuelHUD::ShowResults()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController() || ActiveResults)
	{
		return;
	}
	UClass* ResultsClass = ResultsWidgetClass.LoadSynchronous();
	if (!ResultsClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DuelHUD: %s missing — using C++ base (no layout)"), *ResultsWidgetClass.ToString());
		ResultsClass = UGOTADuelResultsWidget::StaticClass();
	}
	ActiveResults = CreateWidget<UUserWidget>(PC, ResultsClass);
	if (!ActiveResults)
	{
		return;
	}
	ActiveResults->AddToViewport(200);
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(ActiveResults->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	PC->SetIgnoreLookInput(true);
	// SetInputMode focused the widget root; hand focus to REMATCH so Enter / (A) work immediately.
	if (UGOTADuelResultsWidget* Results = Cast<UGOTADuelResultsWidget>(ActiveResults))
	{
		Results->FocusDefault();
	}
}

void ADuelHUD::HideResults()
{
	if (ActiveResults)
	{
		ActiveResults->RemoveFromParent();
		ActiveResults = nullptr;
	}
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		PC->ResetIgnoreLookInput();
	}
}
