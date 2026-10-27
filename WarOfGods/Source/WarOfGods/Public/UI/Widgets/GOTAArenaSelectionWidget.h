// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "UI/MenuTypes.h"
#include "Arenas/ArenaTypes.h"
#include "GOTAArenaSelectionWidget.generated.h"

class UGOTAMenuButtonWidget;

UCLASS(Blueprintable)
class WAROFGODS_API UGOTAArenaSelectionWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAArenaSelectionWidget();

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Arena")
	void RefreshArenaCards();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Arena")
	void SelectArena(EArenaId ArenaId);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Arena")
	void ConfirmSelection();

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Arena")
	void OnArenaCardsUpdated(const TArray<FGOTAArenaCard>& Cards);

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Arena")
	void OnArenaHighlighted(EArenaId ArenaId);

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Arena")
	void RequestTravelToSelectedArena();

protected:
	UFUNCTION()
	void HandleConfirm();
	UFUNCTION()
	void HandleBack();

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Arena")
	TArray<FGOTAArenaCard> ArenaCards;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Arena")
	EArenaId HighlightedArena = EArenaId::TitansQuarry;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Confirm;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;
};
