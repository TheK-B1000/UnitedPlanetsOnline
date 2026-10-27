// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTAMainMenuWidget.generated.h"

class UGOTAMenuButtonWidget;

/**
 * C++ logic for WBP_GOTA_MainMenu. Buttons perform real actions or stay disabled.
 * Continue is hidden/disabled when no campaign save exists.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAMainMenuWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAMainMenuWidget();

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RefreshContinueAvailability();

protected:
	UFUNCTION()
	void HandleContinue();
	UFUNCTION()
	void HandleNewStory();
	UFUNCTION()
	void HandlePvEArena();
	UFUNCTION()
	void HandleMultiplayer();
	UFUNCTION()
	void HandleSettings();
	UFUNCTION()
	void HandleExtras();
	UFUNCTION()
	void HandleQuit();

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
};
