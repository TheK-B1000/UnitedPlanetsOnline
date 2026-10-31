// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaGameState.generated.h"

UCLASS()
class WAROFGODS_API AArenaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AArenaGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArenaPhase, Category = "Arena")
	EArenaPhase ArenaPhase = EArenaPhase::WaitingForPlayers;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Arena")
	EArenaId ActiveArenaId = EArenaId::TitansQuarry;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Arena")
	EArenaMode ActiveMode = EArenaMode::Skirmish;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WaveIndex, Category = "Arena")
	int32 WaveIndex = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Arena")
	int32 AliveEnemyCount = 0;

	UPROPERTY(BlueprintAssignable, Category = "Arena")
	FOnArenaPhaseChanged OnArenaPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Arena")
	FOnArenaWaveChanged OnArenaWaveChanged;

	UFUNCTION(BlueprintCallable, Category = "Arena")
	void SetArenaPhase(EArenaPhase NewPhase);

	UFUNCTION()
	void OnRep_ArenaPhase();

	UFUNCTION()
	void OnRep_WaveIndex();
};
