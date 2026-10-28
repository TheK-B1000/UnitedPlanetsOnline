// White Tiger Studios 2024

#include "Arenas/Horde/HordeSpawnPoint.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AHordeSpawnPoint::AHordeSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AllowedRoles = {
		EHordeEnemyRole::Grunt,
		EHordeEnemyRole::Ranged,
		EHordeEnemyRole::Elite
	};
}

bool AHordeSpawnPoint::AllowsRole(EHordeEnemyRole EnemyRole) const
{
	if (AllowedRoles.Num() == 0)
	{
		return true;
	}
	return AllowedRoles.Contains(EnemyRole);
}

bool AHordeSpawnPoint::IsFarEnoughFromPlayers(const UWorld* World) const
{
	if (!World)
	{
		return true;
	}

	const FVector Loc = GetActorLocation();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (FVector::DistSquared(Loc, Pawn->GetActorLocation()) < FMath::Square(MinPlayerDistance))
				{
					return false;
				}
			}
		}
	}
	return true;
}
