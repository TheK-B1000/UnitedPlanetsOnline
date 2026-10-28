// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Arenas/ArenaGameMode.h"
#include "Arenas/Duel/DuelTypes.h"
#include "Events/CombatEvents.h"
#include "DuelGameMode.generated.h"

class ADuelGameState;
class ADuelPlayerState;
class UDuelMatchDefinition;
class UCombatComponent;
class UUserWidget;

/**
 * Trial of Champions — 1v1 best-of-N on Titan's Quarry.
 * Launch: ?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3?listen
 *
 * Elimination: pawn validity + UCombatComponent::OnCharacterDied (BP must NotifyDied).
 * Does not introduce a second health / damage framework — BPC_DamageSystem remains apply owner.
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	TSoftObjectPtr<UDuelMatchDefinition> MatchDefinitionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel", meta = (ClampMin = "1"))
	int32 RoundsToWin = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	int32 BestOfRounds = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	float CountdownSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	float RoundDurationSeconds = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	float RoundEndDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	float IntermissionSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	bool bTimeoutAwardHigherHealth = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	float NormalizedMaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	bool bRequireBothPlayers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	bool bAutoStartWhenFull = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel")
	FSoftObjectPath LobbyMap = FSoftObjectPath(TEXT("/Game/MainMenu/L_GOTA_MainMenu.L_GOTA_MainMenu"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|UI")
	TSoftClassPtr<UUserWidget> DuelHudWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|UI")
	TSoftClassPtr<UUserWidget> DuelResultsWidgetClass;

protected:
	void ApplyMatchDefinition();
	void AssignDuelSlots();
	void BeginPreparingRound();
	void BeginCountdown();
	void BeginRound();
	void BeginIntermission();
	void EvaluateRound();
	void HandleRoundTimeout();
	void EndRound(int32 WinningSlot, bool bScored);
	void CompleteMatch(EDuelMatchOutcome Outcome);
	void FinishPendingMatch();
	void RespawnFighters();
	void ResetFighterForRound(APawn* Pawn);
	void SetCombatEnabledForAll(bool bEnabled);
	void BindCombatDeath(APawn* Pawn);
	void UnbindCombatDeath(APawn* Pawn);
	void PresentHudToPlayer(APlayerController* PC);
	void PresentResultsToAll();
	bool IsPawnAlive(APawn* Pawn) const;
	float GetObservedHealth(APawn* Pawn) const;
	ADuelGameState* GetDuelGS() const;
	ADuelPlayerState* GetDuelPS(AController* C) const;
	APlayerController* GetControllerForSlot(int32 Slot) const;

	UFUNCTION()
	void HandleCombatantDied(const FCombatActorEvent& Event);

	FTimerHandle PhaseTimerHandle;
	FTimerHandle RoundWatchTimerHandle;
	FTimerHandle RoundDurationTimerHandle;
	bool bMatchComplete = false;
	bool bRoundResolving = false;
	EDuelMatchOutcome PendingOutcome = EDuelMatchOutcome::None;

	UPROPERTY()
	TObjectPtr<UDuelMatchDefinition> RuntimeMatchDef;

	TSet<TWeakObjectPtr<UCombatComponent>> BoundCombatComponents;
};
