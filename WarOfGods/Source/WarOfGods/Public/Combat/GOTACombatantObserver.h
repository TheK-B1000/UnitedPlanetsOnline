// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GOTACombatantObserver.generated.h"

class UActorComponent;

/** Snapshot of a combatant's Blueprint-owned health. */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTACombatantVitals
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bHasDamageSystem = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsDead = false;

	float GetFraction() const { return MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f; }
};

/**
 * Read-only bridge onto BPC_DamageSystem (the project's health authority, Blueprint-owned).
 * Reads Health / MaxHealth / IsDead by reflection. Never writes health — the damage system
 * stays the single source of truth (see GOTA_COMBAT_AUTHORITY_CONTRACT.md).
 */
UCLASS()
class WAROFGODS_API UGOTACombatantObserver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "GOTA|Combat")
	static FGOTACombatantVitals ReadVitals(const AActor* Combatant);

	/** Dead = damage system reports IsDead, or the actor is gone / being destroyed. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Combat")
	static bool IsCombatantDefeated(const AActor* Combatant);

	static UActorComponent* FindDamageSystem(const AActor* Combatant);
};
