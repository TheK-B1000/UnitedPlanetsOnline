// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryTypes.h"
#include "GOTAStoryObjectiveActor.generated.h"

class UBoxComponent;
class USphereComponent;

/**
 * Server-validated objective trigger.
 * Reach = overlap; Interact = player in range + Use key / Interact call.
 */
UCLASS()
class WAROFGODS_API AGOTAStoryObjectiveActor : public AActor
{
	GENERATED_BODY()

public:
	AGOTAStoryObjectiveActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Story")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Story")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	EStoryObjectiveId ObjectiveId = EStoryObjectiveId::ReachSanctuaryGates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName CheckpointId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	bool bRequiresInteraction = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	bool bOnce = true;

	UPROPERTY(BlueprintReadOnly, Category = "Story")
	bool bCompleted = false;

	UFUNCTION(BlueprintCallable, Category = "Story")
	void TryCompleteFromInteractor(AActor* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Story")
	void OnObjectiveVisualActivated();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void NotifyGameModeCompleted(AActor* InstigatorActor);
};
