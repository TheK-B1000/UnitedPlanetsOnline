// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DuelPlayerState.generated.h"

UCLASS()
class WAROFGODS_API ADuelPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ADuelPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 DuelSlot = INDEX_NONE;

	/** Server-validated hero (stable id only — clients resolve art from UGOTAHeroDefinition). */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HeroId, Category = "Duel")
	FName HeroId;

	UFUNCTION()
	void OnRep_HeroId() {}

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 RoundsWon = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 Eliminations = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 Deaths = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	bool bIsReady = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	bool bWantsRematch = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	bool bIsEliminatedThisRound = false;

	/** Owning client → server ready flag. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Duel")
	void ServerSetReady(bool bReady);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Duel")
	void ServerRequestRematch();

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Duel")
	void ServerRequestReturnToLobby();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void SetEliminatedThisRound(bool bEliminated);

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void AddRoundWin();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void AddElimination();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void AddDeath();

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void ResetMatchStats();
};
