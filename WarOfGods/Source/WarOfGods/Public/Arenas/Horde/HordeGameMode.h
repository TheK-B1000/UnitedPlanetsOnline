// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Arenas/ArenaGameMode.h"
#include "Arenas/Horde/HordeTypes.h"
#include "HordeGameMode.generated.h"

class UHordeMatchDefinition;
class UHordeWaveDefinition;
class AHordeEncounterDirector;
class AHordeSpawnPoint;
class AHordeGameState;
class UUserWidget;

/**
 * Wrath of the Gods — server-authoritative 5-wave + boss PvE Horde.
 * Launch: ?game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry
 * Preserves Skirmish via separate AArenaGameMode default.
 */
UCLASS()
class WAROFGODS_API AHordeGameMode : public AArenaGameMode
{
	GENERATED_BODY()

public:
	AHordeGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void RequestStartMatch();

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void RequestRestartMatch();

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void RequestReturnToLobby();

	/** Dev: destroy current wave enemies and advance clear. */
	UFUNCTION(BlueprintCallable, Category = "Horde|Dev")
	void DevForceClearWave();

	/** Dev: skip Waiting/Preparing/Intermission into the next wave immediately. */
	UFUNCTION(BlueprintCallable, Category = "Horde|Dev")
	void DevSkipPrepare();

	UFUNCTION(BlueprintCallable, Category = "Horde|Dev")
	void DevDumpStatus() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde")
	TSoftObjectPtr<UHordeMatchDefinition> MatchDefinitionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde")
	float AutoStartDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde")
	bool bAutoStartWhenPlayersReady = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde")
	FSoftObjectPath LobbyMap = FSoftObjectPath(TEXT("/Game/MainMenu/L_GOTA_MainMenu.L_GOTA_MainMenu"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|Classes")
	TSoftClassPtr<APawn> SoftRangedEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|Classes")
	TSoftClassPtr<APawn> SoftEliteEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|Classes")
	TSoftClassPtr<APawn> SoftBossEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|UI")
	TSoftClassPtr<UUserWidget> HordeHudWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|UI")
	TSoftClassPtr<UUserWidget> HordeResultsWidgetClass;

protected:
	void EnsureHordeInfrastructure();
	void EnsureDefaultSpawnPoints();
	void EnsureHordeNavBounds();
	void ResolveHordeEnemyClasses();
	void BuildRuntimeMatchDefinition();
	UHordeWaveDefinition* MakeRuntimeWave(FName Id, FText Name, int32 Melee, int32 Ranged, int32 Elite, bool bBoss, int32 Concurrent);
	bool TryTransition(EHordeMatchPhase NewPhase);
	void BeginPreparing();
	void BeginNextWave();
	void BeginIntermission();
	void BeginBossFinale();
	void HandleWaveCleared();
	void HandleEnemyScore(int32 ScoreValue);
	void EvaluateTeamWipe();
	void CompleteVictory();
	void CompleteDefeat();
	void RespawnEliminatedPlayers();
	void PresentHudToPlayer(APlayerController* PC);
	void PresentResultsToAll();
	UClass* ResolveHudWidgetClass() const;
	UClass* ResolveResultsWidgetClass() const;
	int32 GetActivePlayerCount() const;
	AHordeGameState* GetHordeGS() const;

	UFUNCTION()
	void OnDirectorWaveCleared();

	UFUNCTION()
	void OnDirectorEnemyDefeated(int32 ScoreValue);

	UPROPERTY()
	TObjectPtr<AHordeEncounterDirector> HordeDirector;

	UPROPERTY()
	TObjectPtr<UHordeMatchDefinition> RuntimeMatchDef;

	UPROPERTY()
	TArray<TObjectPtr<UHordeWaveDefinition>> RuntimeWaves;

	UPROPERTY()
	TObjectPtr<UHordeWaveDefinition> RuntimeBossWave;

	bool bMatchStarted = false;
	bool bMatchEnded = false;
	bool bHordeSmokeTest = false;
	bool bHordeInfrastructureReady = false;
	int32 NextWaveToStart = 0;

	FTimerHandle PhaseTimerHandle;
	FTimerHandle PlayerWatchTimerHandle;
	FTimerHandle SmokeTimerHandle;

	void RunHordeSmokeStep();
};
