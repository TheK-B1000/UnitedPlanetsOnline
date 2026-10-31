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
 * PvP launch:      ?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3?listen
 * Fight AI launch: ?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3?Opponent=AI
 *
 * Fight AI: slot 1 is an AI champion (its own AIController + a bot ADuelPlayerState for
 * score/name). No second PlayerController is faked. Round rules are identical to PvP.
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
	/** Validates ?Hero=<id> on the server and records it on the player's state. */
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	/** Spawns the accepted hero's gameplay pawn (never a client-supplied class). */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	/** Applies the selected hero's cosmetic body (UGOTAHeroBodyComponent) to every spawned fighter. */
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

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

	/** Champion pawn for Fight AI. Uses its own AIController and BPC_DamageSystem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|AI")
	TSoftClassPtr<APawn> AIChampionClass;

	/**
	 * Blueprint function on the fighter pawn that equips its melee weapon (BP_Player::MeleeStance —
	 * the same path as pressing 1). Called at every round start so fighters never begin unarmed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|Fighters")
	FName FighterLoadoutFunction = TEXT("MeleeStance");

	/** Blueprint function on the champion's AIController that commits it to a target (AIC_Enemy_Base). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|AI")
	FName ChampionEngageFunction = TEXT("SetStateAsAttacking");

	/** Distance from the Warden's start at which the champion enters (same lane, facing the Warden). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|AI", meta = (ClampMin = "300.0"))
	float AIChampionSpawnDistance = 1100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|AI")
	FText AIChampionName = NSLOCTEXT("GOTA", "AIChampionName", "Sovereign Champion");

	/** Fight AI difficulty (`?Difficulty=Recruit|Veteran|Ascendant`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Duel|AI")
	EGOTADuelDifficulty Difficulty = EGOTADuelDifficulty::Veteran;

	UFUNCTION(BlueprintPure, Category = "Duel|AI")
	EGOTADuelDifficulty GetDifficulty() const { return Difficulty; }

	/** The Sovereign Champion's director (server). */
	class UGOTAChampionDirectorComponent* GetChampionDirector() const;

	UFUNCTION(BlueprintPure, Category = "Duel")
	bool IsVersusAI() const { return bVersusAI; }

	/** Fighter pawn currently occupying a duel slot (human or AI). */
	UFUNCTION(BlueprintPure, Category = "Duel")
	APawn* GetFighterPawn(int32 Slot) const;

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
	ADuelPlayerState* GetPSForSlot(int32 Slot) const;
	APlayerController* GetControllerForSlot(int32 Slot) const;
	int32 GetRequiredHumanPlayers() const { return bVersusAI ? 1 : SoftMaxPlayers; }

	// --- Fight AI ---
	void EnsureAIParticipant();
	void SpawnAIFighter();
	void DespawnAIFighter();
	void SetAIBrainActive(bool bActive);
	/** Runtime graybox arenas have no baked navmesh — add bounds and build (same as Horde). */
	void EnsureDuelNavigation();
	/** Round start: make the champion aware of its challenger through its own perception (hearing + damage senses). */
	void AlertChampionToChallenger();
	/** Arms a fighter through its own Blueprint loadout function (no new combat code). */
	void EquipFighterLoadout(APawn* Pawn);
	/** Removes developer-only actors (EQS testing pawns) that perception would treat as targets. */
	void RemoveDeveloperPawns();
	/** Destroys a fighter and everything attached to it (weapons/shields would otherwise litter the arena). */
	void DestroyFighter(APawn* Pawn);
	/** Adds the duel combat layer (momentum, cues, champion director) to this round's fighters. */
	void InitDuelCombat();
	/** Removes the per-pawn W_PlayerHUD that BP_Player adds on BeginPlay, before a respawn. */
	void RemovePawnOwnedHud(APlayerController* PC);
	AActor* FindDuelStartForSlot(int32 Slot) const;

	UFUNCTION()
	void HandleCombatantDied(const FCombatActorEvent& Event);

	/** Diagnostics: logs when / where / why a fighter pawn leaves play. */
	UFUNCTION()
	void HandleFighterEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

	FTimerHandle PhaseTimerHandle;
	FTimerHandle RoundWatchTimerHandle;
	FTimerHandle RoundDurationTimerHandle;
	bool bMatchComplete = false;
	bool bRoundResolving = false;
	EDuelMatchOutcome PendingOutcome = EDuelMatchOutcome::None;

	UPROPERTY()
	TObjectPtr<UDuelMatchDefinition> RuntimeMatchDef;

	TSet<TWeakObjectPtr<UCombatComponent>> BoundCombatComponents;

	bool bVersusAI = false;

	UPROPERTY()
	TObjectPtr<ADuelPlayerState> AIPlayerState;

	UPROPERTY()
	TWeakObjectPtr<APawn> AIFighterPawn;
};
