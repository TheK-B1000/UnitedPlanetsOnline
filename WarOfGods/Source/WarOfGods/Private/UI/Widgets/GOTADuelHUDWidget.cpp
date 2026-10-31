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
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void UGOTADuelHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Text_Callout)
	{
		Text_Callout->SetRenderOpacity(0.f);
	}
	BindGameState();
	RefreshFromGameState();
}

void UGOTADuelHUDWidget::BindGameState()
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS || BoundGameState.Get() == GS)
	{
		return;
	}
	GS->OnDuelPhaseChanged.AddUniqueDynamic(this, &UGOTADuelHUDWidget::HandlePhaseChanged);
	GS->OnDuelScoreChanged.AddUniqueDynamic(this, &UGOTADuelHUDWidget::HandleScoreChanged);
	GS->OnDuelCombatCue.AddUniqueDynamic(this, &UGOTADuelHUDWidget::HandleCombatCue);
	BoundGameState = GS;
}

void UGOTADuelHUDWidget::ShowCallout(const FText& Text, const FLinearColor& Color, float Seconds)
{
	if (!Text_Callout)
	{
		return;
	}
	Text_Callout->SetText(Text);
	Text_Callout->SetColorAndOpacity(FSlateColor(Color));
	Text_Callout->SetRenderOpacity(1.f);
	CalloutRemaining = Seconds;
}

void UGOTADuelHUDWidget::HandleCombatCue(EGOTADuelCue Cue, AActor* Source, AActor* Target)
{
	static const FLinearColor Gold(1.f, 0.82f, 0.48f, 1.f);
	static const FLinearColor Danger(1.f, 0.36f, 0.26f, 1.f);
	static const FLinearColor Azure(0.55f, 0.78f, 1.f, 1.f);
	static const FLinearColor Muted(0.7f, 0.72f, 0.78f, 1.f);
	switch (Cue)
	{
	case EGOTADuelCue::PerfectParry: ShowCallout(NSLOCTEXT("GOTA", "CuePerfectParry", "PERFECT PARRY"), Gold, 1.1f); break;
	case EGOTADuelCue::PerfectDodge: ShowCallout(NSLOCTEXT("GOTA", "CuePerfectDodge", "PERFECT DODGE"), Azure, 1.1f); break;
	case EGOTADuelCue::GuardBreak: ShowCallout(NSLOCTEXT("GOTA", "CueGuardBreak", "GUARD BROKEN"), Gold, 1.4f); break;
	case EGOTADuelCue::UnblockableTelegraph: ShowCallout(NSLOCTEXT("GOTA", "CueUnblockable", "UNBLOCKABLE  —  EVADE"), Danger, 1.0f); break;
	case EGOTADuelCue::PhaseTwo: ShowCallout(NSLOCTEXT("GOTA", "CuePhaseTwo", "THE SOVEREIGN CHAMPION ASCENDS"), Danger, 2.0f); break;
	case EGOTADuelCue::SurgeTelegraph: ShowCallout(NSLOCTEXT("GOTA", "CueSurge", "SOVEREIGN SURGE  —  EVADE!"), Danger, 1.4f); break;
	case EGOTADuelCue::DivineCounter: ShowCallout(NSLOCTEXT("GOTA", "CueDivineCounter", "DIVINE COUNTER"), Gold, 1.3f); break;
	case EGOTADuelCue::CounterDenied:
		ShowCallout(Target ? NSLOCTEXT("GOTA", "CueCounterRange", "TOO FAR FOR A DIVINE COUNTER")
			: NSLOCTEXT("GOTA", "CueCounterMomentum", "NOT ENOUGH DIVINE MOMENTUM"), Muted, 0.9f);
		break;
	default: break;
	}
}

void UGOTADuelHUDWidget::RefreshCombatLayer(float InDeltaTime)
{
	if (Text_Callout && CalloutRemaining > 0.f)
	{
		CalloutRemaining -= InDeltaTime;
		Text_Callout->SetRenderOpacity(FMath::Clamp(CalloutRemaining / 0.35f, 0.f, 1.f));
	}

	const APlayerController* PC = GetOwningPlayer();
	const APawn* LocalPawn = PC ? PC->GetPawn() : nullptr;
	const UGOTADuelFighterComponent* Fighter = LocalPawn ? LocalPawn->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr;
	if (Bar_Momentum)
	{
		Bar_Momentum->SetPercent(Fighter ? Fighter->GetMomentumFraction() : 0.f);
		Bar_Momentum->SetFillColorAndOpacity(Fighter && Fighter->CanDivineCounter()
			? FLinearColor(1.f, 0.82f, 0.48f, 1.f) : FLinearColor(0.25f, 0.55f, 1.f, 1.f));
	}
	if (Text_Momentum)
	{
		const bool bReady = Fighter && Fighter->CanDivineCounter();
		Text_Momentum->SetText(bReady
			? NSLOCTEXT("GOTA", "MomentumReady", "DIVINE COUNTER READY  —  PRESS Q")
			: FText::Format(NSLOCTEXT("GOTA", "MomentumValue", "DIVINE MOMENTUM  {0}"), FText::AsNumber(FMath::FloorToInt(Fighter ? Fighter->GetMomentum() : 0.f))));
		Text_Momentum->SetColorAndOpacity(FSlateColor(bReady ? FLinearColor(1.f, 0.82f, 0.48f, 1.f) : FLinearColor(0.56f, 0.6f, 0.68f, 1.f)));
	}
	if (Text_ChampionState)
	{
		const ADuelGameState* GS = GetDuelGS();
		const bool bChampion = GS && GS->ChampionPawn && GS->ChampionPhase > 0;
		Text_ChampionState->SetVisibility(bChampion ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bChampion)
		{
			FString State = FGOTAChampionProfile::GetDifficultyName(GS->Difficulty).ToString().ToUpper();
			State += GS->ChampionPhase >= 2 ? TEXT("  ·  PHASE II — ASCENDED") : TEXT("  ·  PHASE I");
			if (GS->bChampionGuardBroken)
			{
				State += TEXT("  ·  GUARD BROKEN");
			}
			Text_ChampionState->SetText(FText::FromString(State));
			Text_ChampionState->SetColorAndOpacity(FSlateColor(GS->bChampionGuardBroken ? FLinearColor(1.f, 0.82f, 0.48f, 1.f)
				: (GS->ChampionPhase >= 2 ? FLinearColor(1.f, 0.36f, 0.26f, 1.f) : FLinearColor(0.56f, 0.6f, 0.68f, 1.f))));
		}
	}
}

void UGOTADuelHUDWidget::NativeDestruct()
{
	if (ADuelGameState* GS = BoundGameState.Get())
	{
		GS->OnDuelPhaseChanged.RemoveDynamic(this, &UGOTADuelHUDWidget::HandlePhaseChanged);
		GS->OnDuelScoreChanged.RemoveDynamic(this, &UGOTADuelHUDWidget::HandleScoreChanged);
		GS->OnDuelCombatCue.RemoveDynamic(this, &UGOTADuelHUDWidget::HandleCombatCue);
	}
	BoundGameState.Reset();
	Super::NativeDestruct();
}

void UGOTADuelHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	BindGameState();
	RefreshFromGameState();
	RefreshCombatLayer(InDeltaTime);
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
		const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
		if (GS->DuelPhase != ShownPhase)
		{
			ShownPhase = GS->DuelPhase;
			ShownPhaseSince = Now;
		}
		// The results panel owns the end-of-match message; "FIGHT" is a 1.5s announcement, not a banner.
		const bool bHide = GS->DuelPhase == EDuelPhase::MatchEnd
			|| (GS->DuelPhase == EDuelPhase::InRound && Now - ShownPhaseSince > 1.5);
		Text_Phase->SetVisibility(bHide ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
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
	// AI champions have no PlayerState link; the replicated GameState names them (works on clients).
	if (GS->ChampionPawn && LocalSlot == 0)
	{
		return GS->ChampionPawn;
	}
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
