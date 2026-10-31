// White Tiger Studios 2024

#include "UI/Widgets/GOTASettingRowWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "UI/GOTAMenuContentDefinition.h"
#include "UI/GOTAMenuSubsystem.h"

void UGOTASettingRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Arrows + slider are authored non-focusable in WBP_GOTA_SettingRow (row is the single focus stop).
	if (Btn_Prev)
	{
		Btn_Prev->OnClicked.AddUniqueDynamic(this, &UGOTASettingRowWidget::HandlePrev);
	}
	if (Btn_Next)
	{
		Btn_Next->OnClicked.AddUniqueDynamic(this, &UGOTASettingRowWidget::HandleNext);
	}
	if (Slider_Value)
	{
		Slider_Value->OnValueChanged.AddUniqueDynamic(this, &UGOTASettingRowWidget::HandleSlider);
	}
	ApplyFocusVisual();
}

void UGOTASettingRowWidget::SetupChoice(const FText& InLabel, const TArray<FText>& InOptions, TFunction<int32()> InGet, TFunction<void(int32)> InSet)
{
	bIsSlider = false;
	Options = InOptions;
	GetChoice = MoveTemp(InGet);
	SetChoice = MoveTemp(InSet);
	if (Text_Label) { Text_Label->SetText(InLabel); }
	if (Slider_Value) { Slider_Value->SetVisibility(ESlateVisibility::Collapsed); }
	Refresh();
}

void UGOTASettingRowWidget::SetupSlider(const FText& InLabel, float InMin, float InMax, float InStep,
	TFunction<float()> InGet, TFunction<void(float)> InSet, TFunction<FText(float)> InFormat)
{
	bIsSlider = true;
	Min = InMin;
	Max = FMath::Max(InMax, InMin + KINDA_SMALL_NUMBER);
	Step = FMath::Max(InStep, KINDA_SMALL_NUMBER);
	GetValue = MoveTemp(InGet);
	SetValue = MoveTemp(InSet);
	FormatValue = MoveTemp(InFormat);
	if (Text_Label) { Text_Label->SetText(InLabel); }
	if (Slider_Value)
	{
		Slider_Value->SetVisibility(ESlateVisibility::Visible);
		Slider_Value->SetMinValue(Min);
		Slider_Value->SetMaxValue(Max);
		Slider_Value->SetStepSize(Step);
	}
	Refresh();
}

void UGOTASettingRowWidget::Refresh()
{
	if (bIsSlider)
	{
		const float V = GetValue ? GetValue() : Min;
		if (Text_Value) { Text_Value->SetText(FormatValue ? FormatValue(V) : FText::AsNumber(V)); }
		if (Slider_Value)
		{
			TGuardValue<bool> Guard(bSuppressSlider, true);
			Slider_Value->SetValue(V);
		}
	}
	else if (Text_Value)
	{
		const int32 Index = GetChoice ? GetChoice() : 0;
		Text_Value->SetText(Options.IsValidIndex(Index) ? Options[Index] : NSLOCTEXT("GOTA", "Custom", "CUSTOM"));
	}
}

void UGOTASettingRowWidget::StepValue(int32 Direction)
{
	if (bIsSlider)
	{
		if (!GetValue || !SetValue)
		{
			return;
		}
		const float Next = FMath::Clamp(FMath::GridSnap(GetValue() + Direction * Step, Step), Min, Max);
		SetValue(Next);
	}
	else
	{
		if (!GetChoice || !SetChoice || Options.Num() == 0)
		{
			return;
		}
		const int32 Current = FMath::Clamp(GetChoice(), 0, Options.Num() - 1);
		SetChoice((Current + Direction + Options.Num()) % Options.Num());
	}
	Refresh();
	if (OnEdited)
	{
		OnEdited();
	}
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (UGOTAMenuSubsystem* Menu = GI->GetSubsystem<UGOTAMenuSubsystem>())
		{
			Menu->PlayUISound(EGOTAMenuSound::Focus);
		}
	}
}

void UGOTASettingRowWidget::HandlePrev()
{
	StepValue(-1);
}

void UGOTASettingRowWidget::HandleNext()
{
	StepValue(1);
}

void UGOTASettingRowWidget::HandleSlider(float Value)
{
	if (bSuppressSlider || !SetValue)
	{
		return;
	}
	SetValue(FMath::Clamp(FMath::GridSnap(Value, Step), Min, Max));
	Refresh();
	if (OnEdited)
	{
		OnEdited();
	}
}

FReply UGOTASettingRowWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
	{
		StepValue(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
	{
		StepValue(1);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGOTASettingRowWidget::FocusRow()
{
	if (Button_Row)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			Button_Row->SetUserFocus(PC);
		}
		else
		{
			Button_Row->SetKeyboardFocus();
		}
	}
}

void UGOTASettingRowWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bFocused = true;
	ApplyFocusVisual();
}

void UGOTASettingRowWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bFocused = false;
	ApplyFocusVisual();
}

void UGOTASettingRowWidget::ApplyFocusVisual()
{
	FGOTAMenuTheme Theme;
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UGOTAMenuSubsystem* Menu = GI->GetSubsystem<UGOTAMenuSubsystem>())
		{
			if (const UGOTAMenuContentDefinition* Content = Menu->GetMenuContent())
			{
				Theme = Content->Theme;
			}
		}
	}
	if (Border_Frame)
	{
		FLinearColor Frame = Theme.Gold;
		Frame.A = bFocused ? 0.9f : 0.12f;
		Border_Frame->SetBrushColor(Frame);
	}
	if (Text_Label)
	{
		Text_Label->SetColorAndOpacity(FSlateColor(bFocused ? Theme.GoldBright : Theme.Text));
	}
	if (Text_Value)
	{
		Text_Value->SetColorAndOpacity(FSlateColor(bFocused ? Theme.Text : Theme.TextMuted));
	}
}
