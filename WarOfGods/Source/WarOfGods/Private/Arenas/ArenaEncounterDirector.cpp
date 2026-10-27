// White Tiger Studios 2024

#include "Arenas/ArenaEncounterDirector.h"

#include "Arenas/ArenaGameState.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AArenaEncounterDirector::AArenaEncounterDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	SpawnOffsets = {
		FVector(800.f, 0.f, 100.f),
		FVector(-800.f, 400.f, 100.f),
		FVector(0.f, -900.f, 100.f),
		FVector(600.f, 700.f, 100.f),
		FVector(-500.f, -600.f, 100.f),
		FVector(200.f, 1100.f, 100.f)
	};
}

void AArenaEncounterDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AArenaEncounterDirector, CurrentWave);
}

void AArenaEncounterDirector::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && bAutoStart)
	{
		StartEncounters();
	}
}

void AArenaEncounterDirector::StartEncounters()
{
	if (!HasAuthority())
	{
		return;
	}

	if (AArenaGameState* GS = GetWorld()->GetGameState<AArenaGameState>())
	{
		GS->SetArenaPhase(EArenaPhase::Active);
	}

	SpawnWave();
	GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &AArenaEncounterDirector::TickWaveGate, 1.f, true);
}

void AArenaEncounterDirector::StopEncounters()
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
}

void AArenaEncounterDirector::TickWaveGate()
{
	if (!HasAuthority())
	{
		return;
	}

	AliveEnemies.RemoveAll([](const TWeakObjectPtr<APawn>& P) { return !P.IsValid(); });

	if (AArenaGameState* GS = GetWorld()->GetGameState<AArenaGameState>())
	{
		GS->AliveEnemyCount = AliveEnemies.Num();
		GS->WaveIndex = CurrentWave;
	}

	if (AliveEnemies.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(WaveTimerHandle);
		FTimerHandle Delay;
		GetWorldTimerManager().SetTimer(Delay, this, &AArenaEncounterDirector::SpawnWave, TimeBetweenWaves, false);
	}
}

void AArenaEncounterDirector::SpawnWave()
{
	if (!HasAuthority())
	{
		return;
	}

	++CurrentWave;

	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArenaEncounterDirector: EnemyClass not set — wave %d logged only (assign BP_Enemy_Melee)."), CurrentWave);
		if (AArenaGameState* GS = GetWorld()->GetGameState<AArenaGameState>())
		{
			GS->WaveIndex = CurrentWave;
			GS->AliveEnemyCount = 0;
			GS->OnArenaWaveChanged.Broadcast(CurrentWave, 0);
		}
		GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &AArenaEncounterDirector::TickWaveGate, 1.f, true);
		return;
	}

	const int32 ToSpawn = FMath::Min(EnemiesPerWave, MaxConcurrentEnemies);
	for (int32 i = 0; i < ToSpawn; ++i)
	{
		const FVector Offset = SpawnOffsets.IsValidIndex(i) ? SpawnOffsets[i] : FVector(500.f * i, 0.f, 100.f);
		const FVector Loc = GetActorLocation() + Offset;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (APawn* Enemy = GetWorld()->SpawnActor<APawn>(EnemyClass, Loc, FRotator::ZeroRotator, Params))
		{
			AliveEnemies.Add(Enemy);
		}
	}

	if (AArenaGameState* GS = GetWorld()->GetGameState<AArenaGameState>())
	{
		GS->WaveIndex = CurrentWave;
		GS->AliveEnemyCount = AliveEnemies.Num();
		GS->OnArenaWaveChanged.Broadcast(CurrentWave, AliveEnemies.Num());
	}

	GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &AArenaEncounterDirector::TickWaveGate, 1.f, true);
	UE_LOG(LogTemp, Log, TEXT("Arena wave %d spawned %d enemies"), CurrentWave, AliveEnemies.Num());
}
