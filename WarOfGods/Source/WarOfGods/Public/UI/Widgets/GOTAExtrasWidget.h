// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTAExtrasWidget.generated.h"

class UGOTAMenuButtonWidget;
class UPanelWidget;
class UTextBlock;
class UScrollBox;

/** WBP_GOTA_Extras — UPG codex, controls guide, credits and licenses from DA_GOTA_MenuContent. */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAExtrasWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAExtrasWidget();
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GOTA|Extras")
	TSubclassOf<UGOTAMenuButtonWidget> EntryClass;

protected:
	virtual UGOTAMenuButtonWidget* GetDefaultFocusButton() const override;
	virtual void OnScreenActivated() override;
	void RebuildEntries();
	void ShowEntry(int32 Index);

	UFUNCTION()
	void HandleEntryFocused(UGOTAMenuButtonWidget* Button);
	UFUNCTION()
	void HandleBack();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Extras")
	TObjectPtr<UPanelWidget> Box_Entries;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Extras")
	TObjectPtr<UTextBlock> Text_EntryCategory;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Extras")
	TObjectPtr<UTextBlock> Text_EntryTitle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Extras")
	TObjectPtr<UTextBlock> Text_EntryBody;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Extras")
	TObjectPtr<UScrollBox> Scroll_Body;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Extras")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;

	UPROPERTY()
	TArray<TObjectPtr<UGOTAMenuButtonWidget>> Entries;
};
