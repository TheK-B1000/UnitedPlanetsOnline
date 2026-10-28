// White Tiger Studios 2024

#include "UI/Widgets/GOTAHordeResultsWidget.h"

#include "Arenas/Horde/HordeGameMode.h"
#include "Arenas/Horde/HordeGameState.h"
#include "Components/TextBlock.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "Kismet/GameplayStatics.h"

void UGOTAHordeResultsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Btn_Replay)
	{
		Btn_Replay->SetLabel(NSLOCTEXT("GOTA", "Replay", "FIGHT AGAIN"));
		Btn_Replay->OnActivated.AddDynamic(this, &UGOTAHordeResultsWidget::HandleReplay);
	}
	if (Btn_ReturnToLobby)
	{
		Btn_ReturnToLobby->SetLabel(NSLOCTEXT("GOTA", "ReturnLobby", "RETURN TO LOBBY"));
		Btn_ReturnToLobby->OnActivated.AddDynamic(this, &UGOTAHordeResultsWidget::HandleReturnToLobby);
	}
	Refresh();
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
	// Client: ask server via console/travel fallback
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
