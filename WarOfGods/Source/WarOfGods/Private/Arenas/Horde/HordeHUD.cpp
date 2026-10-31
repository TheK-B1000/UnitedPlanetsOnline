// White Tiger Studios 2024

#include "Arenas/Horde/HordeHUD.h"

#include "Arenas/Horde/HordeGameState.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/Widgets/GOTAHordeHUDWidget.h"
#include "UI/Widgets/GOTAHordeResultsWidget.h"

void AHordeHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	UClass* HudClass = HudWidgetClass.LoadSynchronous();
	if (!HudClass)
	{
		HudClass = UGOTAHordeHUDWidget::StaticClass();
	}

	ActiveHud = CreateWidget<UUserWidget>(PC, HudClass);
	if (ActiveHud)
	{
		ActiveHud->AddToViewport(50);
	}

	if (AHordeGameState* GS = GetWorld()->GetGameState<AHordeGameState>())
	{
		GS->OnHordeMatchOutcomeChanged.AddDynamic(this, &AHordeHUD::HandleMatchOutcome);
		if (GS->MatchOutcome != EHordeMatchOutcome::None)
		{
			ShowResults();
		}
	}
}

void AHordeHUD::HandleMatchOutcome(EHordeMatchOutcome Outcome)
{
	if (Outcome != EHordeMatchOutcome::None)
	{
		ShowResults();
	}
}

void AHordeHUD::ShowResults()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController() || ActiveResults)
	{
		return;
	}

	UClass* ResultsClass = ResultsWidgetClass.LoadSynchronous();
	if (!ResultsClass)
	{
		ResultsClass = UGOTAHordeResultsWidget::StaticClass();
	}

	ActiveResults = CreateWidget<UUserWidget>(PC, ResultsClass);
	if (ActiveResults)
	{
		ActiveResults->AddToViewport(200);
	}
}
