// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "UI/MenuTypes.h"
#include "GOTAArenaSelectionWidget.generated.h"

class UGOTAMenuButtonWidget;
class UPanelWidget;
class UTextBlock;
class UImage;

/**
 * WBP_GOTA_ArenaSelection — Arena Mode.
 * Pick a ruleset (Trial of Champions: Fight AI / Wrath of the Gods: Horde) and an arena.
 * All seven arenas are browsable; launch is enabled only where the subsystem has validated
 * the ruleset on that arena. Launch travels via UGOTAMenuSubsystem::RequestLaunchArena.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAArenaSelectionWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAArenaSelectionWidget();

	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Arena")
	void SelectRuleset(EGOTAArenaRuleset Ruleset);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Arena")
	void SelectArena(EArenaId ArenaId);

	UFUNCTION(BlueprintPure, Category = "GOTA|Arena")
	EGOTAArenaRuleset GetSelectedRuleset() const { return SelectedRuleset; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Arena")
	EArenaId GetSelectedArena() const { return SelectedArena; }

	/** Row class spawned into Box_Arenas (WBP_GOTA_ListEntry). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GOTA|Arena")
	TSubclassOf<UGOTAMenuButtonWidget> EntryClass;

protected:
	virtual UGOTAMenuButtonWidget* GetDefaultFocusButton() const override;
	virtual void OnScreenActivated() override;

	void RebuildArenaList();
	void RefreshDetails();

	UFUNCTION()
	void HandleFightAI();
	UFUNCTION()
	void HandleHorde();
	UFUNCTION()
	void HandleArenaFocused(UGOTAMenuButtonWidget* Button);
	UFUNCTION()
	void HandleConfirm();
	UFUNCTION()
	void HandleBack();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_ModeFightAI;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_ModeHorde;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UPanelWidget> Box_Arenas;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UImage> Image_ArenaPreview;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UTextBlock> Text_ArenaName;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UTextBlock> Text_ArenaDescription;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UTextBlock> Text_ModeTitle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UTextBlock> Text_ModeDescription;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UTextBlock> Text_Availability;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Confirm;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Arena")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Arena")
	TArray<FGOTAArenaCard> Cards;

	UPROPERTY()
	TArray<TObjectPtr<UGOTAMenuButtonWidget>> ArenaEntries;

	EGOTAArenaRuleset SelectedRuleset = EGOTAArenaRuleset::FightAI;
	EArenaId SelectedArena = EArenaId::TitansQuarry;
};
