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
	UFUNCTION()
	void HandleMatchOutcome(EDuelMatchOutcome Outcome);

	void ShowResults();

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveHud;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveResults;
};
