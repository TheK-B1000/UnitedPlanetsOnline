// White Tiger Studios 2024

#include "Arenas/ArenaStreamingAnchor.h"

#include "Components/SceneComponent.h"

AArenaStreamingAnchor::AArenaStreamingAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

bool AArenaStreamingAnchor::IsPlayerInRange(const AActor* Player) const
{
	if (!Player)
	{
		return false;
	}
	return FVector::DistSquared(Player->GetActorLocation(), GetActorLocation())
		<= FMath::Square(ActivationRadius);
}
