// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTAStoryMenuWidget.generated.h"

class UGOTAMenuButtonWidget;

/**
 * Story Mode hub: New Game, Continue, Host Co-op, Join Co-op, Back.
 * Bind in WBP_GOTA_StoryMode.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAStoryMenuWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAStoryMenuWidget();

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Story")
	void RefreshContinueAvailability();

protected:
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
};
