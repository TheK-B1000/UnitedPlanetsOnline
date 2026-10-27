// White Tiger Studios 2024

#include "Arenas/BiomeVolume.h"

#include "Arenas/BiomeDefinition.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"

ABiomeVolume::ABiomeVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	SetRootComponent(Area);
	Area->SetBoxExtent(FVector(2000.f, 2000.f, 1000.f));
	Area->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Area->SetGenerateOverlapEvents(true);
}

void ABiomeVolume::BeginPlay()
{
	Super::BeginPlay();

	if (BiomeDefinition)
	{
		BiomeId = BiomeDefinition->BiomeId;
	}

	Area->OnComponentBeginOverlap.AddDynamic(this, &ABiomeVolume::HandleBeginOverlap);
	Area->OnComponentEndOverlap.AddDynamic(this, &ABiomeVolume::HandleEndOverlap);
}

void ABiomeVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (Pawn->IsPlayerControlled())
		{
			OnBiomeOverlapChanged.Broadcast(BiomeId, EBiomeId::None);
			UE_LOG(LogTemp, Log, TEXT("BiomeEnter %s -> %d"), *GetNameSafe(Pawn), static_cast<int32>(BiomeId));
		}
	}
}

void ABiomeVolume::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (Pawn->IsPlayerControlled())
		{
			OnBiomeOverlapChanged.Broadcast(EBiomeId::None, BiomeId);
		}
	}
}
