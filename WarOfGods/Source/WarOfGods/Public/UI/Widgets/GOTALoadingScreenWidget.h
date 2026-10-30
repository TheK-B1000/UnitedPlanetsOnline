// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GOTALoadingScreenWidget.generated.h"

class UImage;
class UTextBlock;
class UGOTAHeroDefinition;
class UTexture2D;

/**
 * WBP_GOTA_LoadingScreen — shown by the MoviePlayer during blocking map loads (it renders on the
 * Slate loading thread, so it is configured once before travel and holds no tick logic).
 * No fake percentage: the indicator only states that deployment is in progress.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTALoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Hero spotlight: portrait, name, title, two abilities, a tip. */
	void ConfigureHero(const UGOTAHeroDefinition& Hero, const FText& ArenaName, const FText& ModeName, const FText& Tip);

	/** Arena spotlight (no hero chosen). */
	void ConfigureArena(UTexture2D* Art, const FText& ArenaName, const FText& Description, const FText& ModeName, const FText& Tip);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Loading") TObjectPtr<UImage> Image_Art;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Loading") TObjectPtr<UTextBlock> Text_Kicker;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Loading") TObjectPtr<UTextBlock> Text_Title;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Loading") TObjectPtr<UTextBlock> Text_Subtitle;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Loading") TObjectPtr<UTextBlock> Text_Body;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Loading") TObjectPtr<UTextBlock> Text_Tip;
};
