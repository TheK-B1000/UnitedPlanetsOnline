// White Tiger Studios 2024

#include "Arenas/Duel/DuelHUD.h"

#include "Arenas/Duel/DuelGameState.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

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

	if (UClass* HudClass = HudWidgetClass.LoadSynchronous())
	{
		ActiveHud = CreateWidget<UUserWidget>(PC, HudClass);
		if (ActiveHud)
		{
			ActiveHud->AddToViewport(50);
		}
	}

	if (ADuelGameState* GS = GetWorld()->GetGameState<ADuelGameState>())
	{
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

void ADuelHUD::ShowResults()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController() || ActiveResults)
	{
		return;
	}
	if (UClass* ResultsClass = ResultsWidgetClass.LoadSynchronous())
	{
		ActiveResults = CreateWidget<UUserWidget>(PC, ResultsClass);
		if (ActiveResults)
		{
			ActiveResults->AddToViewport(200);
		}
	}
}
