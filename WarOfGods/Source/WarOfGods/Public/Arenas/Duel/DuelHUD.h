// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Arenas/Duel/DuelTypes.h"
#include "DuelHUD.generated.h"

class UUserWidget;

UCLASS()
class WAROFGODS_API ADuelHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|UI")
	TSoftClassPtr<UUserWidget> HudWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|UI")
	TSoftClassPtr<UUserWidget> ResultsWidgetClass;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleMatchOutcome(EDuelMatchOutcome Outcome);

	UFUNCTION()
	void HandlePhaseChanged(EDuelPhase NewPhase);

	void ShowResults();
	void HideResults();

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveHud;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveResults;
};
