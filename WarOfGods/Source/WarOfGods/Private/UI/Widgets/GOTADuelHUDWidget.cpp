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
#include "Abilities/GOTAHeroAbilityComponent.h"
#include "Bosses/GOTABossComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

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
	case EGOTADuelCue::BossTelegraph:
	case EGOTADuelCue::BossPhaseTwo:
	case EGOTADuelCue::BossTitle:
	case EGOTADuelCue::BossDefeated:
	{
		const UGOTABossComponent* B = Source ? Source->FindComponentByClass<UGOTABossComponent>() : nullptr;
		const FText T = B ? B->GetLastTelegraph() : FText::GetEmpty();
		if (Cue == EGOTADuelCue::BossTitle)
		{
			ShowCallout(T.ToUpper(), Gold, 2.6f);
		}
		else if (Cue == EGOTADuelCue::BossPhaseTwo)
		{
			ShowCallout(NSLOCTEXT("GOTA", "BossFury", "UNCHAINED FURY"), Danger, 2.2f);
		}
		else if (Cue == EGOTADuelCue::BossDefeated)
		{
			ShowCallout(FText::Format(NSLOCTEXT("GOTA", "BossFallen", "{0} FALLS"), T.ToUpper()), Gold, 2.5f);
		}
		else if (!T.IsEmpty())
		{
			ShowCallout(T, Danger, 1.0f);
		}
		break;
	}
	case EGOTADuelCue::AbilityUsed:
	{
		// Readable counterplay: name the ENEMY's ability as it winds up; own casts only for ultimates.
		const UGOTAHeroAbilityComponent* A = Source ? Source->FindComponentByClass<UGOTAHeroAbilityComponent>() : nullptr;
		const FGOTAAbilityDef* Def = A ? A->GetAbility(A->GetLastCastSlot()) : nullptr;
		const APawn* LocalPawn = GetOwningPlayerPawn();
		if (Def && Source != LocalPawn)
		{
			ShowCallout(FText::Format(NSLOCTEXT("GOTA", "CueEnemyAbility", "{0}{1}"), Def->DisplayName,
				Def->bUltimate ? NSLOCTEXT("GOTA", "CueEnemyUlt", "  -  EVADE!") : FText::GetEmpty()), Def->bUltimate ? Danger : Muted, Def->bUltimate ? 1.4f : 0.9f);
		}
		else if (Def && Def->bUltimate)
		{
			ShowCallout(Def->DisplayName, Gold, 1.3f);
		}
		break;
	}
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
	RefreshAbilityBar();
	const ADuelGameState* PhaseGS = GetDuelGS();
	const UGOTABossComponent* Boss = (PhaseGS && PhaseGS->ChampionPawn) ? PhaseGS->ChampionPawn->FindComponentByClass<UGOTABossComponent>() : nullptr;
	if (Text_ChampionState && Boss)
	{
		const bool bFury = Boss->GetPhase() >= 2;
		const bool bDead = Boss->GetState() == EGOTABossState::Dead;
		Text_ChampionState->SetVisibility(ESlateVisibility::HitTestInvisible);
		Text_ChampionState->SetText(bDead ? NSLOCTEXT("GOTA", "BossDown", "DEFEATED")
			: (bFury ? NSLOCTEXT("GOTA", "BossPhase2Line", "PHASE II  -  UNCHAINED FURY") : NSLOCTEXT("GOTA", "BossPhase1Line", "PHASE I")));
		Text_ChampionState->SetColorAndOpacity(FSlateColor(bFury ? FLinearColor(1.f, 0.36f, 0.26f, 1.f) : FLinearColor(1.f, 0.82f, 0.48f, 1.f)));
	}
	else if (Text_ChampionState)
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

void UGOTADuelHUDWidget::BuildAbilityBar(const UGOTAHeroAbilityComponent* Abilities)
{
	UCanvasPanel* Canvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
	if (AbilityBarRoot)
	{
		AbilityBarRoot->RemoveFromParent();
		AbilityBarRoot = nullptr;
	}
	AbilitySlots.Reset();
	BuiltForSet = Abilities ? Abilities->GetAbilitySet() : nullptr;
	if (!Canvas || !Abilities || Abilities->GetNumAbilities() == 0)
	{
		return;
	}
	const FSlateFontInfo Font = Text_Momentum ? Text_Momentum->GetFont() : FSlateFontInfo();
	auto MakeText = [this, &Font](int32 Size, const FLinearColor& Color, const FString& Str)
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo F = Font;
		F.Size = Size;
		T->SetFont(F);
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetText(FText::FromString(Str));
		T->SetJustification(ETextJustify::Center);
		return T;
	};

	AbilityBarRoot = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	static const TCHAR* Keys[] = { TEXT("E"), TEXT("R"), TEXT("C"), TEXT("V") };
	for (int32 i = 0; i < Abilities->GetNumAbilities() && i < 4; ++i)
	{
		const FGOTAAbilityDef* Def = Abilities->GetAbility(i);
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.035f, 0.78f));
		Frame->SetPadding(FMargin(10.f, 6.f));
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* Key = MakeText(12, FLinearColor(1.f, 0.82f, 0.48f, 1.f), Keys[i]);
		UTextBlock* Name = MakeText(11, FLinearColor(0.9f, 0.92f, 0.95f, 1.f),
			(Def->ShortLabel.IsEmpty() ? Def->DisplayName : Def->ShortLabel).ToString().ToUpper());
		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
		Bar->SetFillColorAndOpacity(FLinearColor(1.f, 0.82f, 0.48f, 1.f));
		UTextBlock* State = MakeText(10, FLinearColor(0.56f, 0.6f, 0.68f, 1.f), TEXT(""));
		Col->AddChildToVerticalBox(Key)->SetHorizontalAlignment(HAlign_Center);
		Col->AddChildToVerticalBox(Name)->SetHorizontalAlignment(HAlign_Center);
		UVerticalBoxSlot* BarSlot = Col->AddChildToVerticalBox(Bar);
		BarSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 2.f));
		Col->AddChildToVerticalBox(State)->SetHorizontalAlignment(HAlign_Center);
		Frame->SetContent(Col);
		UHorizontalBoxSlot* HSlot = Row->AddChildToHorizontalBox(Frame);
		HSlot->SetPadding(FMargin(5.f, 0.f));
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		FAbilitySlotWidgets W;
		W.Frame = Frame;
		W.Key = Key;
		W.Name = Name;
		W.Cooldown = Bar;
		W.State = State;
		AbilitySlots.Add(W);
	}
	Text_AbilityStatus = MakeText(12, FLinearColor(1.f, 0.82f, 0.48f, 1.f), TEXT(""));
	AbilityBarRoot->AddChildToVerticalBox(Text_AbilityStatus)->SetHorizontalAlignment(HAlign_Center);
	AbilityBarRoot->AddChildToVerticalBox(Row);
	UCanvasPanelSlot* CSlot = Canvas->AddChildToCanvas(AbilityBarRoot);
	CSlot->SetAnchors(FAnchors(0.5f, 1.f));
	CSlot->SetAlignment(FVector2D(0.5f, 1.f));
	CSlot->SetPosition(FVector2D(0.f, -24.f));
	CSlot->SetSize(FVector2D(620.f, 96.f));
}

void UGOTADuelHUDWidget::RefreshAbilityBar()
{
	const APawn* LocalPawn = GetOwningPlayerPawn();
	const UGOTAHeroAbilityComponent* A = LocalPawn ? LocalPawn->FindComponentByClass<UGOTAHeroAbilityComponent>() : nullptr;
	if ((A ? A->GetAbilitySet() : nullptr) != BuiltForSet.Get() || (!A && AbilityBarRoot))
	{
		BuildAbilityBar(A);
	}
	if (!A || AbilitySlots.Num() == 0)
	{
		return;
	}
	const int32 Active = A->GetActiveSlot();
	for (int32 i = 0; i < AbilitySlots.Num(); ++i)
	{
		const FGOTAAbilityDef* Def = A->GetAbility(i);
		FAbilitySlotWidgets& W = AbilitySlots[i];
		if (!Def || !W.Cooldown.IsValid())
		{
			continue;
		}
		const float Remaining = A->GetCooldownRemaining(i);
		const bool bReady = A->IsSlotReady(i);
		const bool bCooling = Remaining > 0.f;
		W.Cooldown->SetPercent(bCooling ? 1.f - Remaining / FMath::Max(Def->Cooldown, 0.01f) : 1.f);
		const FLinearColor ReadyColor = Def->bUltimate ? FLinearColor(1.f, 0.36f, 0.26f, 1.f) : FLinearColor(1.f, 0.82f, 0.48f, 1.f);
		W.Cooldown->SetFillColorAndOpacity(bReady ? ReadyColor : FLinearColor(0.3f, 0.32f, 0.38f, 1.f));
		FString State;
		if (i == Active)
		{
			State = TEXT("CASTING");
		}
		else if (bCooling)
		{
			State = FString::Printf(TEXT("%.1fs"), Remaining);
		}
		else if (!bReady && Def->MomentumCost > 0.f)
		{
			State = FString::Printf(TEXT("NEEDS %d MOMENTUM"), FMath::RoundToInt(Def->MomentumCost));
		}
		else
		{
			State = Def->bUltimate ? TEXT("ULTIMATE READY") : TEXT("READY");
		}
		W.State->SetText(FText::FromString(State));
		W.State->SetColorAndOpacity(FSlateColor(bReady || i == Active ? ReadyColor : FLinearColor(0.56f, 0.6f, 0.68f, 1.f)));
		W.Frame->SetBrushColor(i == Active ? FLinearColor(0.25f, 0.18f, 0.06f, 0.9f)
			: (Def->bUltimate && bReady ? FLinearColor(0.22f, 0.05f, 0.03f, 0.85f) : FLinearColor(0.02f, 0.025f, 0.035f, 0.78f)));
		W.Name->SetRenderOpacity(bReady || i == Active ? 1.f : 0.55f);
	}
	if (Text_AbilityStatus)
	{
		FString Status;
		switch (A->GetStatus())
		{
		case EGOTAStatus::Unbreakable: Status = TEXT("UNBREAKABLE"); break;
		case EGOTAStatus::Veiled: Status = TEXT("VEILED"); break;
		case EGOTAStatus::Dread: Status = TEXT("DREAD  -  SLOWED"); break;
		default: break;
		}
		const float Left = A->GetStatusRemaining();
		Text_AbilityStatus->SetText(Status.IsEmpty() || Left <= 0.f ? FText::GetEmpty() : FText::FromString(FString::Printf(TEXT("%s  %.1fs"), *Status, Left)));
	}
	// Server refusals (cooldown / momentum / range / line of sight ...).
	double When = 0.0;
	const EGOTAAbilityDenied Denied = A->GetLastDenied(When);
	if (Denied != EGOTAAbilityDenied::None && When > ShownDeniedTime)
	{
		ShownDeniedTime = When;
		FText Msg;
		switch (Denied)
		{
		case EGOTAAbilityDenied::Cooldown: Msg = NSLOCTEXT("GOTA", "AbDenyCd", "ABILITY ON COOLDOWN"); break;
		case EGOTAAbilityDenied::Momentum: Msg = NSLOCTEXT("GOTA", "AbDenyMom", "NOT ENOUGH DIVINE MOMENTUM"); break;
		case EGOTAAbilityDenied::NoTarget: Msg = NSLOCTEXT("GOTA", "AbDenyTarget", "NO TARGET"); break;
		case EGOTAAbilityDenied::OutOfRange: Msg = NSLOCTEXT("GOTA", "AbDenyRange", "OUT OF RANGE"); break;
		case EGOTAAbilityDenied::NoLineOfSight: Msg = NSLOCTEXT("GOTA", "AbDenyLOS", "NO LINE OF SIGHT"); break;
		case EGOTAAbilityDenied::NoSafeDestination: Msg = NSLOCTEXT("GOTA", "AbDenyDest", "NO SAFE DESTINATION"); break;
		case EGOTAAbilityDenied::Busy: Msg = NSLOCTEXT("GOTA", "AbDenyBusy", "BUSY"); break;
		default: break;
		}
		if (!Msg.IsEmpty())
		{
			ShowCallout(Msg, FLinearColor(0.7f, 0.72f, 0.78f, 1.f), 0.8f);
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
	const ADuelGameState* BossGS = GetDuelGS();
	const UGOTABossComponent* BossComp = (BossGS && BossGS->ChampionPawn) ? BossGS->ChampionPawn->FindComponentByClass<UGOTABossComponent>() : nullptr;
	if (Text_OpponentName)
	{
		if (BossComp && BossComp->GetDefinition())
		{
			Text_OpponentName->SetText(BossComp->GetDefinition()->DisplayName.ToUpper());
		}
		else
		{
			Text_OpponentName->SetText(OpponentPS ? FText::FromString(OpponentPS->GetPlayerName().ToUpper()) : FText::GetEmpty());
		}
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
