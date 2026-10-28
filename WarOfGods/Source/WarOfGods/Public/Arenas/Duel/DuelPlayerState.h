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

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	int32 RoundsWon = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	bool bIsReady = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Duel")
	bool bIsEliminatedThisRound = false;

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void ServerSetReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void SetEliminatedThisRound(bool bEliminated);

	UFUNCTION(BlueprintCallable, Category = "Duel")
	void AddRoundWin();
};
