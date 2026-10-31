// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaVisualDressDefinition.generated.h"

class UStaticMesh;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct WAROFGODS_API FArenaDressMeshSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	FName SlotId = NAME_None;

	/** Soft path so missing Marketplace content falls back to graybox. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	TSoftObjectPtr<UMaterialInterface> OverrideMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	FVector Scale = FVector(1.f);

	/** When true, dress mesh is cosmetic-only (no complex collision). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	bool bVisualOnly = true;
};

/**
 * Biome visual dressing separate from gameplay graybox layout.
 * Missing soft references → keep placeholder boxes. Do not hardcode pack paths in the builder.
 */
UCLASS(BlueprintType)
class WAROFGODS_API UArenaVisualDressDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	EArenaGrayboxLayout TargetLayout = EArenaGrayboxLayout::TitansQuarry;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	FString SourcePackNotes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	TArray<FArenaDressMeshSlot> DressSlots;

	/** Optional floor / cliff mesh pools for procedural rings (soft). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress|Pools")
	TArray<TSoftObjectPtr<UStaticMesh>> CliffMeshPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress|Pools")
	TArray<TSoftObjectPtr<UStaticMesh>> DebrisMeshPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress|Pools")
	TArray<TSoftObjectPtr<UStaticMesh>> PillarMeshPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dress")
	bool bHidePlaceholderWallsWhenDressed = false;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ArenaVisualDress"), GetFName());
	}
};
