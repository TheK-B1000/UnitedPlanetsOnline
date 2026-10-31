// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuScreenWidget.generated.h"

class UGOTAMenuSubsystem;
class UGOTAMenuButtonWidget;
class UGOTAMenuContentDefinition;

/**
 * Base for frontend screens. Visibility follows UGOTAMenuSubsystem's current screen; only the
 * visible screen is hit-testable, so only it receives input. Handles entrance motion
 * (interruptible, skipped for Reduced Motion) and focus restore.
 */
UCLASS(Abstract, Blueprintable)
class WAROFGODS_API UGOTAMenuScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UGOTAMenuSubsystem* GetMenuSubsystem() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	UGOTAMenuContentDefinition* GetMenuContent() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Menu")
	void OnMenuScreenChanged(EGOTAMenuScreen NewScreen, EGOTAMenuScreen PreviousScreen);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	virtual void ShowScreen(bool bAnimate = true);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	virtual void HideScreen(bool bAnimate = true);

	/** Focus the last focused control on this screen, or its default. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RestoreFocus();

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool IsActiveScreen() const { return bIsActive; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GOTA|Menu")
	EGOTAMenuScreen ScreenId = EGOTAMenuScreen::MainMenu;

protected:
	/** Override to pick the first focused control when the screen opens. */
	virtual UGOTAMenuButtonWidget* GetDefaultFocusButton() const { return nullptr; }

	/** Called each time the screen becomes active (refresh data here, not in Tick). */
	virtual void OnScreenActivated() {}

	/** Bind a button so focus is remembered for RestoreFocus. Safe to call repeatedly. */
	void TrackFocus(UGOTAMenuButtonWidget* Button);

	UFUNCTION()
	void HandleMenuScreenChanged(EGOTAMenuScreen NewScreen, EGOTAMenuScreen PreviousScreen);

	UFUNCTION()
	void HandleTrackedFocus(UGOTAMenuButtonWidget* Button);

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuSubsystem> MenuSubsystem;

	TWeakObjectPtr<UGOTAMenuButtonWidget> LastFocused;
	bool bIsActive = false;
	float EnterAlpha = 1.f;
};
