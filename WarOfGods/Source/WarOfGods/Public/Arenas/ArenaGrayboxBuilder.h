// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaGrayboxBuilder.generated.h"

class UStaticMesh;
class AArenaPlayerStart;
class ABiomeVolume;
class UArenaVisualDressDefinition;

/**
 * Runtime graybox for every standalone arena + full Shattered Dominion.
 * Gameplay layout stays cube-based; optional UArenaVisualDressDefinition soft-loads art.
 */
UCLASS()
class WAROFGODS_API AArenaGrayboxBuilder : public AActor
{
	GENERATED_BODY()

public:
	AArenaGrayboxBuilder();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifeProps) const override;

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

	/** Optional visual dress. Missing soft refs → graybox only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Dress")
	TSoftObjectPtr<UArenaVisualDressDefinition> VisualDress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Dress")
	bool bApplyVisualDress = true;

	UFUNCTION(BlueprintCallable, Category = "Arena|Graybox")
	void BuildLayout();

	UFUNCTION(BlueprintCallable, Category = "Arena|Graybox")
	void ClearBuiltActors();

	UFUNCTION(BlueprintCallable, Category = "Arena|Dress")
	void ApplyVisualDress();

protected:
	void EnsureMeshes();
	void AddBox(const FVector& Center, const FVector& Extent, const FLinearColor& Color, bool bPlaceholderBoundary = false);
	void AddPillar(const FVector& Center, float HeightUU);
	void AddMeshAt(UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale, bool bVisualOnly = false);
	UStaticMesh* TryLoadMesh(const TCHAR* Path) const;
	UStaticMesh* TryLoadSoftMesh(const TSoftObjectPtr<UStaticMesh>& SoftMesh) const;
	void SpawnPlayerStartAt(const FVector& Location, int32 Slot, EBiomeId Biome);
	void SpawnBiomeVolumeAt(const FVector& Center, const FVector& Extent, EBiomeId BiomeId);
	void SpawnFourCornerStarts(float Radius, float Z, EBiomeId Biome);
	void AddRingWalls(float HalfExtent, float WallHeight, const FLinearColor& Color);
	void AddRingPillars(float Radius, int32 Count, float Height, const FVector& Center = FVector::ZeroVector);
	void PlaceTreeRing(const FVector& Center, float Radius, int32 Count);
	void PlaceDressPoolRing(const TArray<TSoftObjectPtr<UStaticMesh>>& Pool, float Radius, int32 Count, float Z, float UniformScale);

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
