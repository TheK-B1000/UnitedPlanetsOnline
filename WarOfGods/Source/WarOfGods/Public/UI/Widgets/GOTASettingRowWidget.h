// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GOTASettingRowWidget.generated.h"

class UButton;
class UTextBlock;
class USlider;
class UBorder;

/**
 * WBP_GOTA_SettingRow — one setting. The whole row is a single focus stop; Left/Right
 * (keys, D-pad, stick) change the value so gamepad never fights the layout. Mouse uses the
 * arrows or slider. The row owns no data: it is bound to getter/setter callbacks.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTASettingRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

	void SetupChoice(const FText& InLabel, const TArray<FText>& InOptions, TFunction<int32()> InGet, TFunction<void(int32)> InSet);
	void SetupSlider(const FText& InLabel, float InMin, float InMax, float InStep,
		TFunction<float()> InGet, TFunction<void(float)> InSet, TFunction<FText(float)> InFormat);

	/** Pull the current value from the getter and redraw. */
	void Refresh();
	void StepValue(int32 Direction);
	void FocusRow();

	/** Fired after any user change so the screen can mark pending edits. */
	TFunction<void()> OnEdited;

protected:
	UFUNCTION()
	void HandlePrev();
	UFUNCTION()
	void HandleNext();
	UFUNCTION()
	void HandleSlider(float Value);
	void ApplyFocusVisual();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UButton> Button_Row;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UTextBlock> Text_Label;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UTextBlock> Text_Value;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UButton> Btn_Prev;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UButton> Btn_Next;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<USlider> Slider_Value;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UBorder> Border_Frame;

	bool bIsSlider = false;
	bool bFocused = false;
	bool bSuppressSlider = false;
	TArray<FText> Options;
	TFunction<int32()> GetChoice;
	TFunction<void(int32)> SetChoice;
	float Min = 0.f;
	float Max = 1.f;
	float Step = 0.05f;
	TFunction<float()> GetValue;
	TFunction<void(float)> SetValue;
	TFunction<FText(float)> FormatValue;
};
