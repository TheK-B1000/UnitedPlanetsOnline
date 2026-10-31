// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Arenas/Horde/HordeTypes.h"
#include "GOTAHordeHUDWidget.generated.h"

class AHordeGameState;
class UTextBlock;
class UProgressBar;

UCLASS(Blueprintable)
class WAROFGODS_API UGOTAHordeHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UFUNCTION()
	void HandlePhaseChanged(EHordeMatchPhase NewPhase);

	UFUNCTION()
	void HandleWaveStatus(int32 WaveIndex, int32 TotalWaves, int32 EnemiesRemaining);

	UFUNCTION()
	void HandleScoreChanged(int32 TeamScore);

	void RefreshFromGameState();
	void EnsureRuntimeLayout();
	AHordeGameState* GetHordeGS() const;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UTextBlock> Text_Wave;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UTextBlock> Text_Phase;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UTextBlock> Text_Enemies;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UTextBlock> Text_Score;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UTextBlock> Text_Countdown;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UProgressBar> Bar_BossHealth;
};
