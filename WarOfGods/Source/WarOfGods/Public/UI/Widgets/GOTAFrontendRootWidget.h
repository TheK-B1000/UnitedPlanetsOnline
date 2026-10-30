// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MenuTypes.h"
#include "GOTAFrontendRootWidget.generated.h"

class UImage;
class UTextBlock;
class UGOTAMenuSubsystem;
class UGOTAMenuButtonWidget;

/**
 * WBP_GOTA_FrontendRoot — hosts the backdrop, every screen and the modal layer.
 * Global input: Back (Esc / gamepad B) and focus recovery when navigation input arrives
 * with nothing focused (e.g. after a mouse click on empty space, or switching to a pad).
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAFrontendRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;

	/** Focus the active screen (or the modal) — used after modal close and device switches. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void RestoreActiveFocus();

protected:
	UFUNCTION()
	void HandleModalChanged(const FGOTAModalRequest& Request);

	UGOTAMenuSubsystem* GetMenuSubsystem() const;

	/** Clean-plate concept backdrop (temporary until the 3D scene ships). Drifts slowly. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UImage> Image_Backdrop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	float BackdropDriftPixels = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	float BackdropDriftPeriod = 46.f;

	float DriftTime = 0.f;
	bool bRestoreFocusNextTick = false;
};
