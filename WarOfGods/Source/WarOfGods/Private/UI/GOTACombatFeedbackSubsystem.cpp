// White Tiger Studios 2024

#include "UI/GOTACombatFeedbackSubsystem.h"

#include "Abilities/GAS/GOTAStatusEffects.h"
#include "Blueprint/WidgetTree.h"
#include "Bosses/GOTABossComponent.h"
#include "Bosses/GOTABossTypes.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTACombatFeedbackSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAFeedback, Log, All);

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
}

// ============================================================================ subsystem

void UGOTACombatFeedbackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResistedHandle = GOTAGAS::OnStatusResisted.AddUObject(this, &UGOTACombatFeedbackSubsystem::HandleStatusResisted);
}

void UGOTACombatFeedbackSubsystem::Deinitialize()
{
	GOTAGAS::OnStatusResisted.Remove(ResistedHandle);
	Super::Deinitialize();
}

void UGOTACombatFeedbackSubsystem::ShowMessage(const FText& Text, const FLinearColor& Color, float Seconds)
{
	ULocalPlayer* LP = GetLocalPlayer();
	UWorld* World = LP ? LP->GetWorld() : nullptr;
	APlayerController* PC = LP ? LP->GetPlayerController(World) : nullptr;
	if (!PC)
	{
		return;
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
	if (Widget)
	{
		Widget->Show(Text, Color, Seconds);
	}
	UE_LOG(LogGOTAFeedback, Log, TEXT("FEEDBACK %s"), *Text.ToString());
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
