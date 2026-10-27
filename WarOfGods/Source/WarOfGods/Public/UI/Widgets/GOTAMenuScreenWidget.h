// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuScreenWidget.generated.h"

class UGOTAMenuSubsystem;

/** Base for frontend screens — binds to UGOTAMenuSubsystem on construct. */
UCLASS(Abstract, Blueprintable)
class WAROFGODS_API UGOTAMenuScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UGOTAMenuSubsystem* GetMenuSubsystem() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Menu")
	void OnMenuScreenChanged(EGOTAMenuScreen NewScreen, EGOTAMenuScreen PreviousScreen);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	virtual void ShowScreen(bool bAnimate = true);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	virtual void HideScreen(bool bAnimate = true);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GOTA|Menu")
	EGOTAMenuScreen ScreenId = EGOTAMenuScreen::MainMenu;

protected:
	UFUNCTION()
	void HandleMenuScreenChanged(EGOTAMenuScreen NewScreen, EGOTAMenuScreen PreviousScreen);

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuSubsystem> MenuSubsystem;
};
