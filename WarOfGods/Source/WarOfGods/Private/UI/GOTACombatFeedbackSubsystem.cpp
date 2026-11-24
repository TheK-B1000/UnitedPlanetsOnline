// White Tiger Studios 2024

#include "UI/GOTACombatFeedbackSubsystem.h"

#include "Abilities/GAS/GOTAAbilitySystemComponent.h"
#include "Abilities/GAS/GOTAStatusEffects.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Blueprint/WidgetTree.h"
#include "Bosses/GOTABossComponent.h"
#include "Bosses/GOTABossTypes.h"
#include "Characters/GOTAPlayerCharacter.h"
#include "Combat/GOTACombatantObserver.h"
#include "Combat/LockonComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTACombatFeedbackSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAFeedback, Log, All);

namespace
{
	/** Chips per row. Statuses arrive most important first, so anything past this is the least urgent. */
	constexpr int32 MaxStatusChips = 6;
	constexpr float ReadoutInterval = 0.1f;
	constexpr float TargetInterval = 0.5f;

	struct FChipStyle { const TCHAR* Short; FLinearColor Color; };

	/** Hot and cold read as their element; buffs are green or gold; debuffs are violet or red. */
	FChipStyle ChipStyle(EGOTAStatus Status)
	{
		switch (Status)
		{
		case EGOTAStatus::Frozen: return { TEXT("FROZEN"), FLinearColor(0.55f, 0.85f, 1.f) };
		case EGOTAStatus::Stunned: return { TEXT("STUN"), FLinearColor(1.f, 0.9f, 0.3f) };
		case EGOTAStatus::Burning: return { TEXT("BURN"), FLinearColor(1.f, 0.45f, 0.12f) };
		case EGOTAStatus::Chilled: return { TEXT("CHILL"), FLinearColor(0.45f, 0.7f, 1.f) };
		case EGOTAStatus::Shocked: return { TEXT("SHOCK"), FLinearColor(0.75f, 0.65f, 1.f) };
		case EGOTAStatus::Vulnerable: return { TEXT("VULN"), FLinearColor(1.f, 0.3f, 0.35f) };
		case EGOTAStatus::Weakened: return { TEXT("WEAK"), FLinearColor(0.8f, 0.45f, 0.75f) };
		case EGOTAStatus::Dread: return { TEXT("DREAD"), FLinearColor(0.65f, 0.35f, 0.9f) };
		case EGOTAStatus::Slowed: return { TEXT("SLOW"), FLinearColor(0.6f, 0.6f, 0.85f) };
		case EGOTAStatus::Shielded: return { TEXT("SHIELD"), FLinearColor(0.35f, 0.8f, 1.f) };
		case EGOTAStatus::Veiled: return { TEXT("VEIL"), FLinearColor(0.7f, 0.7f, 0.95f) };
		case EGOTAStatus::Unbreakable: return { TEXT("UNBRK"), FLinearColor(1.f, 0.8f, 0.35f) };
		case EGOTAStatus::Fortified: return { TEXT("FORT"), FLinearColor(0.45f, 0.9f, 0.55f) };
		case EGOTAStatus::Empowered: return { TEXT("POWER"), FLinearColor(1.f, 0.7f, 0.25f) };
		case EGOTAStatus::Hasted: return { TEXT("HASTE"), FLinearColor(0.4f, 1.f, 0.7f) };
		default: return { TEXT("?"), FLinearColor::White };
		}
	}
}

// ============================================================================ widget

TSharedRef<SWidget> UGOTACombatFeedbackWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UGOTACombatFeedbackWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
}

void UGOTACombatFeedbackWidget::EnsureLayout()
{
	if (Line || !WidgetTree)
	{
		return;
	}
	UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FeedbackRoot"));
		WidgetTree->RootWidget = Root;
	}
	Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FeedbackLine"));
	FSlateFontInfo Font = Line->GetFont();
	Font.Size = 22;
	Line->SetFont(Font);
	Line->SetShadowOffset(FVector2D(1.5f, 1.5f));
	Line->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	Line->SetJustification(ETextJustify::Center);
	Line->SetRenderOpacity(0.f);
	if (UCanvasPanelSlot* LineSlot = Root->AddChildToCanvas(Line))
	{
		LineSlot->SetAutoSize(true);
		LineSlot->SetAnchors(FAnchors(0.5f, 0.2f));
		LineSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}
	// Status indicator: above the ability bar, below the centre of the screen. Hidden until a status event shows it.
	StatusLine = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FeedbackStatus"));
	Font.Size = 18;
	StatusLine->SetFont(Font);
	StatusLine->SetShadowOffset(FVector2D(1.5f, 1.5f));
	StatusLine->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	StatusLine->SetJustification(ETextJustify::Center);
	StatusLine->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* StatusSlot = Root->AddChildToCanvas(StatusLine))
	{
		StatusSlot->SetAutoSize(true);
		StatusSlot->SetAnchors(FAnchors(0.5f, 0.78f));
		StatusSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}

	// The local player's chips and shield sit together just above the hard-status line; the target's sit under the top bars.
	LocalRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FeedbackLocalChips"));
	if (UCanvasPanelSlot* RowSlot = Root->AddChildToCanvas(LocalRow))
	{
		RowSlot->SetAutoSize(true);
		RowSlot->SetAnchors(FAnchors(0.5f, 0.855f)); // low, under the hero's body and just above the ability bar
		RowSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}
	BuildChips(LocalRow, LocalChips);
	TargetRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FeedbackTargetChips"));
	if (UCanvasPanelSlot* RowSlot = Root->AddChildToCanvas(TargetRow))
	{
		RowSlot->SetAutoSize(true);
		// Under the opponent's bar, growing right, so it never covers the score or the round timer.
		RowSlot->SetAnchors(FAnchors(0.56f, 0.14f));
		RowSlot->SetAlignment(FVector2D(0.f, 0.5f));
	}
	BuildChips(TargetRow, TargetChips);

	ShieldBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FeedbackShield"));
	ShieldBox->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.62f));
	ShieldBox->SetPadding(FMargin(6.f, 2.f));
	UVerticalBox* ShieldColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FeedbackShieldColumn"));
	ShieldBox->SetContent(ShieldColumn);
	ShieldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FeedbackShieldText"));
	Font.Size = 12;
	ShieldText->SetFont(Font);
	ShieldText->SetShadowOffset(FVector2D(1.f, 1.f));
	ShieldText->SetColorAndOpacity(FSlateColor(ChipStyle(EGOTAStatus::Shielded).Color));
	ShieldText->SetJustification(ETextJustify::Center);
	ShieldBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("FeedbackShieldBar"));
	ShieldBar->SetFillColorAndOpacity(ChipStyle(EGOTAStatus::Shielded).Color);
	ShieldColumn->AddChildToVerticalBox(ShieldText);
	if (UVerticalBoxSlot* BarSlot = ShieldColumn->AddChildToVerticalBox(ShieldBar))
	{
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	ShieldBox->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* BoxSlot = Root->AddChildToCanvas(ShieldBox))
	{
		BoxSlot->SetAnchors(FAnchors(0.5f, 0.81f));
		BoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		BoxSlot->SetSize(FVector2D(280.f, 38.f));
	}
}

void UGOTACombatFeedbackWidget::BuildChips(UHorizontalBox* Row, TArray<FGOTAStatusChip>& Pool)
{
	for (int32 i = 0; i < MaxStatusChips; ++i)
	{
		FGOTAStatusChip& Chip = Pool.AddDefaulted_GetRef();
		Chip.Box = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Chip.Box->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.62f));
		Chip.Box->SetPadding(FMargin(3.f, 1.f));
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Chip.Box->SetContent(Column);
		Chip.Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo Font = Chip.Label->GetFont();
		Font.Size = 13;
		Chip.Label->SetFont(Font);
		Chip.Label->SetShadowOffset(FVector2D(1.f, 1.f));
		Chip.Label->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
		Chip.Label->SetJustification(ETextJustify::Center);
		Chip.Label->SetMinDesiredWidth(82.f);
		Chip.Time = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		Column->AddChildToVerticalBox(Chip.Label);
		if (UVerticalBoxSlot* TimeSlot = Column->AddChildToVerticalBox(Chip.Time))
		{
			TimeSlot->SetPadding(FMargin(2.f, 1.f));
		}
		if (UHorizontalBoxSlot* ChipSlot = Row->AddChildToHorizontalBox(Chip.Box))
		{
			ChipSlot->SetPadding(FMargin(3.f, 0.f));
		}
		Chip.Box->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGOTACombatFeedbackWidget::FillChips(const UGOTAAbilitySystemComponent* ASC, TArray<FGOTAStatusChip>& Pool, FString& OutKey, FString& OutLog)
{
	int32 Used = 0;
	if (ASC)
	{
		for (const FGOTAStatusTimer& Timer : ASC->GetStatusTimers())
		{
			const float Left = ASC->GetStatusTimeLeft(Timer);
			if (Left <= 0.f)
			{
				continue; // ended here; the server's removal is on its way
			}
			const FString Name = StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(Timer.Status));
			// The end time, not the countdown: the key changes on a refresh and stays put while time runs.
			OutKey += FString::Printf(TEXT("%s@%.2f,"), *Name, Timer.EndServerTime);
			OutLog += FString::Printf(TEXT("%s%s %.1f/%.1f"), OutLog.IsEmpty() ? TEXT("") : TEXT(" "), *Name, Left, Timer.Duration);
			if (!Pool.IsValidIndex(Used))
			{
				continue; // past the row's limit: still logged, not drawn
			}
			const FChipStyle Style = ChipStyle(Timer.Status);
			const FGOTAStatusChip& Chip = Pool[Used++];
			const FString Stacks = Timer.Stacks > 1 ? FString::Printf(TEXT(" x%d"), Timer.Stacks) : FString();
			Chip.Label->SetText(FText::FromString(FString::Printf(TEXT("%s%s %.1f"), Style.Short, *Stacks, Left)));
			Chip.Label->SetColorAndOpacity(FSlateColor(Style.Color));
			Chip.Time->SetFillColorAndOpacity(Style.Color);
			Chip.Time->SetPercent(Timer.Duration > 0.f ? FMath::Clamp(Left / Timer.Duration, 0.f, 1.f) : 0.f);
			Chip.Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	for (int32 i = Used; i < Pool.Num(); ++i)
	{
		Pool[i].Box->SetVisibility(ESlateVisibility::Collapsed);
	}
}

APawn* UGOTACombatFeedbackWidget::ResolveTarget(const APawn* Local) const
{
	UWorld* World = GetWorld();
	if (!World || !Local)
	{
		return nullptr;
	}
	if (const ULockonComponent* Lockon = Local->FindComponentByClass<ULockonComponent>())
	{
		if (APawn* Locked = Cast<APawn>(Lockon->GetCurrentTarget()))
		{
			return Locked;
		}
	}
	if (const ADuelGameState* Duel = World->GetGameState<ADuelGameState>())
	{
		if (Duel->ChampionPawn && Duel->ChampionPawn != Local)
		{
			return Duel->ChampionPawn;
		}
		for (const APlayerState* PS : Duel->PlayerArray)
		{
			if (APawn* Other = PS ? PS->GetPawn() : nullptr; Other && Other != Local)
			{
				return Other;
			}
		}
		return nullptr;
	}
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		if (It->FindComponentByClass<UGOTABossComponent>() && !UGOTACombatantObserver::IsCombatantDefeated(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

void UGOTACombatFeedbackWidget::UpdateReadout()
{
	if (!LocalRow)
	{
		return;
	}
	const APlayerController* PC = GetOwningPlayer();
	const APawn* Local = PC ? PC->GetPawn() : nullptr;
	TargetWait -= ReadoutInterval;
	if (TargetWait <= 0.f || !ReadoutTarget.IsValid())
	{
		ReadoutTarget = ResolveTarget(Local);
		TargetWait = TargetInterval;
	}
	const APawn* Target = ReadoutTarget.Get();

	const UGOTAAbilitySystemComponent* LocalASC = UGOTAAbilitySystemComponent::Find(Local);
	FString Key, LocalLog, TargetLog;
	FillChips(LocalASC, LocalChips, Key, LocalLog);
	Key += TEXT("|");
	FillChips(UGOTAAbilitySystemComponent::Find(Target), TargetChips, Key, TargetLog);

	// The shield: what is left of what was granted, from the replicated pool. Hidden the moment it is gone.
	const float Pool = LocalASC ? LocalASC->GetShieldPool() : 0.f;
	float Granted = Pool;
	if (LocalASC)
	{
		if (const FGOTAStatusTimer* Timer = LocalASC->GetStatusTimers().FindByPredicate([](const FGOTAStatusTimer& T) { return T.Status == EGOTAStatus::Shielded; }))
		{
			Granted = FMath::Max(Pool, Timer->Magnitude);
		}
	}
	const bool bShield = Pool > 0.01f && Granted > 0.f;
	ShieldBox->SetVisibility(bShield ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bShield)
	{
		ShieldBar->SetPercent(FMath::Clamp(Pool / Granted, 0.f, 1.f));
		ShieldText->SetText(FText::Format(NSLOCTEXT("GOTA", "ShieldBar", "SHIELD {0} / {1}"), FText::AsNumber(FMath::CeilToInt(Pool)), FText::AsNumber(FMath::RoundToInt(Granted))));
	}
	Key += FString::Printf(TEXT("|%.1f/%.1f|%s"), bShield ? Pool : 0.f, bShield ? Granted : 0.f, *GetNameSafe(Target));

	// One line per real change (never per countdown step), so tests can read what the player was shown.
	if (Key != LastReadoutKey)
	{
		LastReadoutKey = Key;
		const GOTAGAS::FDisplayVitals Vitals = GOTAGAS::ReadDisplayVitals(Local);
		const AGOTAPlayerCharacter* Hero = Cast<AGOTAPlayerCharacter>(Local);
		const float HudBar = Hero ? Hero->ReadHudHealthBar() : -1.f;
		UE_LOG(LogGOTAFeedback, Log, TEXT("FEEDBACK CHIPS local=[%s] shield=%.0f/%.0f hp=%.0f/%.0f target=%s[%s] bar=%d"), *LocalLog, bShield ? Pool : 0.f, bShield ? Granted : 0.f,
			Vitals.Health, Vitals.MaxHealth, *GetNameSafe(Target), *TargetLog, HudBar < 0.f ? FMath::RoundToInt(HudBar) : FMath::RoundToInt(HudBar * 100.f));
	}
}

void UGOTACombatFeedbackWidget::SetStatus(const FText& Text, const FLinearColor& Color)
{
	EnsureLayout();
	if (!StatusLine)
	{
		return;
	}
	StatusLine->SetText(Text);
	StatusLine->SetColorAndOpacity(FSlateColor(Color));
	StatusLine->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UGOTACombatFeedbackWidget::Show(const FText& Text, const FLinearColor& Color, float Seconds)
{
	EnsureLayout();
	if (!Line)
	{
		return;
	}
	Line->SetText(Text);
	Line->SetColorAndOpacity(FSlateColor(Color));
	Line->SetRenderOpacity(1.f);
	Remaining = Seconds;
}

void UGOTACombatFeedbackWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (Line && Remaining > 0.f)
	{
		Remaining -= InDeltaTime;
		Line->SetRenderOpacity(FMath::Clamp(Remaining / 0.4f, 0.f, 1.f));
	}
	ReadoutWait -= InDeltaTime;
	if (ReadoutWait <= 0.f)
	{
		ReadoutWait = ReadoutInterval;
		UpdateReadout();
	}
}

// ============================================================================ subsystem

void UGOTACombatFeedbackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResistedHandle = GOTAGAS::OnStatusResisted.AddUObject(this, &UGOTACombatFeedbackSubsystem::HandleStatusResisted);
	StatusCueHandle = GOTAGAS::OnStatusCue.AddUObject(this, &UGOTACombatFeedbackSubsystem::HandleStatusCue);
}

void UGOTACombatFeedbackSubsystem::Deinitialize()
{
	GOTAGAS::OnStatusResisted.Remove(ResistedHandle);
	GOTAGAS::OnStatusCue.Remove(StatusCueHandle);
	Super::Deinitialize();
}

UGOTACombatFeedbackWidget* UGOTACombatFeedbackSubsystem::EnsureWidget()
{
	ULocalPlayer* LP = GetLocalPlayer();
	UWorld* World = LP ? LP->GetWorld() : nullptr;
	APlayerController* PC = LP ? LP->GetPlayerController(World) : nullptr;
	// A pawn destroyed while the world shuts down still refreshes the status line. Widgets cannot be created then.
	if (!PC || World->bIsTearingDown)
	{
		return nullptr;
	}
	// The viewport is cleared on every map change, so a widget from an earlier world is rebuilt.
	if (!Widget || !Widget->IsInViewport() || Widget->GetWorld() != World)
	{
		Widget = CreateWidget<UGOTACombatFeedbackWidget>(PC, UGOTACombatFeedbackWidget::StaticClass());
		if (Widget)
		{
			Widget->AddToViewport(50);
		}
	}
	return Widget;
}

void UGOTACombatFeedbackSubsystem::ShowMessage(const FText& Text, const FLinearColor& Color, float Seconds)
{
	if (UGOTACombatFeedbackWidget* Live = EnsureWidget())
	{
		Live->Show(Text, Color, Seconds);
		UE_LOG(LogGOTAFeedback, Log, TEXT("FEEDBACK %s"), *Text.ToString());
	}
}

namespace
{
	/** Statuses with a local indicator, most important first, and their words and colours. */
	struct FIndicated { EGOTAStatus Status; const TCHAR* Log; FText Text; FLinearColor Color; };
	const TArray<FIndicated>& Indicated()
	{
		static const TArray<FIndicated> List = {
			{ EGOTAStatus::Frozen, TEXT("FROZEN"), NSLOCTEXT("GOTA", "StatusFrozen", "FROZEN"), FLinearColor(0.55f, 0.85f, 1.f, 1.f) },
			{ EGOTAStatus::Stunned, TEXT("STUNNED"), NSLOCTEXT("GOTA", "StatusStunned", "STUNNED"), FLinearColor(1.f, 0.9f, 0.3f, 1.f) },
			{ EGOTAStatus::Burning, TEXT("BURNING"), NSLOCTEXT("GOTA", "StatusBurning", "BURNING"), FLinearColor(1.f, 0.45f, 0.12f, 1.f) },
		};
		return List;
	}
}

void UGOTACombatFeedbackSubsystem::SetLocalStatus(AActor* Pawn, EGOTAStatus Status, bool bOn)
{
	const FIndicated* Entry = Indicated().FindByPredicate([Status](const FIndicated& I) { return I.Status == Status; });
	if (!Entry)
	{
		return;
	}
	if (bOn && Pawn != StatusPawn.Get())
	{
		// A new local pawn (respawn): start clean.
		if (AActor* Old = StatusPawn.Get())
		{
			Old->OnDestroyed.RemoveDynamic(this, &UGOTACombatFeedbackSubsystem::HandleStatusPawnDestroyed);
		}
		LocalStatuses.Reset();
		StatusPawn = Pawn;
		Pawn->OnDestroyed.AddUniqueDynamic(this, &UGOTACombatFeedbackSubsystem::HandleStatusPawnDestroyed);
	}
	const bool bHad = LocalStatuses.Contains(Status);
	if (bOn == bHad)
	{
		return;
	}
	if (bOn)
	{
		LocalStatuses.Add(Status);
	}
	else
	{
		LocalStatuses.Remove(Status);
	}
	UE_LOG(LogGOTAFeedback, Log, TEXT("FEEDBACK STATUS %s %s"), Entry->Log, bOn ? TEXT("on") : TEXT("off"));
	RefreshStatusLine();
}

void UGOTACombatFeedbackSubsystem::RefreshStatusLine()
{
	FString Line;
	FLinearColor Color = FLinearColor::White;
	for (const FIndicated& I : Indicated())
	{
		if (LocalStatuses.Contains(I.Status))
		{
			if (Line.IsEmpty())
			{
				Color = I.Color; // the most important status sets the colour
			}
			Line += (Line.IsEmpty() ? TEXT("") : TEXT("  |  ")) + I.Text.ToString();
		}
	}
	if (UGOTACombatFeedbackWidget* Live = (!Line.IsEmpty() || Widget) ? EnsureWidget() : nullptr)
	{
		Live->SetStatus(FText::FromString(Line), Color);
	}
}

void UGOTACombatFeedbackSubsystem::HandleStatusCue(AActor* Target, EGOTAStatus Status, bool bActive)
{
	if (!Target)
	{
		return;
	}
	if (!bActive)
	{
		if (Target == StatusPawn.Get())
		{
			SetLocalStatus(Target, Status, false);
		}
		return;
	}
	// Only the player whose own pawn has the status sees the line; everyone sees the body (overlay, flames, ice).
	ULocalPlayer* LP = GetLocalPlayer();
	UWorld* World = LP ? LP->GetWorld() : nullptr;
	const APlayerController* PC = LP ? LP->GetPlayerController(World) : nullptr;
	if (PC && PC->GetPawn() == Target)
	{
		SetLocalStatus(Target, Status, true);
	}
	// The chips also show the target's statuses, so the first status on anyone in this world brings the widget up.
	if (PC && Target->GetWorld() == World)
	{
		EnsureWidget();
	}
}

void UGOTACombatFeedbackSubsystem::HandleStatusPawnDestroyed(AActor* Destroyed)
{
	if (Destroyed == StatusPawn.Get() || !StatusPawn.IsValid())
	{
		for (const EGOTAStatus S : TArray<EGOTAStatus>(LocalStatuses))
		{
			const FIndicated* Entry = Indicated().FindByPredicate([S](const FIndicated& I) { return I.Status == S; });
			UE_LOG(LogGOTAFeedback, Log, TEXT("FEEDBACK STATUS %s off"), Entry ? Entry->Log : TEXT("?"));
		}
		LocalStatuses.Reset();
		StatusPawn.Reset();
		RefreshStatusLine();
	}
}

FText UGOTACombatFeedbackSubsystem::DescribeResist(const AActor* Target, EGOTAStatus Status, int32 Aspects)
{
	FText Name = NSLOCTEXT("GOTA", "ResistTarget", "TARGET");
	if (const UGOTABossComponent* Boss = Target ? Target->FindComponentByClass<UGOTABossComponent>() : nullptr)
	{
		// A client may have unloaded the (softly held) definition since the boss was built; reload it for the name.
		const UGOTABossDefinition* Def = Boss->GetDefinition() ? Boss->GetDefinition() : Boss->GetDefinitionRef().LoadSynchronous();
		Name = Def ? Def->DisplayName.ToUpper() : NSLOCTEXT("GOTA", "ResistBoss", "THE BOSS");
	}
	FText What;
	switch (Status)
	{
	case EGOTAStatus::Slowed: What = NSLOCTEXT("GOTA", "ResistSlow", "SLOW"); break;
	case EGOTAStatus::Dread: What = NSLOCTEXT("GOTA", "ResistDread", "DREAD"); break;
	case EGOTAStatus::Veiled: What = NSLOCTEXT("GOTA", "ResistVeiled", "VEIL"); break;
	case EGOTAStatus::Unbreakable: What = NSLOCTEXT("GOTA", "ResistUnbreakable", "UNBREAKABLE"); break;
	case EGOTAStatus::Burning: What = NSLOCTEXT("GOTA", "ResistBurning", "BURNING"); break;
	case EGOTAStatus::Stunned: What = NSLOCTEXT("GOTA", "ResistStunned", "STUN"); break;
	case EGOTAStatus::Frozen: What = NSLOCTEXT("GOTA", "ResistFrozen", "FREEZE"); break;
	case EGOTAStatus::Weakened: What = NSLOCTEXT("GOTA", "ResistWeakened", "WEAKNESS"); break;
	case EGOTAStatus::Vulnerable: What = NSLOCTEXT("GOTA", "ResistVulnerable", "VULNERABILITY"); break;
	case EGOTAStatus::Chilled: What = NSLOCTEXT("GOTA", "ResistChilled", "CHILL"); break;
	case EGOTAStatus::Shocked: What = NSLOCTEXT("GOTA", "ResistShocked", "SHOCK"); break;
	default: What = NSLOCTEXT("GOTA", "ResistEffect", "EFFECT"); break;
	}
	if (Aspects & GOTAGAS::Aspect_Whole)
	{
		return FText::Format(NSLOCTEXT("GOTA", "ResistWhole", "{0} RESISTS {1}"), Name, What);
	}
	if ((Aspects & GOTAGAS::Aspect_Movement) && (Aspects & GOTAGAS::Aspect_Weaken))
	{
		return FText::Format(NSLOCTEXT("GOTA", "ResistBoth", "{0} RESISTS {1}"), Name, What);
	}
	if (Aspects & GOTAGAS::Aspect_Movement)
	{
		return FText::Format(NSLOCTEXT("GOTA", "ResistMove", "{0} RESISTS THE SLOW OF {1}"), Name, What);
	}
	if (Aspects & GOTAGAS::Aspect_Weaken)
	{
		return FText::Format(NSLOCTEXT("GOTA", "ResistWeaken", "{0} RESISTS THE WEAKENING OF {1}"), Name, What);
	}
	return FText::Format(NSLOCTEXT("GOTA", "ResistReduced", "{0} SHRUGS OFF PART OF {1}"), Name, What);
}

void UGOTACombatFeedbackSubsystem::HandleStatusResisted(AActor* Target, EGOTAStatus Status, int32 Aspects, AActor* Instigator)
{
	ULocalPlayer* LP = GetLocalPlayer();
	UWorld* World = LP ? LP->GetWorld() : nullptr;
	APlayerController* PC = LP ? LP->GetPlayerController(World) : nullptr;
	// Only the player who applied the status is told (four players would otherwise see each other's messages).
	if (!PC || !Instigator || PC->GetPawn() != Instigator || (Target && Target->GetWorld() != World))
	{
		return;
	}
	ShowMessage(DescribeResist(Target, Status, Aspects), FLinearColor(0.62f, 0.8f, 1.f, 1.f));
}
