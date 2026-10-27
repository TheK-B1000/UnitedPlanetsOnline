// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTAMultiplayerWidget.generated.h"

class UGOTALobbyViewModel;
class UGOTAMenuButtonWidget;

/**
 * Multiplayer entry: Host / Join / Browser hooks.
 * Actual Create/Join session remains on BP_WarOfGods_GameInstance —
 * call those from BlueprintImplementableEvents so we do not invent a second session stack.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAMultiplayerWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAMultiplayerWidget();

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintPure, Category = "GOTA|Multiplayer")
	UGOTALobbyViewModel* GetLobby() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Multiplayer")
	void RequestHostGame();

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Multiplayer")
	void RequestJoinGame();

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Multiplayer")
	void RequestFindSessions();

	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Multiplayer")
	void RequestInviteFriends();

protected:
	UFUNCTION()
	void HandleHost();
	UFUNCTION()
	void HandleJoin();
	UFUNCTION()
	void HandleFind();
	UFUNCTION()
	void HandleInvite();
	UFUNCTION()
	void HandleBack();
	UFUNCTION()
	void HandleOpenLobby();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_HostGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_JoinGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_FindSessions;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_InviteFriends;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_OpenLobby;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;
};
