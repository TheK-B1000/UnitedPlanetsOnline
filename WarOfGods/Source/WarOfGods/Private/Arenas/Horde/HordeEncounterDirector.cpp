// White Tiger Studios 2024

#include "Arenas/Horde/HordeEncounterDirector.h"

#include "Arenas/Horde/HordeGameState.h"
#include "Arenas/Horde/HordeSpawnPoint.h"
#include "Arenas/Horde/HordeWaveDefinition.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AHordeEncounterDirector::AHordeEncounterDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void AHordeEncounterDirector::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ScrubTimerHandle, this, &AHordeEncounterDirector::TickAliveScrub, 0.5f, true);
	}
}

void AHordeEncounterDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopWaveSpawning();
	GetWorldTimerManager().ClearTimer(ScrubTimerHandle);
	CleanupTrackedEnemies(true);
	Super::EndPlay(EndPlayReason);
}

void AHordeEncounterDirector::ConfigureWave(UHordeWaveDefinition* WaveDef, int32 PlayerCount)
{
	if (!HasAuthority())
	{
		return;
	}

	StopWaveSpawning();
	CleanupTrackedEnemies(true);

	ActiveWave = WaveDef;
	ActivePlayerCount = FMath::Clamp(PlayerCount, 1, 4);
	SpawnQueue.Reset();
	SpawnQueueIndex = 0;
	RemainingToSpawn = 0;
	AliveCount = 0;
	DefeatedCount = 0;
	FailedSpawnCount = 0;
	bWaveClearBroadcast = false;
	bSpawnQueueActive = false;

	if (ActiveWave)
	{
		BuildSpawnQueue();
	}
	PushCountsToGameState();
}

void AHordeEncounterDirector::BuildSpawnQueue()
{
	SpawnQueue.Reset();
	RemainingToSpawn = 0;
	if (!ActiveWave)
	{
		return;
	}

	for (const FHordeEnemySpawnEntry& Entry : ActiveWave->EnemyEntries)
	{
		const int32 Count = ScaledCount(Entry);
		for (int32 i = 0; i < Count; ++i)
		{
			SpawnQueue.Add(Entry);
		}
		RemainingToSpawn += Count;
	}
}

int32 AHordeEncounterDirector::ScaledCount(const FHordeEnemySpawnEntry& Entry) const
{
	const int32 ExtraPlayers = FMath::Max(0, ActivePlayerCount - 1);
	return FMath::Max(0, Entry.Count + ExtraPlayers * Entry.CountPerExtraPlayer);
}

int32 AHordeEncounterDirector::ScaledConcurrentCap() const
{
	if (!ActiveWave)
	{
		return 6;
	}
	const int32 ExtraPlayers = FMath::Max(0, ActivePlayerCount - 1);
	return FMath::Max(1, ActiveWave->MaxConcurrentEnemies + ExtraPlayers * ActiveWave->MaxConcurrentPerExtraPlayer);
}

void AHordeEncounterDirector::StartWaveSpawning()
{
	if (!HasAuthority() || !ActiveWave)
	{
		return;
	}

	bSpawnQueueActive = true;
	bWaveClearBroadcast = false;
	SpawnQueueIndex = 0;

	const float Interval = ActiveWave->EnemyEntries.Num() > 0
		? FMath::Max(0.05f, ActiveWave->EnemyEntries[0].SpawnInterval)
		: 0.35f;

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AHordeEncounterDirector::TickSpawnQueue, Interval, true);
	TickSpawnQueue();
}

void AHordeEncounterDirector::StopWaveSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	bSpawnQueueActive = false;
}

void AHordeEncounterDirector::CleanupTrackedEnemies(bool bDestroyAlive)
{
	for (FHordeTrackedEnemy& Tracked : TrackedEnemies)
	{
		if (APawn* Pawn = Tracked.Pawn.Get())
		{
			Pawn->OnEndPlay.RemoveAll(this);
			if (bDestroyAlive && IsValid(Pawn))
			{
				Pawn->Destroy();
			}
		}
	}
	TrackedEnemies.Reset();
	AliveCount = 0;
	PushCountsToGameState();
}

void AHordeEncounterDirector::ForceClearWave()
{
	if (!HasAuthority())
	{
		return;
	}

	StopWaveSpawning();
	SpawnQueueIndex = SpawnQueue.Num();
	RemainingToSpawn = 0;
	CleanupTrackedEnemies(true);
	AliveCount = 0;
	PushCountsToGameState();

	if (!bWaveClearBroadcast)
	{
		bWaveClearBroadcast = true;
		UE_LOG(LogTemp, Log, TEXT("Horde ForceClearWave broadcast (defeated=%d failedSpawns=%d)"), DefeatedCount, FailedSpawnCount);
		OnWaveCleared.Broadcast();
	}
}

int32 AHordeEncounterDirector::FlushSpawnQueueNow()
{
	if (!HasAuthority() || !ActiveWave)
	{
		return 0;
	}

	bSpawnQueueActive = true;
	int32 Spawned = 0;
	const int32 Safety = FMath::Max(SpawnQueue.Num() + 4, 16);
	for (int32 i = 0; i < Safety && SpawnQueueIndex < SpawnQueue.Num(); ++i)
	{
		const FHordeEnemySpawnEntry& Entry = SpawnQueue[SpawnQueueIndex];
		if (TrySpawnOne(Entry))
		{
			++Spawned;
			++SpawnQueueIndex;
			RemainingToSpawn = FMath::Max(0, RemainingToSpawn - 1);
		}
		else
		{
			++FailedSpawnCount;
			++SpawnQueueIndex;
			RemainingToSpawn = FMath::Max(0, RemainingToSpawn - 1);
			UE_LOG(LogTemp, Warning, TEXT("Horde FlushSpawnQueueNow failed (total failures=%d)"), FailedSpawnCount);
		}
	}
	StopWaveSpawning();
	PushCountsToGameState();
	UE_LOG(LogTemp, Log, TEXT("Horde FlushSpawnQueueNow spawned=%d alive=%d failed=%d"), Spawned, AliveCount, FailedSpawnCount);
	return Spawned;
}

bool AHordeEncounterDirector::IsWaveFullyCleared() const
{
	return RemainingToSpawn <= 0 && !bSpawnQueueActive && AliveCount <= 0 && SpawnQueueIndex >= SpawnQueue.Num();
}

void AHordeEncounterDirector::TickSpawnQueue()
{
	if (!HasAuthority() || !bSpawnQueueActive)
	{
		return;
	}

	while (SpawnQueueIndex < SpawnQueue.Num() && AliveCount < ScaledConcurrentCap())
	{
		const FHordeEnemySpawnEntry& Entry = SpawnQueue[SpawnQueueIndex];
		if (TrySpawnOne(Entry))
		{
			++SpawnQueueIndex;
			RemainingToSpawn = FMath::Max(0, RemainingToSpawn - 1);
		}
		else
		{
			++FailedSpawnCount;
			++SpawnQueueIndex;
			RemainingToSpawn = FMath::Max(0, RemainingToSpawn - 1);
			UE_LOG(LogTemp, Warning, TEXT("Horde spawn failed (total failures=%d)"), FailedSpawnCount);
			break;
		}

		// One spawn per timer tick keeps hitch spikes down.
		break;
	}

	if (SpawnQueueIndex >= SpawnQueue.Num())
	{
		StopWaveSpawning();
	}

	PushCountsToGameState();
	TickAliveScrub();
}

bool AHordeEncounterDirector::TrySpawnOne(const FHordeEnemySpawnEntry& Entry)
{
	UClass* EnemyClass = Entry.EnemyClass.LoadSynchronous();
	if (!EnemyClass)
	{
		// Soft path may be stale; try the common melee pawn used across GOTA modes.
		static TSoftClassPtr<APawn> MeleeFallback(FSoftObjectPath(TEXT("/Game/Enemies/MeleeEnemy/BP_Enemy_Melee.BP_Enemy_Melee_C")));
		EnemyClass = MeleeFallback.LoadSynchronous();
		if (!EnemyClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Horde TrySpawnOne: class load failed for %s"), *Entry.EnemyClass.ToString());
			return false;
		}
		UE_LOG(LogTemp, Warning, TEXT("Horde TrySpawnOne: fell back to BP_Enemy_Melee for role %d"), static_cast<int32>(Entry.Role));
	}

	AHordeSpawnPoint* Point = PickSpawnPoint(Entry.Role);
	FVector Loc = GetActorLocation() + FVector(FMath::FRandRange(-400.f, 400.f), FMath::FRandRange(600.f, 1200.f), 100.f);
	FRotator Rot = FRotator::ZeroRotator;
	if (Point)
	{
		Loc = Point->GetActorLocation();
		Rot = Point->GetActorRotation();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	APawn* Enemy = GetWorld()->SpawnActor<APawn>(EnemyClass, Loc, Rot, Params);
	if (!Enemy)
	{
		return false;
	}

	RegisterEnemy(Enemy, Entry.Role, Entry.ScoreValue);
	return true;
}

AHordeSpawnPoint* AHordeEncounterDirector::PickSpawnPoint(EHordeEnemyRole EnemyRole) const
{
	TArray<AHordeSpawnPoint*> Candidates;
	float TotalWeight = 0.f;

	for (TActorIterator<AHordeSpawnPoint> It(GetWorld()); It; ++It)
	{
		AHordeSpawnPoint* Point = *It;
		if (!Point || !Point->AllowsRole(EnemyRole))
		{
			continue;
		}
		if (EnemyRole == EHordeEnemyRole::Boss && !Point->bBossEntrance && Point->AllowedRoles.Num() > 0)
		{
			// Prefer boss-marked points when present.
		}
		if (!Point->IsFarEnoughFromPlayers(GetWorld()))
		{
			continue;
		}
		Candidates.Add(Point);
		TotalWeight += FMath::Max(0.01f, Point->Weight);
	}

	if (Candidates.Num() == 0)
	{
		// Relax distance constraint.
		for (TActorIterator<AHordeSpawnPoint> It(GetWorld()); It; ++It)
		{
			if (It->AllowsRole(EnemyRole))
			{
				Candidates.Add(*It);
				TotalWeight += FMath::Max(0.01f, It->Weight);
			}
		}
	}

	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (AHordeSpawnPoint* Point : Candidates)
	{
		Roll -= FMath::Max(0.01f, Point->Weight);
		if (Roll <= 0.f)
		{
			if (EnemyRole == EHordeEnemyRole::Boss)
			{
				for (AHordeSpawnPoint* BossPoint : Candidates)
				{
					if (BossPoint->bBossEntrance)
					{
						return BossPoint;
					}
				}
			}
			return Point;
		}
	}
	return Candidates.Last();
}

void AHordeEncounterDirector::RegisterEnemy(APawn* Enemy, EHordeEnemyRole EnemyRole, int32 ScoreValue)
{
	if (!Enemy)
	{
		return;
	}

	FHordeTrackedEnemy Tracked;
	Tracked.SpawnId = NextSpawnId++;
	Tracked.Pawn = Enemy;
	Tracked.Role = EnemyRole;
	Tracked.ScoreValue = ScoreValue;
	Tracked.bCountedDead = false;
	TrackedEnemies.Add(Tracked);
	++AliveCount;

	Enemy->OnEndPlay.AddDynamic(this, &AHordeEncounterDirector::HandleEnemyEndPlay);
}

void AHordeEncounterDirector::HandleEnemyEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
	if (!HasAuthority() || !Actor)
	{
		return;
	}

	for (FHordeTrackedEnemy& Tracked : TrackedEnemies)
	{
		if (Tracked.Pawn.Get() == Actor)
		{
			const bool bLegitimate = Reason == EEndPlayReason::Destroyed;
			NotifyEnemyDefeated(Tracked, bLegitimate);
			break;
		}
	}
}

void AHordeEncounterDirector::NotifyEnemyDefeated(FHordeTrackedEnemy& Tracked, bool bLegitimateKill)
{
	if (Tracked.bCountedDead)
	{
		return;
	}
	Tracked.bCountedDead = true;
	AliveCount = FMath::Max(0, AliveCount - 1);

	if (bLegitimateKill)
	{
		++DefeatedCount;
		OnEnemyDefeated.Broadcast(Tracked.ScoreValue);
	}

	PushCountsToGameState();

	if (IsWaveFullyCleared() && !bWaveClearBroadcast)
	{
		bWaveClearBroadcast = true;
		OnWaveCleared.Broadcast();
	}
}

void AHordeEncounterDirector::TickAliveScrub()
{
	if (!HasAuthority())
	{
		return;
	}

	bool bChanged = false;
	for (FHordeTrackedEnemy& Tracked : TrackedEnemies)
	{
		if (Tracked.bCountedDead)
		{
			continue;
		}
		APawn* Pawn = Tracked.Pawn.Get();
		if (!Pawn || !IsValid(Pawn) || Pawn->IsActorBeingDestroyed())
		{
			NotifyEnemyDefeated(Tracked, true);
			bChanged = true;
		}
	}

	if (bChanged || true)
	{
		PushCountsToGameState();
	}

	if (IsWaveFullyCleared() && !bWaveClearBroadcast)
	{
		bWaveClearBroadcast = true;
		OnWaveCleared.Broadcast();
	}
}

void AHordeEncounterDirector::PushCountsToGameState() const
{
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->EnemiesAlive = AliveCount;
		GS->EnemiesRemainingToSpawn = RemainingToSpawn;
		GS->EnemiesDefeated = DefeatedCount;
		GS->BroadcastWaveStatus();
	}
}

AHordeGameState* AHordeEncounterDirector::GetHordeGS() const
{
	return GetWorld() ? GetWorld()->GetGameState<AHordeGameState>() : nullptr;
}
