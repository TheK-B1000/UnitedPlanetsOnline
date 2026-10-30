// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTAStoryMenuWidget.generated.h"

class UGOTAMenuButtonWidget;
class UPanelWidget;
class UTextBlock;

/**
 * WBP_GOTA_StoryMode — campaign hub. Copy + chapters come from DA_GOTA_MenuContent.
 * Only chapters flagged playable (real map + GameMode) can launch; others show LOCKED.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAStoryMenuWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAStoryMenuWidget();
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GOTA|Story")
	TSubclassOf<UGOTAMenuButtonWidget> EntryClass;

protected:
	virtual UGOTAMenuButtonWidget* GetDefaultFocusButton() const override;
	virtual void OnScreenActivated() override;
	void RebuildChapters();

	UFUNCTION()
	void HandleNewGameSolo();
	UFUNCTION()
	void HandleNewGameCoop();
	UFUNCTION()
	void HandleJoinCoop();
	UFUNCTION()
	void HandleContinue();
	UFUNCTION()
	void HandleBack();
	UFUNCTION()
	void HandleChapterActivated();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Story")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_NewGameSolo;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Story")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_NewGameCoop;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Story")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_JoinCoop;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Story")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Continue;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Story")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Story")
	TObjectPtr<UTextBlock> Text_CampaignTitle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Story")
	TObjectPtr<UTextBlock> Text_Synopsis;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Story")
	TObjectPtr<UPanelWidget> Box_Chapters;
};
