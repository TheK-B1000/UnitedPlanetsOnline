// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GOTADuelResultsWidget.generated.h"

class UTextBlock;
class UGOTAMenuButtonWidget;

UCLASS(Blueprintable)
class WAROFGODS_API UGOTADuelResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Focus REMATCH for keyboard / gamepad (called after the HUD sets the UI input mode). */
	UFUNCTION(BlueprintCallable, Category = "Duel")
	void FocusDefault();

protected:
	UFUNCTION()
	void HandleRematch();

	UFUNCTION()
	void HandleReturnToLobby();

	void Refresh();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_Outcome;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_Summary;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<class UImage> Image_Hero;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_HeroName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Rematch;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_ReturnToLobby;
};
