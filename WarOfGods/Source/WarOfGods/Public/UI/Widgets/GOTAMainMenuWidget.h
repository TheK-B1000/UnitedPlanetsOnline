// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTAMainMenuWidget.generated.h"

class UGOTAMenuButtonWidget;
class UTextBlock;
class UProgressBar;

/**
 * WBP_GOTA_MainMenu — UPG reference layout.
 * Left nav rows and bottom cards both carry an EGOTAMenuAction and route through
 * UGOTAMenuSubsystem::ExecuteMenuAction (one code path). Profile shows only real data.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAMainMenuWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAMainMenuWidget();

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RefreshContinueAvailability();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RefreshProfile();

protected:
	virtual UGOTAMenuButtonWidget* GetDefaultFocusButton() const override;
	virtual void OnScreenActivated() override;

	void ConfigureAction(UGOTAMenuButtonWidget* Button, EGOTAMenuAction Action, bool bCard);
	void AdvanceInstitutionalMessage();

	// --- Region C: navigation ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Continue;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_NewStory;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_PvEArena;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Multiplayer;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Settings;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Extras;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Quit;

	// --- Region F: feature cards ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Card_Story;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Card_Arena;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Card_Multiplayer;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Card_Settings;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Card_Extras;

	// --- Region D: profile ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_ProfileName;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_ProfileRank;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UProgressBar> Bar_ProfileProgress;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Friends;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Plus;

	// --- Regions B / E: identity copy from content ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_GameSubtitle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_Slogan;

	FTimerHandle MessageTimer;
	int32 MessageIndex = 0;
};
