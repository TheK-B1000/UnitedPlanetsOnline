// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaStreamingAnchor.generated.h"

/**
 * Implementation 10 scaffold: mark giant-arena regions for future World Partition /
 * level-streaming loads (Shattered Dominion biomes, traversal gates, encounter volumes).
 * Does not perform streaming yet — place in maps as authoring markers.
 */
UCLASS(Blueprintable)
class WAROFGODS_API AArenaStreamingAnchor : public AActor
{
	GENERATED_BODY()

public:
	AArenaStreamingAnchor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Streaming")
	FName RegionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Streaming")
	FName EncounterId = NAME_None;

	/** Soft path to a sub-level / WP cell to load when the player enters (wired later). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Streaming")
	FSoftObjectPath StreamingLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Streaming")
	float ActivationRadius = 5000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Streaming")
	TObjectPtr<USceneComponent> SceneRoot;

	UFUNCTION(BlueprintPure, Category = "Arena|Streaming")
	bool IsPlayerInRange(const AActor* Player) const;
};
