// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Arenas/Duel/DuelTypes.h"
#include "GOTADuelHUDWidget.generated.h"

class ADuelGameState;
class UTextBlock;
class UProgressBar;

UCLASS(Blueprintable)
class WAROFGODS_API UGOTADuelHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UFUNCTION()
	void HandlePhaseChanged(EDuelPhase NewPhase);

	UFUNCTION()
	void HandleScoreChanged(int32 Score0, int32 Score1, int32 RoundIndex);

	void RefreshFromGameState();
	ADuelGameState* GetDuelGS() const;
	APawn* ResolveOpponentPawn(APlayerState*& OutOpponentPS) const;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_Round;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_Score;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_Phase;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_Countdown;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_Timer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_LocalName;

	/** Accepted hero portrait (resolved locally from the replicated HeroId). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<class UImage> Image_LocalHero;

	FName ShownHeroId;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UTextBlock> Text_OpponentName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UProgressBar> Bar_LocalHealth;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Duel")
	TObjectPtr<UProgressBar> Bar_OpponentHealth;
};
