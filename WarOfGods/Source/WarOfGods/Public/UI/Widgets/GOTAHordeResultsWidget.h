// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Arenas/Horde/HordeTypes.h"
#include "GOTAHordeResultsWidget.generated.h"

class UTextBlock;
class UButton;
class UGOTAMenuButtonWidget;
class AHordeGameMode;

UCLASS(Blueprintable)
class WAROFGODS_API UGOTAHordeResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	UFUNCTION()
	void HandleReplay();

	UFUNCTION()
	void HandleReturnToLobby();

	void Refresh();
	void EnsureRuntimeLayout();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UTextBlock> Text_Outcome;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UTextBlock> Text_Summary;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Replay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Horde")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_ReturnToLobby;

	/** C++ fallback buttons when WBP / GOTAMenuButtonWidget are absent. */
	UPROPERTY(Transient)
	TObjectPtr<UButton> Btn_ReplayNative;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Btn_ReturnNative;
};
