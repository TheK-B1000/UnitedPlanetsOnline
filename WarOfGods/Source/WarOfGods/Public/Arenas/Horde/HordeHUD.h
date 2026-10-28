// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Arenas/Horde/HordeTypes.h"
#include "HordeHUD.generated.h"

class UUserWidget;

/** Local HUD bootstrap for Wrath of the Gods — creates HUD widget on each client. */
UCLASS()
class WAROFGODS_API AHordeHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|UI")
	TSoftClassPtr<UUserWidget> HudWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|UI")
	TSoftClassPtr<UUserWidget> ResultsWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Horde|UI")
	void ShowResults();

protected:
	UFUNCTION()
	void HandleMatchOutcome(EHordeMatchOutcome Outcome);

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveHud;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveResults;
};
