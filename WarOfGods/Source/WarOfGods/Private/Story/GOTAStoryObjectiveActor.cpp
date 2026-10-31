// White Tiger Studios 2024

#include "Story/GOTAStoryObjectiveActor.h"
#include "Story/GOTAStoryGameMode.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

AGOTAStoryObjectiveActor::AGOTAStoryObjectiveActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(SceneRoot);
	TriggerVolume->SetBoxExtent(FVector(400.f, 400.f, 300.f));
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerVolume->SetGenerateOverlapEvents(true);
}

void AGOTAStoryObjectiveActor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AGOTAStoryObjectiveActor::HandleBeginOverlap);
	}
}

void AGOTAStoryObjectiveActor::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bCompleted)
	{
		return;
	}
	if (bRequiresInteraction)
	{
		return;
	}
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Cast<APlayerController>(Pawn->GetController()))
	{
		return;
	}
	NotifyGameModeCompleted(Pawn);
}

void AGOTAStoryObjectiveActor::TryCompleteFromInteractor(AActor* Interactor)
{
	if (!HasAuthority() || bCompleted || !bRequiresInteraction)
	{
		return;
	}
	if (!Interactor)
	{
		return;
	}
	NotifyGameModeCompleted(Interactor);
}

void AGOTAStoryObjectiveActor::NotifyGameModeCompleted(AActor* InstigatorActor)
{
	if (bOnce && bCompleted)
	{
		return;
	}
	bCompleted = true;
	OnObjectiveVisualActivated();

	if (AGOTAStoryGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AGOTAStoryGameMode>() : nullptr)
	{
		GM->NotifyObjectiveActorCompleted(this, InstigatorActor);
	}
}
