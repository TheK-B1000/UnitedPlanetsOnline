// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CombatEvents.generated.h"

class AActor;

/** Lightweight combat event payloads — identifiers and context, not deep state copies. */

USTRUCT(BlueprintType)
struct WAROFGODS_API FCombatActorEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	FGameplayTagContainer Tags;
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FCombatHitEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	TWeakObjectPtr<AActor> Attacker;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	TWeakObjectPtr<AActor> Victim;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	float Damage = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	FVector_NetQuantize HitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	FName AttackId = NAME_None;
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FCombatDefenseEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	TWeakObjectPtr<AActor> Defender;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	TWeakObjectPtr<AActor> Attacker;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	bool bPerfect = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Events")
	FName AttackId = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatActorEvent, const FCombatActorEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatHitEvent, const FCombatHitEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatDefenseEvent, const FCombatDefenseEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChanged, float, NewValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLockonTargetChanged, AActor*, NewTarget, AActor*, PreviousTarget);
