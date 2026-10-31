// White Tiger Studios 2024

#include "UI/Widgets/GOTAMenuButtonWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GOTAMenuContentDefinition.h"
#include "UI/GOTAMenuSubsystem.h"

namespace GOTAButtonVisuals
{
	static FGOTAMenuTheme ResolveTheme(const UGOTAMenuSubsystem* Subsystem)
	{
		if (Subsystem)
		{
			if (const UGOTAMenuContentDefinition* Content = Subsystem->GetMenuContent())
			{
				return Content->Theme;
			}
		}
		return FGOTAMenuTheme();
	}

	static FLinearColor WithAlpha(FLinearColor C, float A)
	{
		C.A = A;
		return C;
	}
}

UGOTAMenuSubsystem* UGOTAMenuButtonWidget::GetMenuSubsystem() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UGOTAMenuSubsystem>() : nullptr;
}

void UGOTAMenuButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Button_Root)
	{
		Button_Root->OnClicked.AddUniqueDynamic(this, &UGOTAMenuButtonWidget::HandleClicked);
		Button_Root->OnPressed.AddUniqueDynamic(this, &UGOTAMenuButtonWidget::HandlePressed);
		Button_Root->OnHovered.AddUniqueDynamic(this, &UGOTAMenuButtonWidget::HandleHovered);
		Button_Root->OnUnhovered.AddUniqueDynamic(this, &UGOTAMenuButtonWidget::HandleUnhovered);
	}
	SetButtonEnabled(bStartEnabled);
}

void UGOTAMenuButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	// Designer preview + runtime: show authored copy until content overrides it.
	if (Text_Label)
	{
		Text_Label->SetText(Label);
	}
	if (Text_Subtitle)
	{
		Text_Subtitle->SetText(Subtitle);
		Text_Subtitle->SetVisibility(Subtitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (IconOverride)
	{
		SetIcon(IconOverride);
	}
	ApplyVisualState(0.f, true);
}

void UGOTAMenuButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (MenuAction != EGOTAMenuAction::None)
	{
		ApplyDestinationContent();
	}
	ApplyVisualState(0.f, true);
}

void UGOTAMenuButtonWidget::ApplyDestinationContent()
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	const UGOTAMenuContentDefinition* Content = Subsystem ? Subsystem->GetMenuContent() : nullptr;
	FGOTAMenuDestination Destination;
	if (!Content || !Content->FindDestination(MenuAction, Destination))
	{
		return;
	}
	// Feature cards keep their authored short title (STORY, ARENA…); nav rows take the full label.
	if (!Destination.Label.IsEmpty() && !bUseCardContent)
	{
		SetLabel(Destination.Label);
	}
	SetSubtitle(bUseCardContent ? Destination.CardSubtitle : Destination.Subtitle);
	if (!Destination.Tooltip.IsEmpty())
	{
		SetToolTipText(Destination.Tooltip);
	}
	if (UTexture2D* Icon = Destination.Icon.LoadSynchronous())
	{
		SetIcon(Icon);
	}
	if (bUseCardContent)
	{
		if (UTexture2D* Art = Destination.CardArt.LoadSynchronous())
		{
			SetArt(Art);
		}
	}
}

void UGOTAMenuButtonWidget::SetLabel(FText InLabel)
{
	Label = MoveTemp(InLabel);
	if (Text_Label)
	{
		Text_Label->SetText(Label);
	}
}

void UGOTAMenuButtonWidget::SetSubtitle(FText InSubtitle)
{
	Subtitle = MoveTemp(InSubtitle);
	if (Text_Subtitle)
	{
		Text_Subtitle->SetText(Subtitle);
		Text_Subtitle->SetVisibility(Subtitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UGOTAMenuButtonWidget::SetIcon(UTexture2D* InIcon)
{
	if (Image_Icon && InIcon)
	{
		Image_Icon->SetBrushFromTexture(InIcon, false);
	}
}

void UGOTAMenuButtonWidget::SetArt(UTexture2D* InArt)
{
	if (Image_Art && InArt)
	{
		Image_Art->SetBrushFromTexture(InArt, false);
	}
}

void UGOTAMenuButtonWidget::SetButtonEnabled(bool bEnabled)
{
	bButtonEnabled = bEnabled;
	bStartEnabled = bEnabled;
	if (Button_Root)
	{
		// Disabled buttons cannot take focus, so gamepad navigation skips them.
		Button_Root->SetIsEnabled(bEnabled);
	}
	ApplyVisualState(0.f, true);
}

void UGOTAMenuButtonWidget::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	ApplyVisualState(0.f, false);
}

void UGOTAMenuButtonWidget::FocusButton()
{
	if (!Button_Root || !bButtonEnabled)
	{
		return;
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		Button_Root->SetUserFocus(PC);
	}
	else
	{
		Button_Root->SetKeyboardFocus();
	}
}

void UGOTAMenuButtonWidget::HandleClicked()
{
	if (!bButtonEnabled)
	{
		return;
	}
	OnActivated.Broadcast();
	if (MenuAction != EGOTAMenuAction::None)
	{
		if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
		{
			Subsystem->ExecuteMenuAction(MenuAction);
		}
	}
}

void UGOTAMenuButtonWidget::HandlePressed()
{
	PressAlpha = 1.f;
}

void UGOTAMenuButtonWidget::HandleHovered()
{
	bHovered = true;
}

void UGOTAMenuButtonWidget::HandleUnhovered()
{
	bHovered = false;
}

void UGOTAMenuButtonWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	if (bHasFocus)
	{
		return;
	}
	bHasFocus = true;
	OnFocused.Broadcast(this);

	const UWorld* World = GetWorld();
	const float Now = World ? World->GetRealTimeSeconds() : 0.f;
	// Rate-limit so fast stick scrolling doesn't machine-gun the cue.
	if (Now - LastFocusSoundTime > 0.06f && InFocusEvent.GetCause() != EFocusCause::SetDirectly)
	{
		LastFocusSoundTime = Now;
		if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
		{
			Subsystem->PlayUISound(EGOTAMenuSound::Focus);
		}
	}
}

void UGOTAMenuButtonWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bHasFocus = false;
}

void UGOTAMenuButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ApplyVisualState(InDeltaTime, false);
}

void UGOTAMenuButtonWidget::ApplyVisualState(float DeltaTime, bool bSnap)
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	const FGOTAMenuTheme Theme = GOTAButtonVisuals::ResolveTheme(Subsystem);
	const bool bReduced = Subsystem && Subsystem->IsReducedMotion();

	// Focus = full gold treatment; selected = strong; hover = soft. All three read differently.
	float Target = 0.f;
	if (bButtonEnabled)
	{
		Target = bHasFocus ? 1.f : (bIsSelected ? 0.75f : (bHovered ? 0.45f : 0.f));
	}

	const float Rate = 1.f / FMath::Max(0.01f, Theme.FocusBlendSeconds);
	const float Previous = HighlightAlpha;
	HighlightAlpha = (bSnap || bReduced) ? Target : FMath::FInterpConstantTo(HighlightAlpha, Target, DeltaTime, Rate);
	const float PreviousPress = PressAlpha;
	PressAlpha = (bSnap || bReduced) ? 0.f : FMath::FInterpConstantTo(PressAlpha, 0.f, DeltaTime, 6.f);

	if (!bSnap && FMath::IsNearlyEqual(Previous, HighlightAlpha) && FMath::IsNearlyEqual(PreviousPress, PressAlpha) && DeltaTime > 0.f)
	{
		return; // settled — nothing to push to Slate this frame
	}

	const float H = HighlightAlpha;
	const bool bFocusLook = bHasFocus && bButtonEnabled;

	FLinearColor LabelColor = bButtonEnabled ? FLinearColor::LerpUsingHSV(Theme.Text, Theme.GoldBright, bFocusLook ? 1.f : H * 0.35f) : Theme.TextDisabled;
	FLinearColor SubColor = bButtonEnabled ? FLinearColor::LerpUsingHSV(Theme.TextMuted, Theme.Text, H * 0.5f) : Theme.TextDisabled;
	FLinearColor IconColor = bButtonEnabled ? FLinearColor::LerpUsingHSV(Theme.Text, Theme.GoldBright, H) : Theme.TextDisabled;

	if (Text_Label)
	{
		Text_Label->SetColorAndOpacity(FSlateColor(LabelColor));
	}
	if (Text_Subtitle)
	{
		Text_Subtitle->SetColorAndOpacity(FSlateColor(SubColor));
	}
	if (Image_Icon)
	{
		Image_Icon->SetColorAndOpacity(IconColor);
	}

	float FrameAlpha = 0.f;
	float SweepAlpha = 0.f;
	switch (Style)
	{
	case EGOTAButtonStyle::NavRow:
		FrameAlpha = H;
		SweepAlpha = H * 0.55f;
		break;
	case EGOTAButtonStyle::FeatureCard:
		FrameAlpha = 0.35f + 0.65f * H;
		SweepAlpha = H * 0.35f;
		break;
	case EGOTAButtonStyle::Tab:
		FrameAlpha = bIsSelected ? 1.f : H * 0.8f;
		SweepAlpha = (bIsSelected ? 0.35f : 0.f) + H * 0.25f;
		break;
	default:
		FrameAlpha = 0.3f + 0.7f * H;
		SweepAlpha = H * 0.4f;
		break;
	}
	if (!bButtonEnabled)
	{
		FrameAlpha *= 0.4f;
		SweepAlpha = 0.f;
	}

	if (Border_Frame)
	{
		Border_Frame->SetBrushColor(GOTAButtonVisuals::WithAlpha(Theme.Gold, FrameAlpha));
	}
	if (Image_Selection)
	{
		Image_Selection->SetColorAndOpacity(GOTAButtonVisuals::WithAlpha(Theme.Gold, SweepAlpha));
	}
	if (Image_Art)
	{
		const float Bright = bButtonEnabled ? (0.72f + 0.28f * H) : 0.35f;
		Image_Art->SetColorAndOpacity(FLinearColor(Bright, Bright, Bright, 1.f));
	}

	// Card lift + press squash. Reduced motion keeps the color feedback only.
	if (Style == EGOTAButtonStyle::FeatureCard && !bReduced)
	{
		FWidgetTransform Xf;
		Xf.Translation = FVector2D(0.f, -Theme.CardLift * H);
		const float Scale = 1.f + 0.025f * H - 0.02f * PressAlpha;
		Xf.Scale = FVector2D(Scale, Scale);
		SetRenderTransform(Xf);
	}
	else if (!bReduced)
	{
		const float Scale = 1.f - 0.015f * PressAlpha;
		SetRenderScale(FVector2D(Scale, Scale));
	}
}
