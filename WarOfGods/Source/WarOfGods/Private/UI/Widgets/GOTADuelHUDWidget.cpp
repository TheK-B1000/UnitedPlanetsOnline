// White Tiger Studios 2024

#include "UI/Widgets/GOTADuelHUDWidget.h"

#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/DuelGameMode.h"
#include "Characters/GOTAHeroDefinition.h"
#include "Characters/GOTAHeroRegistry.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UI/GOTAMenuSubsystem.h"
#include "Engine/GameInstance.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "Combat/GOTACombatantObserver.h"
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
		FText Phase;
		switch (GS->DuelPhase)
		{
		case EDuelPhase::WaitingForPlayers: Phase = NSLOCTEXT("GOTA", "DuelWaiting", "AWAITING CHALLENGER"); break;
		case EDuelPhase::PreparingRound:
		case EDuelPhase::Countdown: Phase = NSLOCTEXT("GOTA", "DuelReady", "PREPARE YOURSELF"); break;
		case EDuelPhase::InRound: Phase = NSLOCTEXT("GOTA", "DuelFight", "FIGHT"); break;
		case EDuelPhase::RoundEnd: Phase = NSLOCTEXT("GOTA", "DuelRoundOver", "ROUND OVER"); break;
		case EDuelPhase::Intermission: Phase = NSLOCTEXT("GOTA", "DuelNextRound", "NEXT ROUND"); break;
		case EDuelPhase::MatchEnd: Phase = NSLOCTEXT("GOTA", "DuelMatchOver", "TRIAL COMPLETE"); break;
		case EDuelPhase::ReturningToLobby: Phase = NSLOCTEXT("GOTA", "DuelReturning", "RETURNING TO THE CAPITAL"); break;
		default: break;
		}
		Text_Phase->SetText(Phase);
		// The results panel owns the end-of-match message.
		Text_Phase->SetVisibility(GS->DuelPhase == EDuelPhase::MatchEnd ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
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
	APlayerState* OpponentPS = nullptr;
	APawn* OpponentPawn = ResolveOpponentPawn(OpponentPS);

	const ADuelPlayerState* LocalDuelPS = PC ? PC->GetPlayerState<ADuelPlayerState>() : nullptr;
	const UGOTAHeroRegistry* Heroes = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	const UGOTAHeroDefinition* LocalHero = (LocalDuelPS && Heroes) ? Heroes->FindHero(LocalDuelPS->HeroId) : nullptr;
	if (LocalHero && LocalHero->HeroId != ShownHeroId)
	{
		ShownHeroId = LocalHero->HeroId;
		if (Image_LocalHero)
		{
			if (UTexture2D* Thumb = LocalHero->Thumbnail.LoadSynchronous())
			{
				Image_LocalHero->SetBrushFromTexture(Thumb, false);
			}
		}
	}
	if (Text_LocalName && LocalHero)
	{
		Text_LocalName->SetText(LocalHero->DisplayName.ToUpper());
	}
	else if (Text_LocalName && PC)
	{
		// Same local identity as the frontend profile (OSS Null nicknames are machine ids).
		const UGOTAMenuSubsystem* Menu = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGOTAMenuSubsystem>() : nullptr;
		Text_LocalName->SetText(Menu ? Menu->GetProfileView().DisplayName
			: (PC->PlayerState ? FText::FromString(PC->PlayerState->GetPlayerName().ToUpper()) : FText::GetEmpty()));
	}
	if (Text_OpponentName)
	{
		Text_OpponentName->SetText(OpponentPS ? FText::FromString(OpponentPS->GetPlayerName().ToUpper()) : FText::GetEmpty());
	}
	// Health is read from the Blueprint damage system (read-only observer).
	if (Bar_LocalHealth)
	{
		const FGOTACombatantVitals Vitals = UGOTACombatantObserver::ReadVitals(LocalPawn);
		Bar_LocalHealth->SetPercent(Vitals.bHasDamageSystem ? Vitals.GetFraction() : 0.f);
	}
	if (Bar_OpponentHealth)
	{
		const FGOTACombatantVitals Vitals = UGOTACombatantObserver::ReadVitals(OpponentPawn);
		Bar_OpponentHealth->SetPercent(Vitals.bHasDamageSystem ? Vitals.GetFraction() : 0.f);
	}
}

APawn* UGOTADuelHUDWidget::ResolveOpponentPawn(APlayerState*& OutOpponentPS) const
{
	OutOpponentPS = nullptr;
	const ADuelGameState* GS = GetDuelGS();
	const APlayerController* PC = GetOwningPlayer();
	const ADuelPlayerState* LocalPS = PC ? PC->GetPlayerState<ADuelPlayerState>() : nullptr;
	if (!GS)
	{
		return nullptr;
	}
	const int32 LocalSlot = LocalPS ? LocalPS->DuelSlot : 0;
	for (APlayerState* PS : GS->PlayerArray)
	{
		const ADuelPlayerState* DuelPS = Cast<ADuelPlayerState>(PS);
		if (DuelPS && DuelPS != LocalPS && DuelPS->DuelSlot != INDEX_NONE && DuelPS->DuelSlot != LocalSlot)
		{
			OutOpponentPS = PS;
			break;
		}
	}
	// AI champions have no owning controller PlayerState link; the authority GameMode knows them.
	if (const ADuelGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADuelGameMode>() : nullptr)
	{
		return GM->GetFighterPawn(LocalSlot == 0 ? 1 : 0);
	}
	return OutOpponentPS ? OutOpponentPS->GetPawn() : nullptr;
}

ADuelGameState* UGOTADuelHUDWidget::GetDuelGS() const
{
	return GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr;
}
