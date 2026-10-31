// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Events/CombatEvents.h"
#include "CombatComponent.generated.h"

/**
 * Observer-facing combat event hub.
 * Blueprint attack/parry/dodge rules stay in BPC_Attacks until Phase 2 migration;
 * call the Notify* helpers from Blueprint to broadcast typed events.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatActorEvent OnAttackStarted;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatActorEvent OnAttackCommitted;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatHitEvent OnHitConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatHitEvent OnDamageApplied;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatDefenseEvent OnBlockSucceeded;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatDefenseEvent OnParrySucceeded;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatDefenseEvent OnPerfectParry;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatDefenseEvent OnPerfectDodge;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatActorEvent OnCharacterStaggered;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatActorEvent OnCharacterDied;

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyAttackStarted(AActor* Target, FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyAttackCommitted(AActor* Target, FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyHitConfirmed(AActor* Victim, float Damage, FVector HitLocation, FName AttackId);

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyDamageApplied(AActor* Victim, float Damage, FVector HitLocation, FName AttackId);

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyBlockSucceeded(AActor* Attacker, FName AttackId);

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyParrySucceeded(AActor* Attacker, bool bPerfect, FName AttackId);

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyPerfectDodge(AActor* Attacker, FName AttackId);

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyStaggered(AActor* Instigator, FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category = "Combat|Events")
	void NotifyDied(AActor* Killer, FGameplayTagContainer Tags);
};
