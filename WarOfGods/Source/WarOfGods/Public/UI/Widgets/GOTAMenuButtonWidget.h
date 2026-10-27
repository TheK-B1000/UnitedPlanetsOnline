// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "GOTAMenuButtonWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGOTAMenuButtonActivated);

/** Shared cinematic menu button: label, disabled state, focus/hover hooks for WBP styling. */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAMenuButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetLabel(FText InLabel);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetButtonEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool IsSelected() const { return bIsSelected; }

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Menu")
	FOnGOTAMenuButtonActivated OnActivated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	FText Label = NSLOCTEXT("GOTA", "MenuButton", "BUTTON");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	bool bStartEnabled = true;

protected:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UButton> Button_Root;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_Label;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Menu")
	bool bIsSelected = false;
};
