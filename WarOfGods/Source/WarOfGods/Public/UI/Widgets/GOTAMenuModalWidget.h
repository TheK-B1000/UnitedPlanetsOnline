// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuModalWidget.generated.h"

class UTextBlock;
class UImage;
class UWidget;
class UGOTAMenuButtonWidget;
class UGOTAMenuSubsystem;

/**
 * WBP_GOTA_Modal — confirm quit, errors, busy/loading, display-change confirmation.
 * While open it is fully hit-testable (blocks the screen beneath) and traps navigation
 * between its own buttons. Busy has no buttons: travel is committed and honest about it.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAMenuModalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void FocusDefault();

	/** Seconds before an unconfirmed display change reverts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	float DisplayConfirmSeconds = 15.f;

protected:
	UFUNCTION()
	void HandleModalChanged(const FGOTAModalRequest& Request);
	UFUNCTION()
	void HandleConfirm();
	UFUNCTION()
	void HandleCancel();

	void ApplyRequest(const FGOTAModalRequest& Request);
	UGOTAMenuSubsystem* GetMenuSubsystem() const;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_Title;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_Body;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Confirm;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Cancel;
	/** Spinner glyph shown only for Busy. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UImage> Image_Spinner;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UWidget> Panel_Buttons;

	EGOTAMenuModal ActiveModal = EGOTAMenuModal::None;
	FText BaseBody;
	float CountdownRemaining = 0.f;
	float SpinnerAngle = 0.f;
};
