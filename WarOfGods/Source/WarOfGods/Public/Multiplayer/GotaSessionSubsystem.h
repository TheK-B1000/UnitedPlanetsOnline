// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Arenas/ArenaTypes.h"
#include "UI/MenuTypes.h"
#include "GotaSessionSubsystem.generated.h"

class UGOTALobbyViewModel;
class FOnlineSessionSearch;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGotaSessionHosted, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGotaSessionJoined, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGotaSessionsFound, int32, ResultCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGotaSessionStatus, FText, Message);

/**
 * LAN / Null-OSS session authority for menu Host / Find / Join.
 * Prefer existing BP GameInstance CreateMPSession / JoinMPSession / TravelToMap when present;
 * otherwise use OnlineSubsystem (Null) + listen-server travel.
 * UGOTALobbyViewModel is a projection only — refresh it from this subsystem / GameState.
 */
UCLASS()
class WAROFGODS_API UGotaSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "GOTA|Session")
	bool IsSessionBackendAvailable() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Session")
	bool IsFindSessionsSupported() const { return IsSessionBackendAvailable(); }

	UFUNCTION(BlueprintPure, Category = "GOTA|Session")
	bool IsHosting() const { return bIsHost; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Session")
	bool IsInSession() const { return bSessionActive; }

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	bool HostSession(int32 MaxPlayers = 4, bool bLAN = true);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	bool FindSessions(bool bLAN = true);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	bool JoinFirstFoundSession();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	bool JoinSessionAtIndex(int32 SearchResultIndex);

	/** Direct LAN join when discovery is unavailable. Example: 127.0.0.1:17777 */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	bool JoinByAddress(const FString& Address);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	void DestroyCurrentSession();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	void SetSelectedArena(EArenaId ArenaId);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	void SetSelectedMode(EArenaMode Mode);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	EArenaId GetSelectedArena() const { return SelectedArena; }

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	EArenaMode GetSelectedMode() const { return SelectedMode; }

	/** Host-only: travel into selected arena/mode (Duel → DuelGameMode on Quarry). */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	bool StartMatchFromLobby();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	void SetLocalReady(bool bReady);

	UFUNCTION(BlueprintPure, Category = "GOTA|Session")
	bool IsLocalReady() const { return bLocalReady; }

	/** Read-only rows for the session browser (from the last completed search). */
	UFUNCTION(BlueprintPure, Category = "GOTA|Session")
	TArray<FGOTASessionResultView> GetSearchResultViews() const;

	/** Pull PlayerArray into LobbyViewModel (observer projection). */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Session")
	void RefreshLobbyProjection();

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Session")
	FOnGotaSessionHosted OnHostComplete;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Session")
	FOnGotaSessionJoined OnJoinComplete;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Session")
	FOnGotaSessionsFound OnFindComplete;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Session")
	FOnGotaSessionStatus OnStatusMessage;

	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Session")
	FSoftObjectPath MatchMap = FSoftObjectPath(TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap.ThirdPersonMap"));

	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Session")
	FSoftObjectPath LobbyMenuMap = FSoftObjectPath(TEXT("/Game/MainMenu/L_GOTA_MainMenu.L_GOTA_MainMenu"));

	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Session")
	FName SessionName = FName(TEXT("GOTAGameSession"));

	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Session")
	int32 SoftMaxPlayers = 4;

	/**
	 * Legacy BP GameInstance / MenuController session events (CreateMPSession, JoinMpSession,
	 * DestroySession, TravelToMap). Off: JoinMpSession takes a SessionResult struct that cannot be
	 * passed from here, and the BP DestroySession targets a different session name. The native
	 * Null-OSS path below drives the same OSS, so there is still only one session backend.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Session")
	bool bUseBlueprintSessionEvents = false;

protected:
	IOnlineSessionPtr GetSessions() const;
	UGOTALobbyViewModel* GetLobbyVM() const;
	bool TryCallGameInstance(FName FunctionName) const;
	FString BuildTravelOptions(bool bListen) const;
	void TravelToMatch(bool bAsListenHost);
	void PushStatus(const FText& Message);
	void PushError(const FText& Message);
	void HandleCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName InSessionName, bool bWasSuccessful);

	FOnCreateSessionCompleteDelegate CreateCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroyCompleteDelegate;
	FDelegateHandle CreateCompleteHandle;
	FDelegateHandle FindCompleteHandle;
	FDelegateHandle JoinCompleteHandle;
	FDelegateHandle DestroyCompleteHandle;

	TSharedPtr<FOnlineSessionSearch> LastSearch;
	TSharedPtr<FOnlineSessionSettings> LastHostSettings;

	bool bSessionActive = false;
	bool bIsHost = false;
	bool bLocalReady = false;
	bool bLANPreferred = true;

	EArenaId SelectedArena = EArenaId::TitansQuarry;
	EArenaMode SelectedMode = EArenaMode::PvPDuel;
};
