// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Arenas/ArenaTypes.h"
#include "UI/MenuTypes.h"
#include "GOTALobbyViewModel.generated.h"

/**
 * UI-facing lobby snapshot. Does not own multiplayer authority —
 * Blueprint GameInstance / MenuController push observed session state here.
 * Max 4 slots: Host + Friend 1..3.
 */
UCLASS(BlueprintType)
class WAROFGODS_API UGOTALobbyViewModel : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxLobbyPlayers = 4;

	UGOTALobbyViewModel();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void ResetLobby();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void SetSessionActive(bool bActive, bool bLocalIsHost);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void SetSessionPrivacy(EGOTASessionPrivacy Privacy);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void SetSelectedArena(EArenaId ArenaId);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void SetSelectedMode(EArenaMode Mode);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void SetStatusMessage(FText Message);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void SetErrorMessage(FText Message);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void UpsertPlayerSlot(int32 SlotIndex, FText PlayerName, EGOTALobbySlotState State, bool bIsHost, bool bIsLocal, bool bIsReady);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void ClearPlayerSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Lobby")
	void SetPlayerReady(int32 SlotIndex, bool bReady);

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	const TArray<FGOTALobbyPlayerSlot>& GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	int32 GetOccupiedCount() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	bool IsSessionActive() const { return bSessionActive; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	bool IsLocalHost() const { return bLocalPlayerIsHost; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	bool CanHostStartMatch() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	EArenaId GetSelectedArena() const { return SelectedArena; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	EArenaMode GetSelectedMode() const { return SelectedMode; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	EGOTASessionPrivacy GetPrivacy() const { return Privacy; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	FText GetStatusMessage() const { return StatusMessage; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	FText GetErrorMessage() const { return ErrorMessage; }

	/**
	 * Online invites / Steam overlay — not wired until OSS is enabled.
	 * UI must hide or mark Unavailable when this returns false.
	 */
	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	bool AreFriendInvitesSupported() const { return false; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Lobby")
	bool IsMatchmakingSupported() const { return false; }

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Lobby")
	FOnGOTALobbyViewUpdated OnLobbyUpdated;

protected:
	void BroadcastUpdate();
	bool IsValidSlot(int32 SlotIndex) const;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Lobby")
	TArray<FGOTALobbyPlayerSlot>Slots;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Lobby")
	bool bSessionActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Lobby")
	bool bLocalPlayerIsHost = false;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Lobby")
	EArenaId SelectedArena = EArenaId::TitansQuarry;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Lobby")
	EArenaMode SelectedMode = EArenaMode::Skirmish;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Lobby")
	EGOTASessionPrivacy Privacy = EGOTASessionPrivacy::FriendsOnly;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Lobby")
	FText StatusMessage;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Lobby")
	FText ErrorMessage;
};
