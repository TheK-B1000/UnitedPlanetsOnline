// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Events/CombatEvents.h"
#include "LockonComponent.generated.h"

class AActor;
class APlayerController;

/**
 * Camera/target lock-on. Presentation and selection live here;
 * Blueprint combat rules remain authoritative until CombatComponent migration.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API ULockonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockonComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Combat|Lockon")
	void ToggleLockon();

	UFUNCTION(BlueprintCallable, Category = "Combat|Lockon")
	void StartLockon();

	UFUNCTION(BlueprintCallable, Category = "Combat|Lockon")
	void StopLockon();

	UFUNCTION(BlueprintPure, Category = "Combat|Lockon")
	bool IsLockedOn() const { return CurrentTarget != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Combat|Lockon")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Lockon")
	void SwitchTarget(float Direction);

	UPROPERTY(BlueprintAssignable, Category = "Combat|Lockon")
	FOnLockonTargetChanged OnLockonTargetChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Lockon")
	float MaxLockonDistance = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Lockon")
	float MaxLockonAngleDegrees = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Lockon")
	float SoftLookInterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Lockon")
	TEnumAsByte<ECollisionChannel> TargetTraceChannel = ECC_Pawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Lockon")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Lockon")
	bool bRotateOwnerTowardTarget = true;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Lockon")
	TObjectPtr<AActor> CurrentTarget;

	AActor* FindBestTarget() const;
	bool IsValidLockonCandidate(AActor* Candidate, const FVector& ViewLocation, const FVector& ViewDirection) const;
	void SetCurrentTarget(AActor* NewTarget);
	APlayerController* GetOwnerPlayerController() const;
};
