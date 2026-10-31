// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arenas/ArenaTypes.h"
#include "BiomeVolume.generated.h"

class UBoxComponent;
class UBiomeDefinition;

/**
 * Tracks which biome a pawn is inside. Server-relevant for encounter activation later.
 * Presentation/debug color from BiomeDefinition when assigned.
 */
UCLASS()
class WAROFGODS_API ABiomeVolume : public AActor
{
	GENERATED_BODY()

public:
	ABiomeVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Biome")
	TObjectPtr<UBoxComponent> Area;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	EBiomeId BiomeId = EBiomeId::StoneRuins;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	TObjectPtr<UBiomeDefinition> BiomeDefinition;

	UPROPERTY(BlueprintAssignable, Category = "Biome")
	FOnBiomeChanged OnBiomeOverlapChanged;

	UFUNCTION(BlueprintPure, Category = "Biome")
	EBiomeId GetBiomeId() const { return BiomeId; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
