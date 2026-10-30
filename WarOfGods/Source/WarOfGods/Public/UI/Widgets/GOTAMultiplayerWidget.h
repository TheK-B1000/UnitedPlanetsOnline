// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "GOTAMultiplayerWidget.generated.h"

class UGOTALobbyViewModel;
class UGOTAMenuButtonWidget;
class UGotaSessionSubsystem;
class UPanelWidget;
class UTextBlock;

/**
 * WBP_GOTA_Multiplayer — LAN play (Online Subsystem Null), up to 4 Wardens.
 * Every control calls UGotaSessionSubsystem (the single session backend) and shows its
 * real status. No online matchmaking, friends or invites are offered — they do not exist.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAMultiplayerWidget : public UGOTAMenuScreenWidget
{
	GENERATED_BODY()

public:
	UGOTAMultiplayerWidget();
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category = "GOTA|Multiplayer")
	UGOTALobbyViewModel* GetLobby() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GOTA|Multiplayer")
	TSubclassOf<UGOTAMenuButtonWidget> EntryClass;

	// Kept for Blueprint extensions (analytics, SFX); session logic stays in C++.
	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Multiplayer")
	void RequestHostGame();
	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Multiplayer")
	void RequestJoinGame();
	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Multiplayer")
	void RequestFindSessions();
	UFUNCTION(BlueprintImplementableEvent, Category = "GOTA|Multiplayer")
	void RequestInviteFriends();

protected:
	virtual UGOTAMenuButtonWidget* GetDefaultFocusButton() const override;
	virtual void OnScreenActivated() override;

	UGotaSessionSubsystem* GetSessions() const;
	void RefreshButtons();
	void RebuildResults();
	void SetBusy(bool bBusy);

	UFUNCTION()
	void HandleHost();
	UFUNCTION()
	void HandleJoin();
	UFUNCTION()
	void HandleFind();
	UFUNCTION()
	void HandleBack();
	UFUNCTION()
	void HandleOpenLobby();
	UFUNCTION()
	void HandleResultFocused(UGOTAMenuButtonWidget* Button);
	UFUNCTION()
	void HandleResultActivated();
	UFUNCTION()
	void HandleHostComplete(bool bSuccess);
	UFUNCTION()
	void HandleFindComplete(int32 ResultCount);
	UFUNCTION()
	void HandleJoinComplete(bool bSuccess);
	UFUNCTION()
	void HandleStatus(FText Message);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_HostGame;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_FindSessions;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_JoinGame;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_InviteFriends;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_OpenLobby;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UGOTAMenuButtonWidget> Btn_Back;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UPanelWidget> Box_Sessions;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Multiplayer")
	TObjectPtr<UTextBlock> Text_Status;

	int32 SelectedResult = INDEX_NONE;
	bool bOperationPending = false;
};
