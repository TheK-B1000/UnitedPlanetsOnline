// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatBootstrapSubsystem.generated.h"

class APawn;
class UCombatComponent;

/**
 * Ensures combat C++ components exist on player pawns without requiring a BP parent change.
 * Stats stay in observation mode by default so Blueprint damage remains authoritative.
 */
UCLASS()
class WAROFGODS_API UCombatBootstrapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UPROPERTY(EditAnywhere, Category = "Combat|Bootstrap")
	bool bAutoAttachComponents = true;

	/** When false, StatsComponent refuses mutations (BP damage remains source of truth). */
	UPROPERTY(EditAnywhere, Category = "Combat|Bootstrap")
	bool bEnableAuthoritativeStats = false;

	UPROPERTY(EditAnywhere, Category = "Combat|Bootstrap")
	bool bLogCombatEvents = true;

protected:
	UFUNCTION()
	void HandlePawnPossessed(APawn* OldPawn, APawn* NewPawn);

	void HandleActorSpawned(AActor* Spawned);
	void BindPlayerController(APlayerController* PC);
	void EnsureComponentsOnPawn(APawn* Pawn);
	void BindCombatLogging(UCombatComponent* Combat);

	FDelegateHandle ActorSpawnedHandle;
	TSet<TWeakObjectPtr<UCombatComponent>> LoggingBound;
};
