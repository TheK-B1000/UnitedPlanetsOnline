// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTASettingsWidget.generated.h"

class UGOTAMenuSettingsAdapter;
class UGOTAMenuButtonWidget;
class UGOTASettingRowWidget;
class UPanelWidget;
class UTextBlock;

UENUM(BlueprintType)
enum class EGOTASettingsTab : uint8
{
	Video,
	Audio,
	Controls,
	Accessibility
};

/**
 * WBP_GOTA_Settings — tabs (LB/RB on gamepad) of rows that each map to a real effect.
 * Edits are pending until Apply; Revert reloads from disk; Back discards pending edits.
 * Risky display changes ask for confirmation and auto-revert (modal layer).
 * Not offered (no backing system yet): key rebinding, dialogue volume, subtitles, VFX intensity.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTASettingsWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTASettingsWidget();
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION(BlueprintPure, Category = "GOTA|Settings")
	UGOTAMenuSettingsAdapter* GetAdapter() const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SelectTab(EGOTASettingsTab Tab);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GOTA|Settings")
	TSubclassOf<UGOTASettingRowWidget> RowClass;

protected:
	virtual UGOTAMenuButtonWidget* GetDefaultFocusButton() const override;
	virtual void OnScreenActivated() override;
	/** Any exit (Back button, Esc, gamepad B) discards unapplied edits. */
	virtual void HideScreen(bool bAnimate = true) override;

	void RebuildRows();
	void RefreshRows();
	void MarkPending(bool bPending);
	UGOTASettingRowWidget* AddRow();

	UFUNCTION()
	void HandleApply();
	UFUNCTION()
	void HandleCancel();
	UFUNCTION()
	void HandleDefaults();
	UFUNCTION()
	void HandleBack();
	UFUNCTION()
	void HandleTabVideo();
	UFUNCTION()
	void HandleTabAudio();
	UFUNCTION()
	void HandleTabControls();
	UFUNCTION()
	void HandleTabAccessibility();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Tab_Video;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Tab_Audio;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Tab_Controls;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Tab_Accessibility;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UPanelWidget> Box_Rows;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UTextBlock> Text_Pending;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UTextBlock> Text_TabNote;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Apply;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Cancel;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Defaults;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;

	UPROPERTY()
	TArray<TObjectPtr<UGOTASettingRowWidget>> Rows;

	EGOTASettingsTab ActiveTab = EGOTASettingsTab::Video;
	bool bPendingChanges = false;
	TArray<FIntPoint> CachedResolutions;
};
