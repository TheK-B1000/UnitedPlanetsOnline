// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaGrayboxBuilder.generated.h"

class UStaticMesh;
class AArenaPlayerStart;
class ABiomeVolume;

/**
 * Runtime graybox for every standalone arena + full Shattered Dominion.
 * Uses LevelPrototyping / engine cubes; dresses with Beech/Maple/Megascans when present.
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

	UFUNCTION(BlueprintCallable, Category = "Arena|Graybox")
	void BuildLayout();

	UFUNCTION(BlueprintCallable, Category = "Arena|Graybox")
	void ClearBuiltActors();

protected:
	void EnsureMeshes();
	void AddBox(const FVector& Center, const FVector& Extent, const FLinearColor& Color);
	void AddPillar(const FVector& Center, float HeightUU);
	void AddMeshAt(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	UStaticMesh* TryLoadMesh(const TCHAR* Path) const;
	void SpawnPlayerStartAt(const FVector& Location, int32 Slot, EBiomeId Biome);
	void SpawnBiomeVolumeAt(const FVector& Center, const FVector& Extent, EBiomeId BiomeId);
	void SpawnFourCornerStarts(float Radius, float Z, EBiomeId Biome);
	void AddRingWalls(float HalfExtent, float WallHeight, const FLinearColor& Color);
	void AddRingPillars(float Radius, int32 Count, float Height, const FVector& Center = FVector::ZeroVector);
	void PlaceTreeRing(const FVector& Center, float Radius, int32 Count);

	void BuildTitansQuarry();
	void BuildAshfallCrucible();
	void BuildFrostboundCitadel();
	void BuildVerdantSanctuary();
	void BuildSunscarExpanse();
	void BuildStormbreakCoast();
	void BuildShatteredDominion();
	void BuildFallenSanctuary();

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
