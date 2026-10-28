// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DuelFighterInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UDuelFighterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by BP_Player for duel round reset without C++ owning health.
 * GameMode calls these on the server between rounds / during countdown.
 */
class WAROFGODS_API IDuelFighterInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Duel")
	void ResetForDuelRound(float NormalizedMaxHealth);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Duel")
	void SetDuelCombatEnabled(bool bEnabled);
};
