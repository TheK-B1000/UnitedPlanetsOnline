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
#include "TimerManager.h"

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
	BuildRuntimeMatchDefinition();

	if (AHordeGameState* GS = GetHordeGS())
	{
		GS->ActiveMode = EArenaMode::PvEHorde;
		GS->ActiveArenaId = ArenaId;
		GS->TotalRegularWaves = RuntimeMatchDef ? RuntimeMatchDef->Waves.Num() : 5;
		GS->MatchStartServerTime = GS->GetServerWorldTimeSeconds();
		GS->SetHordePhase(EHordeMatchPhase::WaitingForPlayers);
	}

	GetWorldTimerManager().SetTimer(
		PlayerWatchTimerHandle,
		this,
		&AHordeGameMode::EvaluateTeamWipe,
		1.f,
		true);

	if (bAutoStartWhenPlayersReady)
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
		UE_LOG(LogTemp, Log, TEXT("Horde PostLogin players=%d phase=%d"), GetNumPlayers(), (int32)GS->HordePhase);
	}

	if (bAutoStartWhenPlayersReady && !bMatchStarted && GetNumPlayers() >= 1)
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
		HordeDirector->OnWaveCleared.AddDynamic(this, &AHordeGameMode::OnDirectorWaveCleared);
		HordeDirector->OnEnemyDefeated.AddDynamic(this, &AHordeGameMode::OnDirectorEnemyDefeated);
		if (SpawnedGraybox)
		{
			HordeDirector->SetActorLocation(SpawnedGraybox->GetActorLocation());
		}
	}
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
	bMatchStarted = true;
	NextWaveToStart = 0;
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
	const float Prep = RuntimeMatchDef ? RuntimeMatchDef->MatchPrepareSeconds : 5.f;
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

void AHordeGameMode::PresentHudToPlayer(APlayerController* PC)
{
	if (!PC || PC->IsLocalController() == false && !PC->IsLocalPlayerController())
	{
		// Still create on owning client via local PC only.
	}
	if (!PC || !PC->IsLocalPlayerController())
	{
		return;
	}
	if (UClass* HudClass = HordeHudWidgetClass.LoadSynchronous())
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
		if (UClass* ResultsClass = HordeResultsWidgetClass.LoadSynchronous())
		{
			if (UUserWidget* Results = CreateWidget<UUserWidget>(PC, ResultsClass))
			{
				Results->AddToViewport(200);
			}
		}
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
