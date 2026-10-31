// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arenas/Horde/HordeTypes.h"
#include "HordeSpawnPoint.generated.h"

UCLASS()
class WAROFGODS_API AHordeSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AHordeSpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|Spawn")
	FName SpawnGroup = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|Spawn")
	TArray<EHordeEnemyRole> AllowedRoles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|Spawn", meta = (ClampMin = "0.0"))
	float MinPlayerDistance = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Horde|Spawn")
	bool bBossEntrance = false;

	UPROPERTY(VisibleAnywhere, Category = "Horde|Spawn")
	TObjectPtr<USceneComponent> SceneRoot;

	UFUNCTION(BlueprintPure, Category = "Horde|Spawn")
	bool AllowsRole(EHordeEnemyRole EnemyRole) const;

	UFUNCTION(BlueprintPure, Category = "Horde|Spawn")
	bool IsFarEnoughFromPlayers(const UWorld* World) const;
};
