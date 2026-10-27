// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaGrayboxBuilder.generated.h"

class UStaticMesh;
class UInstancedStaticMeshComponent;
class AArenaPlayerStart;
class ABiomeVolume;

/**
 * Builds playable placeholder arena geometry at runtime from LevelPrototyping / engine meshes.
 * Titan's Quarry = first standalone. ShatteredDominionThreeBiome = coliseum + ruins + forest + volcanic stubs.
 */
UCLASS()
class WAROFGODS_API AArenaGrayboxBuilder : public AActor
{
	GENERATED_BODY()

public:
	AArenaGrayboxBuilder();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Replicated so listen-server + clients build the same local placeholder geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Layout, Category = "Arena|Graybox")
	EArenaGrayboxLayout Layout = EArenaGrayboxLayout::TitansQuarry;

	UFUNCTION()
	void OnRep_Layout();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Graybox")
	bool bBuildOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Graybox")
	bool bSpawnPlayerStarts = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Graybox")
	bool bSpawnBiomeVolumes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Graybox")
	float UnitScale = 100.f;

	UFUNCTION(BlueprintCallable, Category = "Arena|Graybox")
	void BuildLayout();

	UFUNCTION(BlueprintCallable, Category = "Arena|Graybox")
	void ClearBuiltActors();

protected:
	void EnsureMeshes();
	void AddBox(const FVector& Center, const FVector& Extent, const FLinearColor& Color);
	void AddPillar(const FVector& Center, float HeightUU);
	void SpawnPlayerStartAt(const FVector& Location, int32 Slot, EBiomeId Biome);
	void SpawnBiomeVolumeAt(const FVector& Center, const FVector& Extent, EBiomeId BiomeId);

	void BuildTitansQuarry();
	void BuildShatteredDominionThreeBiome();

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BuiltMeshes;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> BuiltActors;

	bool bBuilt = false;
};
