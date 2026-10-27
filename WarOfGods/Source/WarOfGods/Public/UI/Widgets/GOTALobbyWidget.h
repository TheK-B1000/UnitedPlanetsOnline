// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "UI/MenuTypes.h"
#include "GOTALobbyWidget.generated.h"

class UGOTALobbyViewModel;
class UGOTAMenuButtonWidget;

/** Champion hall — 4 slots bound to UGOTALobbyViewModel (observed session state). */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTALobbyWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTALobbyWidget();

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	UGOTALobbyViewModel* GetLobby() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Lobby")
	void OnLobbyModelUpdated();

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Lobby")
	void RequestStartMatch();

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Lobby")
	void RequestLeaveLobby();

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Lobby")
	void RequestToggleReady();

protected:
	UFUNCTION()
	void HandleLobbyUpdated();
	UFUNCTION()
	void HandleReady();
	UFUNCTION()
	void HandleStart();
	UFUNCTION()
	void HandleLeave();
	UFUNCTION()
	void HandleSelectArena();
	UFUNCTION()
	void HandleBack();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Lobby")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Ready;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Lobby")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_StartMatch;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Lobby")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Leave;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Lobby")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_SelectArena;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Lobby")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;
};
