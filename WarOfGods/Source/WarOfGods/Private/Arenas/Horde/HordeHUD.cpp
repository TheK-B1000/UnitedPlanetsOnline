// White Tiger Studios 2024

#include "Arenas/Horde/HordeHUD.h"

#include "Arenas/Horde/HordeGameState.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void AHordeHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!HudWidgetClass.ToSoftObjectPath().IsValid())
	{
		HudWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/Horde/WBP_GOTA_HordeHUD.WBP_GOTA_HordeHUD_C")));
	}
	if (!ResultsWidgetClass.ToSoftObjectPath().IsValid())
	{
		ResultsWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/Horde/WBP_GOTA_HordeResults.WBP_GOTA_HordeResults_C")));
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
	if (UClass* ResultsClass = ResultsWidgetClass.LoadSynchronous())
	{
		ActiveResults = CreateWidget<UUserWidget>(PC, ResultsClass);
		if (ActiveResults)
		{
			ActiveResults->AddToViewport(200);
		}
	}
}
