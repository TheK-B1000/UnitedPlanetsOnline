// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Arenas/ArenaGameMode.h"
#include "Arenas/Duel/DuelTypes.h"
#include "DuelGameMode.generated.h"

class ADuelGameState;
class ADuelPlayerState;

/**
 * 1v1 PvP Duel — Titan's Quarry by default. Best-of-N (RoundsToWin).
 * Launch: ?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?listen
 * Preserves Skirmish via separate AArenaGameMode.
 *
 * Elimination uses pawn validity (same pattern as Horde). BP damage remains health authority;
 * this mode does not introduce a second damage framework.
 */
UCLASS()
class WAROFGODS_API ADuelGameMode : public AArenaGameMode
{
	GENERATED_BODY()

public:
	ADuelGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void RequestStartMatch();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void RequestRematch();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void RequestReturnToLobby();

	/** Rounds needed to win match (best-of-3 → 2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel", meta = (ClampMin = "1"))
	int32 RoundsToWin = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	float CountdownSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	float RoundEndDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	bool bRequireBothPlayers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	bool bAutoStartWhenFull = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	FSoftObjectPath LobbyMap = FSoftObjectPath(TEXT("/Game/MainMenu/L_GOTA_MainMenu.L_GOTA_MainMenu"));

protected:
	void AssignDuelSlots();
	void BeginCountdown();
	void BeginRound();
	void EvaluateRound();
	void EndRound(int32 WinningSlot);
	void CompleteMatch(EDuelMatchOutcome Outcome);
	void FinishPendingMatch();
	void RespawnFighters();
	bool IsPawnAlive(APawn* Pawn) const;
	ADuelGameState* GetDuelGS() const;
	ADuelPlayerState* GetDuelPS(AController* C) const;
	APlayerController* GetControllerForSlot(int32 Slot) const;

	FTimerHandle PhaseTimerHandle;
	FTimerHandle RoundWatchTimerHandle;
	bool bMatchComplete = false;
	EDuelMatchOutcome PendingOutcome = EDuelMatchOutcome::None;
};
