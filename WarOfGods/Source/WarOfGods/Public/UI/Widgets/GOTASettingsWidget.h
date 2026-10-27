// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTASettingsWidget.generated.h"

class UGOTAMenuSettingsAdapter;
class UGOTAMenuButtonWidget;

UCLASS(Blueprintable)
class WAROFGODS_API UGOTASettingsWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTASettingsWidget();

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	UGOTAMenuSettingsAdapter* GetAdapter() const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void ApplySettings();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void CancelSettings();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void RestoreDefaults();

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Settings")
	void OnSettingsModelRefreshed();

protected:
	UFUNCTION()
	void HandleApply();
	UFUNCTION()
	void HandleCancel();
	UFUNCTION()
	void HandleDefaults();
	UFUNCTION()
	void HandleBack();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Apply;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Cancel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Defaults;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Settings")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;
};
