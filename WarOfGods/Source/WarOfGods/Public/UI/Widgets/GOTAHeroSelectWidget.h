// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTAHeroSelectWidget.generated.h"

class UGOTAMenuButtonWidget;
class UGOTAHeroDefinition;
class UPanelWidget;
class UTextBlock;
class UImage;
class UProgressBar;

/**
 * WBP_GOTA_HeroSelect — the approved hero-card layout as a live screen:
 * one large portrait (left) with name / title / quote / faction, profile panel (right) with
 * card stats, biography, divine abilities and weapon style, and a roster carousel (bottom).
 * Focusing a tile previews; CONFIRM is enabled only when UGOTAHeroRegistry validates the hero.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAHeroSelectWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAHeroSelectWidget();
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GOTA|Heroes")
	TSubclassOf<UGOTAMenuButtonWidget> TileClass;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Heroes")
	void ShowHero(FName HeroId);

protected:
	virtual UGOTAMenuButtonWidget* GetDefaultFocusButton() const override;
	virtual void OnScreenActivated() override;
	void RebuildRoster();
	void SetStat(UProgressBar* Bar, UTextBlock* Value, int32 Stat);

	UFUNCTION()
	void HandleTileFocused(UGOTAMenuButtonWidget* Tile);
	UFUNCTION()
	void HandleTileActivated();
	UFUNCTION()
	void HandleConfirm();
	UFUNCTION()
	void HandleBack();
	UFUNCTION()
	void HandleDifficulty();
	void RefreshDifficultyLabel();

	// Left — single large character illustration + identity
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UImage> Image_Portrait;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_HeroName;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_HeroTitle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Quote;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Faction;

	// Right — profile panel
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_PanelName;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_PanelTitle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UProgressBar> Bar_Attack;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UProgressBar> Bar_Defense;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UProgressBar> Bar_Mobility;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UProgressBar> Bar_Technique;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Attack;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Defense;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Mobility;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Technique;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Biography;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Abilities;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Weapon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UTextBlock> Text_Availability;

	// Bottom — roster carousel + actions
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UPanelWidget> Box_Roster;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UGOTAMenuButtonWidget> Btn_Confirm;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;
	/** Fight AI only: cycles the Sovereign Champion's difficulty. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Heroes") TObjectPtr<UGOTAMenuButtonWidget> Btn_Difficulty;

	UPROPERTY()
	TArray<TObjectPtr<UGOTAMenuButtonWidget>> Tiles;

	TArray<FName> TileHeroIds;
	FName PreviewHero;
};
