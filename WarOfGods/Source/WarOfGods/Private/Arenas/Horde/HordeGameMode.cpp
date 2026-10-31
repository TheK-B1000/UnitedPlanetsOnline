// White Tiger Studios 2024

#include "Arenas/Horde/HordeGameMode.h"

#include "Arenas/ArenaGrayboxBuilder.h"
#include "Arenas/Horde/HordeEncounterDirector.h"
#include "Arenas/Horde/HordeGameState.h"
#include "Arenas/Horde/HordeHUD.h"
#include "Arenas/Horde/HordeMatchDefinition.h"
#include "Arenas/Horde/HordePlayerState.h"
#include "Arenas/Horde/HordeSpawnPoint.h"
#include "Arenas/Horde/HordeWaveDefinition.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "UI/Widgets/GOTAHordeHUDWidget.h"
#include "UI/Widgets/GOTAHordeResultsWidget.h"

AHordeGameMode::AHordeGameMode()
{
	GameStateClass = AHordeGameState::StaticClass();
	PlayerStateClass = AHordePlayerState::StaticClass();
	HUDClass = AHordeHUD::StaticClass();
	ArenaMode = EArenaMode::PvEHorde;
	ArenaId = EArenaId::TitansQuarry;
	GrayboxLayout = EArenaGrayboxLayout::TitansQuarry;
	bAutoStartEncounterDirector = false; // Horde owns encounters.
	SoftMaxPlayers = 4;

	SoftRangedEnemyClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/Enemies/BP_Enemy_Ranged.BP_Enemy_Ranged_C")));
	SoftEliteEnemyClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/Enemies/MeleeEnemy/BP_Enemy_Melee.BP_Enemy_Melee_C")));
	SoftBossEnemyClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/Enemies/BossEnemy/BP_Enemy_Boss.BP_Enemy_Boss_C")));
	// Prefer authored WBPs when present; C++ UGOTAHorde*Widget is the runtime fallback.
	HordeHudWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/Horde/WBP_GOTA_HordeHUD.WBP_GOTA_HordeHUD_C")));
	HordeResultsWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/Horde/WBP_GOTA_HordeResults.WBP_GOTA_HordeResults_C")));
}

void AHordeGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	ArenaMode = EArenaMode::PvEHorde;
	Super::InitGame(MapName, Options, ErrorMessage);
	ArenaMode = EArenaMode::PvEHorde;

	if (UGameplayStatics::HasOption(Options, TEXT("Mode")))
	{
		const FString ModeOpt = UGameplayStatics::ParseOption(Options, TEXT("Mode"));
		if (ModeOpt.Equals(TEXT("Horde"), ESearchCase::IgnoreCase)
			|| ModeOpt.Equals(TEXT("PvEHorde"), ESearchCase::IgnoreCase)
			|| ModeOpt.Equals(TEXT("Wrath"), ESearchCase::IgnoreCase))
		{
			ArenaMode = EArenaMode::PvEHorde;
		}
	}

	bHordeSmokeTest = UGameplayStatics::HasOption(Options, TEXT("HordeSmoke"));
	if (bHordeSmokeTest)
	{
		AutoStartDelay = 0.25f;
		bAutoStartWhenPlayersReady = true;
		UE_LOG(LogTemp, Log, TEXT("HordeSmoke: enabled (fast prep + auto ForceClear wave 1)"));
	}
}

void AHordeGameMode::BeginPlay()
{
	bAutoStartEncounterDirector = false;
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	EnsureHordeInfrastructure();
	ResolveHordeEnemyClasses();
	BuildRuntimeMatchDefinition();

	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->ActiveMode = EArenaMode::PvEHorde;
		GS->ActiveArenaId = ArenaId;
		GS->TotalRegularWaves = RuntimeMatchDef ? RuntimeMatchDef->Waves.Num() : 5;
		GS->MatchStartServerTime = GS->GetServerWorldTimeSeconds();
		// PostLogin can race before BeginPlay — never clobber an already-started match.
		if (!bMatchStarted)
		{
			GS->SetHordePhase(EHordeMatchPhase::WaitingForPlayers);
		}
		UE_LOG(LogTemp, Log, TEXT("Horde BeginPlay arena=%d waves=%d director=%s"),
			static_cast<int32>(ArenaId),
			GS->TotalRegularWaves,
			HordeDirector ? TEXT("ok") : TEXT("null"));
	}

	GetWorldTimerManager().SetTimer(
		PlayerWatchTimerHandle,
		this,
		&AHordeGameMode::EvaluateTeamWipe,
		1.f,
		true);

	bHordeInfrastructureReady = true;

	if (bHordeSmokeTest && !bMatchStarted && GetNumPlayers() >= 1)
	{
		// NullRHI/-game may not tick timers reliably — run wave1 spawn+clear synchronously.
		RequestStartMatch();
		GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
		BeginNextWave();
		if (HordeDirector)
		{
			HordeDirector->FlushSpawnQueueNow();
		}
		RunHordeSmokeStep();
	}
	else if (bAutoStartWhenPlayersReady && !bMatchStarted && GetNumPlayers() >= 1)
	{
		GetWorldTimerManager().SetTimer(
			PhaseTimerHandle,
			this,
			&AHordeGameMode::RequestStartMatch,
			AutoStartDelay,
			false);
	}
}

void AHordeGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Skip ArenaGameMode's immediate Active transition.
	AGameModeBase::PostLogin(NewPlayer);
	PresentHudToPlayer(NewPlayer);

	if (AHordeGameState* GS = GetHordeGS())
	{
		UE_LOG(LogTemp, Log, TEXT("Horde PostLogin players=%d phase=%d infra=%d"),
			GetNumPlayers(), (int32)GS->HordePhase, bHordeInfrastructureReady ? 1 : 0);
	}

	// Only start after BeginPlay built director + wave defs (PostLogin can fire first).
	if (bHordeInfrastructureReady && bAutoStartWhenPlayersReady && !bMatchStarted && GetNumPlayers() >= 1)
	{
		RequestStartMatch();
	}
}

void AHordeGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	// Disconnects are not combat eliminations — reevaluate wipe next tick.
}

void AHordeGameMode::EnsureHordeInfrastructure()
{
	EnsureDefaultSpawnPoints();
	EnsureHordeNavBounds();

	for (TActorIterator<AHordeEncounterDirector> It(GetWorld()); It; ++It)
	{
		HordeDirector = *It;
		break;
	}
	if (!HordeDirector)
	{
		FActorSpawnParameters Params;
		HordeDirector = GetWorld()->SpawnActor<AHordeEncounterDirector>(
			AHordeEncounterDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}
	if (HordeDirector)
	{
		// Avoid double-binding if Ensure is called more than once.
		HordeDirector->OnWaveCleared.RemoveAll(this);
		HordeDirector->OnEnemyDefeated.RemoveAll(this);
		HordeDirector->OnWaveCleared.AddDynamic(this, &AHordeGameMode::OnDirectorWaveCleared);
		HordeDirector->OnEnemyDefeated.AddDynamic(this, &AHordeGameMode::OnDirectorEnemyDefeated);
		if (SpawnedGraybox)
		{
			HordeDirector->SetActorLocation(SpawnedGraybox->GetActorLocation());
		}
	}
}

void AHordeGameMode::EnsureHordeNavBounds()
{
	for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ANavMeshBoundsVolume* Bounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(
		ANavMeshBoundsVolume::StaticClass(), FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, Params);
	if (Bounds)
	{
		Bounds->SetActorScale3D(FVector(50.f, 50.f, 20.f)); // ~5000uu footprint for Quarry pit
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			NavSys->Build();
			UE_LOG(LogTemp, Log, TEXT("Horde: spawned NavMeshBoundsVolume and requested nav build"));
		}
	}
}

void AHordeGameMode::ResolveHordeEnemyClasses()
{
	auto ResolveOrFallback = [](TSoftClassPtr<APawn>& Soft, const TSoftClassPtr<APawn>& Fallback, const TCHAR* Label)
	{
		if (Soft.IsNull())
		{
			Soft = Fallback;
			return;
		}
		if (!Soft.LoadSynchronous())
		{
			UE_LOG(LogTemp, Warning, TEXT("Horde: %s class missing (%s) — falling back to melee"), Label, *Soft.ToString());
			Soft = Fallback;
		}
	};

	if (SoftEnemyClass.IsNull() || !SoftEnemyClass.LoadSynchronous())
	{
		SoftEnemyClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/Enemies/MeleeEnemy/BP_Enemy_Melee.BP_Enemy_Melee_C")));
	}

	ResolveOrFallback(SoftRangedEnemyClass, SoftEnemyClass, TEXT("Ranged"));
	ResolveOrFallback(SoftEliteEnemyClass, SoftEnemyClass, TEXT("Elite"));
	ResolveOrFallback(SoftBossEnemyClass, SoftEnemyClass, TEXT("Boss"));
}

void AHordeGameMode::EnsureDefaultSpawnPoints()
{
	int32 Existing = 0;
	for (TActorIterator<AHordeSpawnPoint> It(GetWorld()); It; ++It)
	{
		++Existing;
	}
	if (Existing > 0)
	{
		return;
	}

	const TArray<FVector> Offsets = {
		FVector(900.f, 200.f, 100.f),
		FVector(-900.f, 300.f, 100.f),
		FVector(200.f, 1100.f, 100.f),
		FVector(-300.f, -1000.f, 100.f),
		FVector(700.f, -700.f, 100.f),
		FVector(-700.f, 900.f, 100.f),
		FVector(0.f, 1400.f, 100.f)
	};

	for (int32 i = 0; i < Offsets.Num(); ++i)
	{
		FActorSpawnParameters Params;
		AHordeSpawnPoint* Point = GetWorld()->SpawnActor<AHordeSpawnPoint>(
			AHordeSpawnPoint::StaticClass(), Offsets[i], FRotator(0.f, 180.f, 0.f), Params);
		if (Point)
		{
			Point->SpawnGroup = TEXT("Quarry");
			Point->bBossEntrance = (i == Offsets.Num() - 1);
			if (Point->bBossEntrance)
			{
				Point->AllowedRoles = { EHordeEnemyRole::Boss, EHordeEnemyRole::Elite };
			}
		}
	}
}

void AHordeGameMode::BuildRuntimeMatchDefinition()
{
	if (UHordeMatchDefinition* Loaded = MatchDefinitionAsset.LoadSynchronous())
	{
		RuntimeMatchDef = Loaded;
		return;
	}

	RuntimeWaves.Reset();
	RuntimeWaves.Add(MakeRuntimeWave(TEXT("W1"), NSLOCTEXT("GOTA", "W1", "Wave 1 — First Blood"), 4, 0, 0, false, 6));
	RuntimeWaves.Add(MakeRuntimeWave(TEXT("W2"), NSLOCTEXT("GOTA", "W2", "Wave 2 — Crossfire"), 3, 2, 0, false, 7));
	RuntimeWaves.Add(MakeRuntimeWave(TEXT("W3"), NSLOCTEXT("GOTA", "W3", "Wave 3 — Pressure"), 4, 2, 0, false, 8));
	RuntimeWaves.Add(MakeRuntimeWave(TEXT("W4"), NSLOCTEXT("GOTA", "W4", "Wave 4 — Champion"), 3, 2, 1, false, 8));
	RuntimeWaves.Add(MakeRuntimeWave(TEXT("W5"), NSLOCTEXT("GOTA", "W5", "Wave 5 — Onslaught"), 5, 3, 0, false, 10));
	RuntimeBossWave = MakeRuntimeWave(TEXT("Boss"), NSLOCTEXT("GOTA", "Boss", "Sanctuary Guardian"), 0, 0, 0, true, 4);

	RuntimeMatchDef = NewObject<UHordeMatchDefinition>(this, TEXT("RuntimeWrathMatch"));
	RuntimeMatchDef->DisplayName = NSLOCTEXT("GOTA", "WrathOfTheGods", "Wrath of the Gods");
	RuntimeMatchDef->ArenaId = ArenaId;
	RuntimeMatchDef->Waves = RuntimeWaves;
	RuntimeMatchDef->BossWave = RuntimeBossWave;
	RuntimeMatchDef->MatchPrepareSeconds = 5.f;
	RuntimeMatchDef->IntermissionSeconds = 8.f;
}

UHordeWaveDefinition* AHordeGameMode::MakeRuntimeWave(FName Id, FText Name, int32 Melee, int32 Ranged, int32 Elite, bool bBoss, int32 Concurrent)
{
	UHordeWaveDefinition* Wave = NewObject<UHordeWaveDefinition>(this, Id);
	Wave->WaveId = Id;
	Wave->DisplayName = Name;
	Wave->MaxConcurrentEnemies = Concurrent;
	Wave->MaxConcurrentPerExtraPlayer = 2;
	Wave->bIsBossWave = bBoss;
	Wave->CompletionBonusScore = bBoss ? 1500 : 250;

	auto AddEntry = [&](TSoftClassPtr<APawn> Class, EHordeEnemyRole EnemyRole, int32 Count, int32 Score)
	{
		if (Count <= 0)
		{
			return;
		}
		FHordeEnemySpawnEntry Entry;
		Entry.EnemyClass = Class;
		Entry.Role = EnemyRole;
		Entry.Count = Count;
		Entry.CountPerExtraPlayer = (EnemyRole == EHordeEnemyRole::Boss) ? 0 : 1;
		Entry.SpawnInterval = bBoss ? 0.1f : 0.4f;
		Entry.ScoreValue = Score;
		Wave->EnemyEntries.Add(Entry);
	};

	if (bBoss)
	{
		AddEntry(SoftBossEnemyClass.IsNull() ? SoftEnemyClass : SoftBossEnemyClass, EHordeEnemyRole::Boss, 1, 2500);
		AddEntry(SoftEnemyClass, EHordeEnemyRole::Grunt, 2, 100);
	}
	else
	{
		AddEntry(SoftEnemyClass, EHordeEnemyRole::Grunt, Melee, 100);
		AddEntry(SoftRangedEnemyClass.IsNull() ? SoftEnemyClass : SoftRangedEnemyClass, EHordeEnemyRole::Ranged, Ranged, 125);
		AddEntry(SoftEliteEnemyClass.IsNull() ? SoftEnemyClass : SoftEliteEnemyClass, EHordeEnemyRole::Elite, Elite, 400);
	}
	return Wave;
}

void AHordeGameMode::RequestStartMatch()
{
	if (!HasAuthority() || bMatchStarted || bMatchEnded)
	{
		return;
	}
	if (GetNumPlayers() < 1)
	{
		return;
	}
	if (!bHordeInfrastructureReady || !HordeDirector || !RuntimeMatchDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("Horde RequestStartMatch deferred — infra not ready"));
		return;
	}
	bMatchStarted = true;
	NextWaveToStart = 0;
	UE_LOG(LogTemp, Log, TEXT("Horde RequestStartMatch -> Preparing"));
	BeginPreparing();
}

void AHordeGameMode::RequestRestartMatch()
{
	if (!HasAuthority())
	{
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName()), true, TEXT("game=/Script/WarOfGods.HordeGameMode?Mode=Horde?listen"));
}

void AHordeGameMode::RequestReturnToLobby()
{
	if (!HasAuthority())
	{
		return;
	}
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->SetHordePhase(EHordeMatchPhase::ReturningToLobby);
	}
	const FString Package = LobbyMap.GetLongPackageName();
	UGameplayStatics::OpenLevel(this, FName(*Package), true);
}

bool AHordeGameMode::TryTransition(EHordeMatchPhase NewPhase)
{
	AHordeGameState* GS = GetHordeGS();
	if (!GS || (bMatchEnded && NewPhase != EHordeMatchPhase::ReturningToLobby))
	{
		return false;
	}
	GS->SetHordePhase(NewPhase);
	return true;
}

void AHordeGameMode::BeginPreparing()
{
	TryTransition(EHordeMatchPhase::Preparing);
	float Prep = RuntimeMatchDef ? RuntimeMatchDef->MatchPrepareSeconds : 5.f;
	if (bHordeSmokeTest)
	{
		Prep = 0.5f;
	}
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->PhaseDeadlineServerTime = GS->GetServerWorldTimeSeconds() + Prep;
	}
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AHordeGameMode::BeginNextWave, Prep, false);
}

void AHordeGameMode::BeginNextWave()
{
	if (bMatchEnded || !HordeDirector || !RuntimeMatchDef)
	{
		return;
	}

	if (NextWaveToStart >= RuntimeMatchDef->Waves.Num())
	{
		BeginBossFinale();
		return;
	}

	UHordeWaveDefinition* Wave = RuntimeMatchDef->Waves[NextWaveToStart];
	++NextWaveToStart;

	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->CurrentWaveIndex = NextWaveToStart;
		GS->bBossEncounterActive = false;
		GS->BossHealthFraction = 1.f;
	}

	TryTransition(EHordeMatchPhase::WaveStarting);
	HordeDirector->ConfigureWave(Wave, GetActivePlayerCount());
	TryTransition(EHordeMatchPhase::WaveActive);
	HordeDirector->StartWaveSpawning();
}

void AHordeGameMode::RunHordeSmokeStep()
{
	if (!HasAuthority() || !bHordeSmokeTest)
	{
		return;
	}

	DevDumpStatus();
	const int32 Alive = HordeDirector ? HordeDirector->GetAliveCount() : 0;
	const int32 Failed = HordeDirector ? HordeDirector->GetFailedSpawnCount() : 0;
	const int32 Remaining = HordeDirector ? HordeDirector->GetRemainingToSpawn() : 0;

	if (Alive <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("HordeSmoke: FAIL - no enemies spawned (alive=%d remaining=%d failed=%d)"),
			Alive, Remaining, Failed);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("HordeSmoke: wave1 spawned alive=%d remaining=%d failed=%d - ForceClear"),
			Alive, Remaining, Failed);
		DevForceClearWave();
		DevDumpStatus();

		if (AHordeGameState* GS = GetHordeGS())
		{
			const bool bAdvanced = GS->HordePhase == EHordeMatchPhase::WaveCompleted
				|| GS->HordePhase == EHordeMatchPhase::Intermission
				|| GS->CurrentWaveIndex >= 1;
			UE_LOG(LogTemp, Log, TEXT("HordeSmoke: %s phase=%d wave=%d"),
				bAdvanced ? TEXT("PASS") : TEXT("FAIL"),
				static_cast<int32>(GS->HordePhase),
				GS->CurrentWaveIndex);
		}
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("HordeSmokeQuit")))
	{
		FPlatformMisc::RequestExit(false);
	}
}

void AHordeGameMode::BeginIntermission()
{
	TryTransition(EHordeMatchPhase::WaveCompleted);
	if (AHordeGameState* GS = GetHordeGS())
	{
		if (RuntimeMatchDef && RuntimeMatchDef->Waves.IsValidIndex(NextWaveToStart - 1))
		{
			if (UHordeWaveDefinition* Cleared = RuntimeMatchDef->Waves[NextWaveToStart - 1])
			{
				GS->AddTeamScore(Cleared->CompletionBonusScore);
			}
		}
	}

	if (RuntimeMatchDef && RuntimeMatchDef->bRespawnPlayersDuringIntermission)
	{
		RespawnEliminatedPlayers();
	}

	TryTransition(EHordeMatchPhase::Intermission);
	const float Wait = RuntimeMatchDef ? RuntimeMatchDef->IntermissionSeconds : 8.f;
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->PhaseDeadlineServerTime = GS->GetServerWorldTimeSeconds() + Wait;
	}
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &AHordeGameMode::BeginNextWave, Wait, false);
}

void AHordeGameMode::BeginBossFinale()
{
	if (!RuntimeMatchDef || !RuntimeMatchDef->BossWave || !HordeDirector)
	{
		CompleteVictory();
		return;
	}

	TryTransition(EHordeMatchPhase::BossStarting);
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->bBossEncounterActive = true;
		GS->BossHealthFraction = 1.f;
		GS->CurrentWaveIndex = GS->TotalRegularWaves + 1;
	}

	HordeDirector->ConfigureWave(RuntimeMatchDef->BossWave, GetActivePlayerCount());
	TryTransition(EHordeMatchPhase::BossActive);
	HordeDirector->StartWaveSpawning();
}

void AHordeGameMode::OnDirectorWaveCleared()
{
	HandleWaveCleared();
}

void AHordeGameMode::HandleWaveCleared()
{
	if (bMatchEnded)
	{
		return;
	}

	AHordeGameState* GS = GetHordeGS();
	if (!GS)
	{
		return;
	}

	if (GS->HordePhase == EHordeMatchPhase::BossActive)
	{
		if (RuntimeMatchDef)
		{
			GS->AddTeamScore(RuntimeMatchDef->VictoryBonusScore);
		}
		CompleteVictory();
		return;
	}

	if (GS->HordePhase == EHordeMatchPhase::WaveActive)
	{
		BeginIntermission();
	}
}

void AHordeGameMode::OnDirectorEnemyDefeated(int32 ScoreValue)
{
	HandleEnemyScore(ScoreValue);
}

void AHordeGameMode::HandleEnemyScore(int32 ScoreValue)
{
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->AddTeamScore(ScoreValue);
	}
	// Attribute to first non-eliminated player as a simple co-op share until kill credit RPCs exist.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (AHordePlayerState* PS = PC->GetPlayerState<AHordePlayerState>())
			{
				if (!PS->bIsEliminated)
				{
					PS->ServerAddElimination(ScoreValue >= 400, ScoreValue);
					break;
				}
			}
		}
	}
}

void AHordeGameMode::EvaluateTeamWipe()
{
	if (!HasAuthority() || bMatchEnded || !bMatchStarted)
	{
		return;
	}

	AHordeGameState* GS = GetHordeGS();
	if (!GS)
	{
		return;
	}
	if (GS->HordePhase != EHordeMatchPhase::WaveActive && GS->HordePhase != EHordeMatchPhase::BossActive)
	{
		return;
	}

	int32 Living = 0;
	int32 Considered = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		++Considered;
		AHordePlayerState* PS = PC->GetPlayerState<AHordePlayerState>();
		APawn* Pawn = PC->GetPawn();
		const bool bAlive = Pawn && IsValid(Pawn) && !Pawn->IsActorBeingDestroyed();
		if (PS)
		{
			PS->SetEliminated(!bAlive);
		}
		if (bAlive)
		{
			++Living;
		}
	}

	if (Considered > 0 && Living == 0)
	{
		CompleteDefeat();
	}
}

void AHordeGameMode::CompleteVictory()
{
	if (bMatchEnded)
	{
		return;
	}
	bMatchEnded = true;
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	if (HordeDirector)
	{
		HordeDirector->StopWaveSpawning();
		HordeDirector->CleanupTrackedEnemies(true);
	}
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->MatchOutcome = EHordeMatchOutcome::Victory;
		GS->OnHordeMatchOutcomeChanged.Broadcast(EHordeMatchOutcome::Victory);
	}
	TryTransition(EHordeMatchPhase::Victory);
	PresentResultsToAll();
}

void AHordeGameMode::CompleteDefeat()
{
	if (bMatchEnded)
	{
		return;
	}
	bMatchEnded = true;
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	if (HordeDirector)
	{
		HordeDirector->StopWaveSpawning();
		HordeDirector->CleanupTrackedEnemies(true);
	}
	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->MatchOutcome = EHordeMatchOutcome::Defeat;
		GS->OnHordeMatchOutcomeChanged.Broadcast(EHordeMatchOutcome::Defeat);
	}
	TryTransition(EHordeMatchPhase::Defeat);
	PresentResultsToAll();
}

void AHordeGameMode::RespawnEliminatedPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		AHordePlayerState* PS = PC->GetPlayerState<AHordePlayerState>();
		if (PS && PS->bIsEliminated)
		{
			RestartPlayer(PC);
			PS->SetEliminated(false);
		}
		else if (!PC->GetPawn())
		{
			RestartPlayer(PC);
		}
	}
}

UClass* AHordeGameMode::ResolveHudWidgetClass() const
{
	if (UClass* HudClass = HordeHudWidgetClass.LoadSynchronous())
	{
		return HudClass;
	}
	return UGOTAHordeHUDWidget::StaticClass();
}

UClass* AHordeGameMode::ResolveResultsWidgetClass() const
{
	if (UClass* ResultsClass = HordeResultsWidgetClass.LoadSynchronous())
	{
		return ResultsClass;
	}
	return UGOTAHordeResultsWidget::StaticClass();
}

void AHordeGameMode::PresentHudToPlayer(APlayerController* PC)
{
	if (!PC || !PC->IsLocalPlayerController())
	{
		return;
	}
	if (UClass* HudClass = ResolveHudWidgetClass())
	{
		if (UUserWidget* Hud = CreateWidget<UUserWidget>(PC, HudClass))
		{
			Hud->AddToViewport(50);
		}
	}
}

void AHordeGameMode::PresentResultsToAll()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->IsLocalPlayerController())
		{
			continue;
		}
		if (UClass* ResultsClass = ResolveResultsWidgetClass())
		{
			if (UUserWidget* Results = CreateWidget<UUserWidget>(PC, ResultsClass))
			{
				Results->AddToViewport(200);
			}
		}
	}
}

void AHordeGameMode::DevForceClearWave()
{
	if (!HasAuthority() || !HordeDirector)
	{
		return;
	}
	HordeDirector->ForceClearWave();
}

void AHordeGameMode::DevSkipPrepare()
{
	if (!HasAuthority())
	{
		return;
	}
	AHordeGameState* GS = GetHordeGS();
	if (!GS)
	{
		return;
	}
	if (GS->HordePhase == EHordeMatchPhase::WaitingForPlayers)
	{
		RequestStartMatch();
		GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
		BeginNextWave();
		return;
	}
	if (GS->HordePhase == EHordeMatchPhase::Preparing || GS->HordePhase == EHordeMatchPhase::Intermission)
	{
		GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
		BeginNextWave();
	}
}

void AHordeGameMode::DevDumpStatus() const
{
	const AHordeGameState* GS = GetHordeGS();
	const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : 0;
	UE_LOG(LogTemp, Log, TEXT("Horde Dump: players=%d started=%d ended=%d nextWave=%d"),
		PlayerCount, bMatchStarted ? 1 : 0, bMatchEnded ? 1 : 0, NextWaveToStart);
	if (GS)
	{
		UE_LOG(LogTemp, Log, TEXT("  phase=%d wave=%d/%d alive=%d toSpawn=%d defeated=%d score=%d outcome=%d"),
			static_cast<int32>(GS->HordePhase),
			GS->CurrentWaveIndex,
			GS->TotalRegularWaves,
			GS->EnemiesAlive,
			GS->EnemiesRemainingToSpawn,
			GS->EnemiesDefeated,
			GS->TeamScore,
			static_cast<int32>(GS->MatchOutcome));
	}
	if (HordeDirector)
	{
		UE_LOG(LogTemp, Log, TEXT("  director alive=%d remaining=%d defeated=%d failed=%d cleared=%d"),
			HordeDirector->GetAliveCount(),
			HordeDirector->GetRemainingToSpawn(),
			HordeDirector->GetDefeatedCount(),
			HordeDirector->GetFailedSpawnCount(),
			HordeDirector->IsWaveFullyCleared() ? 1 : 0);
	}
}

int32 AHordeGameMode::GetActivePlayerCount() const
{
	const int32 Count = GameState ? GameState->PlayerArray.Num() : 1;
	return FMath::Clamp(Count, 1, SoftMaxPlayers);
}

AHordeGameState* AHordeGameMode::GetHordeGS() const
{
	return GetGameState<AHordeGameState>();
}
