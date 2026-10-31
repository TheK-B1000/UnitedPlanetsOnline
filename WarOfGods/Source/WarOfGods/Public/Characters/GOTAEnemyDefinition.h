// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Characters/GOTAHeroDefinition.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "GOTAEnemyDefinition.generated.h"

/** Combat role of an enemy body. Presentation/roster bookkeeping only: combat comes from the pawn class. */
UENUM(BlueprintType)
enum class EGOTAEnemyArchetype : uint8
{
	MeleeSoldier,
	RangedSoldier,
	HeavyBrute,
	MagicCaster,
	EliteChampion,
	Boss,
	/** The only role allowed to show the UE mannequin. */
	TrainingDummy
};

/**
 * An enemy's real character body over its existing combat pawn (BP_Enemy_*). Reuses the hero body fields
 * (BodyMesh, BodyRetargeter, BodyScale, hidden slots...) and the same runtime retarget path, but lives in its
 * own primary asset type ("GOTAEnemy", /Game/Enemies/Data) so it never appears in hero selection.
 * HeroId is the enemy id (prefix "Enemy_").
 */
UCLASS(BlueprintType)
class WAROFGODS_API UGOTAEnemyDefinition : public UGOTAHeroDefinition
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("GOTAEnemy"), HeroId); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EGOTAEnemyArchetype Archetype = EGOTAEnemyArchetype::MeleeSoldier;

	/** Exact pawn class this body dresses (subclasses are not matched). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TSoftClassPtr<APawn> PawnClass;

	/** Dress every spawned PawnClass automatically (server). Off for bodies applied explicitly (Fight AI champion). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	bool bAutoDressPawnClass = true;
};

/** Finds enemy definitions (primary asset type "GOTAEnemy"). */
UCLASS()
class WAROFGODS_API UGOTAEnemyRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	const UGOTAEnemyDefinition* FindEnemy(FName EnemyId) const;
	/** The auto-dress definition for exactly this pawn class, or null. */
	const UGOTAEnemyDefinition* FindAutoDressForClass(const UClass* PawnClass) const;
	TArray<const UGOTAEnemyDefinition*> GetEnemies() const;

private:
	void EnsureLoaded() const;
	mutable bool bLoaded = false;
	UPROPERTY(Transient)
	mutable TArray<TObjectPtr<UGOTAEnemyDefinition>> Enemies;
};

/** Server: gives every spawned (or level-placed) enemy pawn its roster body. Cosmetic; replicates via the body component. */
UCLASS()
class WAROFGODS_API UGOTAEnemyBodySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Applies the enemy body for this pawn's exact class if one is defined. */
	static void DressPawn(APawn* Pawn);

private:
	void HandleActorSpawned(AActor* Actor);
	FDelegateHandle SpawnHandle;
};
