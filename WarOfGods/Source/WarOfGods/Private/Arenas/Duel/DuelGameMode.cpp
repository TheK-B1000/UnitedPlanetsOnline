// White Tiger Studios 2024

#include "Arenas/Duel/DuelGameMode.h"

#include "Arenas/ArenaPlayerStart.h"
#include "Arenas/Duel/DuelFighterInterface.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Arenas/Duel/DuelHUD.h"
#include "Arenas/Duel/DuelMatchDefinition.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "Characters/GOTAHeroDefinition.h"
#include "Characters/GOTAHeroRegistry.h"
#include "Characters/GOTAHeroBodyComponent.h"
#include "Abilities/GOTAHeroAbilityComponent.h"
#include "Bosses/GOTABossComponent.h"
#include "Bosses/GOTABossTypes.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Characters/StatsComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "AIController.h"
#include "EnvironmentQuery/EQSTestingPawn.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "BrainComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "Combat/CombatComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADuelGameMode::ADuelGameMode()
{
	GameStateClass = ADuelGameState::StaticClass();
	PlayerStateClass = ADuelPlayerState::StaticClass();
	HUDClass = ADuelHUD::StaticClass();
	ArenaMode = EArenaMode::PvPDuel;
	ArenaId = EArenaId::TitansQuarry;
	GrayboxLayout = EArenaGrayboxLayout::TitansQuarry;
	SoftMaxPlayers = 2;
	bAutoStartEncounterDirector = false;

	DuelHudWidgetClass = TSoftClassPtr<UUserWidget>(
		FSoftObjectPath(TEXT("/Game/UI/Duel/WBP_GOTA_DuelHUD.WBP_GOTA_DuelHUD_C")));
	DuelResultsWidgetClass = TSoftClassPtr<UUserWidget>(
		FSoftObjectPath(TEXT("/Game/UI/Duel/WBP_GOTA_DuelResults.WBP_GOTA_DuelResults_C")));
	AIChampionClass = TSoftClassPtr<APawn>(
		FSoftObjectPath(TEXT("/Game/Enemies/MeleeEnemy/BP_Enemy_Melee.BP_Enemy_Melee_C")));
}

void ADuelGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	ArenaMode = EArenaMode::PvPDuel;
	SoftMaxPlayers = 2;
	Super::InitGame(MapName, Options, ErrorMessage);
	ArenaMode = EArenaMode::PvPDuel;
	SoftMaxPlayers = 2;
	bAutoStartEncounterDirector = false;

	bVersusAI = UGameplayStatics::ParseOption(Options, TEXT("Opponent")).Equals(TEXT("AI"), ESearchCase::IgnoreCase);
#if !UE_BUILD_SHIPPING
	// Testing only: ?DevHeroes=1 lets In Development heroes spawn (see UGOTAHeroRegistry::IsHeroSpawnable).
	if (UGameplayStatics::GetIntOption(Options, TEXT("DevHeroes"), 0) != 0)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("gota.Heroes.AllowInDevelopment")))
		{
			CVar->Set(1, ECVF_SetByCode);
			UE_LOG(LogTemp, Warning, TEXT("Duel: DevHeroes=1 — In Development heroes may spawn (testing only)"));
		}
	}
#endif
	if (bVersusAI)
	{
		UE_LOG(LogTemp, Log, TEXT("Duel: Fight AI mode (champion %s)"), *AIChampionClass.ToString());
	}

#if !UE_BUILD_SHIPPING
	// Testing: the AI champion plays a hero (body + divine abilities) — ?Foe=Varoth.
	FoeHeroId = FName(*UGameplayStatics::ParseOption(Options, TEXT("Foe")));
	if (!FoeHeroId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Duel: AI champion plays hero %s (testing)"), *FoeHeroId.ToString());
	}
#endif
	// Boss encounter: ?Boss=<Id> -> /Game/Bosses/BP_Boss_<Id> + /Game/Bosses/DA_Boss_<Id>.
	{
		const FString BossId = UGameplayStatics::ParseOption(Options, TEXT("Boss"));
		if (!BossId.IsEmpty())
		{
			BossDefinition = LoadObject<UGOTABossDefinition>(nullptr, *FString::Printf(TEXT("/Game/Bosses/DA_Boss_%s.DA_Boss_%s"), *BossId, *BossId));
			if (BossDefinition)
			{
				bVersusAI = true;
				AIChampionClass = TSoftClassPtr<APawn>(FSoftObjectPath(FString::Printf(TEXT("/Game/Bosses/BP_Boss_%s.BP_Boss_%s_C"), *BossId, *BossId)));
				UE_LOG(LogTemp, Log, TEXT("Duel: BOSS encounter %s (%s)"), *BossId, *AIChampionClass.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Duel: unknown boss '%s' — falling back to the standard Fight AI"), *BossId);
			}
		}
	}
	if (UGameplayStatics::HasOption(Options, TEXT("Difficulty")))
	{
		Difficulty = FGOTAChampionProfile::ParseDifficulty(UGameplayStatics::ParseOption(Options, TEXT("Difficulty")), Difficulty);
	}

	if (UGameplayStatics::HasOption(Options, TEXT("BestOf")))
	{
		const int32 BestOf = FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("BestOf")));
		if (BestOf >= 1)
		{
			BestOfRounds = BestOf;
			RoundsToWin = (BestOf / 2) + 1;
		}
	}
}

void ADuelGameMode::BeginPlay()
{
	bAutoStartEncounterDirector = false;
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	ApplyMatchDefinition();
	if (BossDefinition)
	{
		// A boss encounter is one fight: boss death (or the player's) ends it. No best-of-three.
		BestOfRounds = 1;
		RoundsToWin = 1;
		RoundDurationSeconds = 600.f;
	}

	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->ActiveMode = EArenaMode::PvPDuel;
		GS->ActiveArenaId = ArenaId;
		GS->RoundsToWin = RoundsToWin;
		GS->BestOfRounds = BestOfRounds;
		GS->CurrentRound = 0;
		GS->ScorePlayer0 = 0;
		GS->ScorePlayer1 = 0;
		GS->MatchOutcome = EDuelMatchOutcome::None;
		GS->bCombatEnabled = false;
		GS->Difficulty = Difficulty;
		GS->SetDuelPhase(EDuelPhase::WaitingForPlayers);
	}

	GetWorldTimerManager().SetTimer(
		RoundWatchTimerHandle,
		this,
		&ADuelGameMode::EvaluateRound,
		0.25f,
		true);

	if (bVersusAI)
	{
		EnsureAIParticipant();
		EnsureDuelNavigation();
		RemoveDeveloperPawns();
	}
	// Local hosts log in before BeginPlay; start here once the arena (and its starts) exist.
	if (bAutoStartWhenFull && GetNumPlayers() >= GetRequiredHumanPlayers())
	{
		RequestStartMatch();
	}
}

void ADuelGameMode::ApplyMatchDefinition()
{
	RuntimeMatchDef = MatchDefinitionAsset.LoadSynchronous();
	if (!RuntimeMatchDef)
	{
		RuntimeMatchDef = NewObject<UDuelMatchDefinition>(this, TEXT("RuntimeTrialRules"));
		RuntimeMatchDef->BestOfRounds = BestOfRounds;
		RuntimeMatchDef->CountdownSeconds = CountdownSeconds;
		RuntimeMatchDef->RoundDurationSeconds = RoundDurationSeconds;
		RuntimeMatchDef->RoundEndDelaySeconds = RoundEndDelay;
		RuntimeMatchDef->IntermissionSeconds = IntermissionSeconds;
		RuntimeMatchDef->bTimeoutAwardHigherHealth = bTimeoutAwardHigherHealth;
		RuntimeMatchDef->NormalizedMaxHealth = NormalizedMaxHealth;
		RuntimeMatchDef->ArenaId = ArenaId;
	}

	BestOfRounds = RuntimeMatchDef->BestOfRounds;
	RoundsToWin = RuntimeMatchDef->GetRoundsToWin();
	CountdownSeconds = RuntimeMatchDef->CountdownSeconds;
	RoundDurationSeconds = RuntimeMatchDef->RoundDurationSeconds;
	RoundEndDelay = RuntimeMatchDef->RoundEndDelaySeconds;
	IntermissionSeconds = RuntimeMatchDef->IntermissionSeconds;
	bTimeoutAwardHigherHealth = RuntimeMatchDef->bTimeoutAwardHigherHealth;
	NormalizedMaxHealth = RuntimeMatchDef->NormalizedMaxHealth;
	ArenaId = RuntimeMatchDef->ArenaId;
}

void ADuelGameMode::PostLogin(APlayerController* NewPlayer)
{
	AGameModeBase::PostLogin(NewPlayer);
	AssignDuelSlots();
	PresentHudToPlayer(NewPlayer);

	if (NewPlayer && NewPlayer->GetPawn())
	{
		BindCombatDeath(NewPlayer->GetPawn());
		ResetFighterForRound(NewPlayer->GetPawn());
	}

	if (ADuelGameState* GS = GetDuelGS())
	{
		UE_LOG(LogTemp, Log, TEXT("Duel PostLogin players=%d/%d phase=%d"),
			GetNumPlayers(), SoftMaxPlayers, static_cast<int32>(GS->DuelPhase));
	}

	if (bVersusAI && GetNumPlayers() > GetRequiredHumanPlayers())
	{
		UE_LOG(LogTemp, Warning, TEXT("Duel Fight AI accepts one human; extra player will spectate."));
	}
	if (bAutoStartWhenFull && HasActorBegunPlay() && GetNumPlayers() >= GetRequiredHumanPlayers())
	{
		RequestStartMatch();
	}
}

void ADuelGameMode::Logout(AController* Exiting)
{
	if (Exiting && Exiting->GetPawn())
	{
		UnbindCombatDeath(Exiting->GetPawn());
	}

	Super::Logout(Exiting);
	if (!HasAuthority() || bMatchComplete)
	{
		return;
	}
	if (ADuelGameState* GS = GetDuelGS())
	{
		if (GS->DuelPhase == EDuelPhase::InRound
			|| GS->DuelPhase == EDuelPhase::Countdown
			|| GS->DuelPhase == EDuelPhase::PreparingRound)
		{
			ADuelPlayerState* Leaving = GetDuelPS(Exiting);
			const int32 WinnerSlot = (Leaving && Leaving->DuelSlot == 0) ? 1 : 0;
			CompleteMatch(WinnerSlot == 0 ? EDuelMatchOutcome::Player0Wins : EDuelMatchOutcome::Player1Wins);
		}
	}
}

AActor* ADuelGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	ADuelPlayerState* PS = GetDuelPS(Player);
	const int32 Desired = PS && PS->DuelSlot != INDEX_NONE ? PS->DuelSlot : NextPlayerSlot % SoftMaxPlayers;

	const int32 PreferredStartSlot = (Desired == 0) ? 0 : 2;
	for (TActorIterator<AArenaPlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerSlot == PreferredStartSlot)
		{
			++NextPlayerSlot;
			return *It;
		}
	}
	for (TActorIterator<AArenaPlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerSlot == Desired)
		{
			++NextPlayerSlot;
			return *It;
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

void ADuelGameMode::AssignDuelSlots()
{
	if (!HasAuthority())
	{
		return;
	}
	int32 NextSlot = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (ADuelPlayerState* PS = GetDuelPS(PC))
			{
				if (PS->DuelSlot == INDEX_NONE && NextSlot < SoftMaxPlayers)
				{
					PS->DuelSlot = NextSlot++;
				}
			}
		}
	}
}

void ADuelGameMode::RequestStartMatch()
{
	if (!HasAuthority() || bMatchComplete)
	{
		return;
	}
	ADuelGameState* GS = GetDuelGS();
	if (!GS)
	{
		return;
	}
	if (GS->DuelPhase != EDuelPhase::WaitingForPlayers
		&& GS->DuelPhase != EDuelPhase::WaitingForReady
		&& GS->DuelPhase != EDuelPhase::MatchEnd)
	{
		return;
	}
	if (bRequireBothPlayers && GetNumPlayers() < GetRequiredHumanPlayers())
	{
		UE_LOG(LogTemp, Log, TEXT("Duel waiting for %d players (have %d)"), GetRequiredHumanPlayers(), GetNumPlayers());
		return;
	}
	if (bVersusAI)
	{
		EnsureAIParticipant();
	}

	AssignDuelSlots();
	GS->CurrentRound = 0;
	GS->ScorePlayer0 = 0;
	GS->ScorePlayer1 = 0;
	GS->MatchOutcome = EDuelMatchOutcome::None;
	GS->LastRoundWinnerSlot = INDEX_NONE;
	GS->RoundsToWin = RoundsToWin;
	GS->BestOfRounds = BestOfRounds;
	GS->BroadcastScore();
	bMatchComplete = false;
	bRoundResolving = false;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ADuelPlayerState* PS = GetDuelPS(It->Get()))
		{
			PS->ResetMatchStats();
		}
	}
	if (AIPlayerState)
	{
		AIPlayerState->ResetMatchStats();
	}

	BeginPreparingRound();
}

void ADuelGameMode::BeginPreparingRound()
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS)
	{
		return;
	}

	++GS->CurrentRound;
	bRoundResolving = false;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ADuelPlayerState* PS = GetDuelPS(It->Get()))
		{
			PS->SetEliminatedThisRound(false);
		}
	}
	if (AIPlayerState)
	{
		AIPlayerState->SetEliminatedThisRound(false);
	}

	GS->SetDuelPhase(EDuelPhase::PreparingRound);
	SetCombatEnabledForAll(false);
	RespawnFighters();

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::BeginCountdown, 0.35f, false);
}

void ADuelGameMode::BeginCountdown()
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS)
	{
		return;
	}

	SetCombatEnabledForAll(false);
	SetAIBrainActive(false);
	GS->PhaseDeadlineServerTime = GS->GetServerWorldTimeSeconds() + CountdownSeconds;
	GS->RoundEndServerTime = 0.f;
	GS->SetDuelPhase(EDuelPhase::Countdown);

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().ClearTimer(RoundDurationTimerHandle);
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::BeginRound, CountdownSeconds, false);
}

void ADuelGameMode::BeginRound()
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS || bMatchComplete)
	{
		return;
	}

	bRoundResolving = false;
	SetCombatEnabledForAll(true);
	SetAIBrainActive(true);
	AlertChampionToChallenger();
	GS->RoundEndServerTime = GS->GetServerWorldTimeSeconds() + RoundDurationSeconds;
	GS->PhaseDeadlineServerTime = GS->RoundEndServerTime;
	GS->SetDuelPhase(EDuelPhase::InRound);

	GetWorldTimerManager().ClearTimer(RoundDurationTimerHandle);
	GetWorldTimerManager().SetTimer(
		RoundDurationTimerHandle,
		this,
		&ADuelGameMode::HandleRoundTimeout,
		RoundDurationSeconds,
		false);

	UE_LOG(LogTemp, Log, TEXT("Duel round %d ACTIVE (%.0fs)"), GS->CurrentRound, RoundDurationSeconds);
}

void ADuelGameMode::HandleRoundTimeout()
{
	if (!HasAuthority() || bMatchComplete || bRoundResolving)
	{
		return;
	}
	ADuelGameState* GS = GetDuelGS();
	if (!GS || GS->DuelPhase != EDuelPhase::InRound)
	{
		return;
	}

	APawn* Pawn0 = GetFighterPawn(0);
	APawn* Pawn1 = GetFighterPawn(1);
	const bool bAlive0 = IsPawnAlive(Pawn0);
	const bool bAlive1 = IsPawnAlive(Pawn1);

	if (bAlive0 && !bAlive1)
	{
		EndRound(0, true);
		return;
	}
	if (!bAlive0 && bAlive1)
	{
		EndRound(1, true);
		return;
	}

	if (bTimeoutAwardHigherHealth && bAlive0 && bAlive1)
	{
		const float H0 = GetObservedHealth(Pawn0);
		const float H1 = GetObservedHealth(Pawn1);
		if (H0 > H1 + KINDA_SMALL_NUMBER)
		{
			EndRound(0, true);
			return;
		}
		if (H1 > H0 + KINDA_SMALL_NUMBER)
		{
			EndRound(1, true);
			return;
		}
	}

	// Draw by timeout — no round point.
	UE_LOG(LogTemp, Log, TEXT("Duel round %d timeout draw"), GS->CurrentRound);
	EndRound(INDEX_NONE, false);
}

void ADuelGameMode::EvaluateRound()
{
	if (!HasAuthority() || bMatchComplete || bRoundResolving)
	{
		return;
	}
	ADuelGameState* GS = GetDuelGS();
	if (!GS || GS->DuelPhase != EDuelPhase::InRound)
	{
		return;
	}

	const bool bAlive0 = IsPawnAlive(GetFighterPawn(0));
	const bool bAlive1 = IsPawnAlive(GetFighterPawn(1));

	if (ADuelPlayerState* PS0 = GetPSForSlot(0)) { PS0->SetEliminatedThisRound(!bAlive0); }
	if (ADuelPlayerState* PS1 = GetPSForSlot(1)) { PS1->SetEliminatedThisRound(!bAlive1); }

	if (bAlive0 && !bAlive1)
	{
		EndRound(0, true);
	}
	else if (!bAlive0 && bAlive1)
	{
		EndRound(1, true);
	}
	else if (!bAlive0 && !bAlive1)
	{
		// Simultaneous KO — documented draw: no score, intermission then next round.
		UE_LOG(LogTemp, Log, TEXT("Duel simultaneous KO — no score"));
		EndRound(INDEX_NONE, false);
	}
}

void ADuelGameMode::EndRound(int32 WinningSlot, bool bScored)
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS || GS->DuelPhase != EDuelPhase::InRound || bRoundResolving)
	{
		return;
	}

	bRoundResolving = true;
	GetWorldTimerManager().ClearTimer(RoundDurationTimerHandle);
	SetCombatEnabledForAll(false);
	SetAIBrainActive(false);
	GS->SetDuelPhase(EDuelPhase::RoundEnd);
	GS->LastRoundWinnerSlot = WinningSlot;

	if (bScored && WinningSlot == 0)
	{
		++GS->ScorePlayer0;
		if (ADuelPlayerState* PS = GetPSForSlot(0))
		{
			PS->AddRoundWin();
			PS->AddElimination();
		}
		if (ADuelPlayerState* PS = GetPSForSlot(1))
		{
			PS->AddDeath();
		}
	}
	else if (bScored && WinningSlot == 1)
	{
		++GS->ScorePlayer1;
		if (ADuelPlayerState* PS = GetPSForSlot(1))
		{
			PS->AddRoundWin();
			PS->AddElimination();
		}
		if (ADuelPlayerState* PS = GetPSForSlot(0))
		{
			PS->AddDeath();
		}
	}
	GS->BroadcastScore();

	UE_LOG(LogTemp, Log, TEXT("Duel round %d end winner=%d score %d:%d (to %d)"),
		GS->CurrentRound, WinningSlot, GS->ScorePlayer0, GS->ScorePlayer1, GS->RoundsToWin);

	if (GS->ScorePlayer0 >= GS->RoundsToWin)
	{
		PendingOutcome = EDuelMatchOutcome::Player0Wins;
		GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::FinishPendingMatch, RoundEndDelay, false);
		return;
	}
	if (GS->ScorePlayer1 >= GS->RoundsToWin)
	{
		PendingOutcome = EDuelMatchOutcome::Player1Wins;
		GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::FinishPendingMatch, RoundEndDelay, false);
		return;
	}

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::BeginIntermission, RoundEndDelay, false);
}

void ADuelGameMode::BeginIntermission()
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS || bMatchComplete)
	{
		return;
	}
	GS->PhaseDeadlineServerTime = GS->GetServerWorldTimeSeconds() + IntermissionSeconds;
	GS->SetDuelPhase(EDuelPhase::Intermission);
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::BeginPreparingRound, IntermissionSeconds, false);
}

void ADuelGameMode::FinishPendingMatch()
{
	CompleteMatch(PendingOutcome);
}

void ADuelGameMode::CompleteMatch(EDuelMatchOutcome Outcome)
{
	if (bMatchComplete)
	{
		return;
	}
	bMatchComplete = true;
	bRoundResolving = false;
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().ClearTimer(RoundDurationTimerHandle);
	SetCombatEnabledForAll(false);
	SetAIBrainActive(false);

	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->MatchOutcome = Outcome;
		GS->OnDuelMatchOutcomeChanged.Broadcast(Outcome);
		GS->SetDuelPhase(EDuelPhase::MatchEnd);
	}
	PresentResultsToAll();
	UE_LOG(LogTemp, Log, TEXT("Duel match complete outcome=%d"), static_cast<int32>(Outcome));
}

void ADuelGameMode::RequestRematch()
{
	if (!HasAuthority())
	{
		return;
	}
	bMatchComplete = false;
	bRoundResolving = false;
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->MatchOutcome = EDuelMatchOutcome::None;
		GS->SetDuelPhase(EDuelPhase::WaitingForPlayers);
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ADuelPlayerState* PS = GetDuelPS(It->Get()))
		{
			PS->bWantsRematch = false;
		}
	}
	RequestStartMatch();
}

void ADuelGameMode::RequestReturnToLobby()
{
	if (!HasAuthority())
	{
		return;
	}
	ADuelGameState* GS = GetDuelGS();
	if (GS && GS->DuelPhase == EDuelPhase::ReturningToLobby)
	{
		return; // already travelling — ignore repeated presses
	}
	if (GS)
	{
		GS->SetDuelPhase(EDuelPhase::ReturningToLobby);
	}
	GetWorldTimerManager().ClearAllTimersForObject(this);
	DespawnAIFighter();
	const FString MapPackage = LobbyMap.GetLongPackageName();
	UGameplayStatics::OpenLevel(this, FName(*MapPackage), true);
}

void ADuelGameMode::RespawnFighters()
{
	// Every round starts from fresh pawns so health comes from the Blueprint damage
	// system's own defaults — C++ never writes health.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		ADuelPlayerState* PS = GetDuelPS(PC);
		if (!PC || !PS || PS->DuelSlot == INDEX_NONE)
		{
			continue;
		}
		RemovePawnOwnedHud(PC);
		if (APawn* Old = PC->GetPawn())
		{
			UnbindCombatDeath(Old);
			PC->UnPossess();
			DestroyFighter(Old);
		}
		RestartPlayer(PC);
		if (APawn* Pawn = PC->GetPawn())
		{
			if (AActor* Start = FindDuelStartForSlot(PS->DuelSlot))
			{
				Pawn->SetActorLocationAndRotation(Start->GetActorLocation(), Start->GetActorRotation(), false, nullptr, ETeleportType::ResetPhysics);
				PC->SetControlRotation(Start->GetActorRotation());
			}
			BindCombatDeath(Pawn);
			ResetFighterForRound(Pawn);
			EquipFighterLoadout(Pawn);
			PC->SetViewTarget(Pawn);
			Pawn->OnEndPlay.AddUniqueDynamic(this, &ADuelGameMode::HandleFighterEndPlay);
			UE_LOG(LogTemp, Log, TEXT("Duel fighter %s spawned at %s"), *GetNameSafe(Pawn), *Pawn->GetActorLocation().ToCompactString());
		}
	}

	if (bVersusAI)
	{
		SpawnAIFighter();
	}
	InitDuelCombat();
}

void ADuelGameMode::InitDuelCombat()
{
	APawn* Fighters[2] = { GetFighterPawn(0), GetFighterPawn(1) };
	UGOTADuelFighterComponent* Comps[2] = { nullptr, nullptr };
	for (int32 i = 0; i < 2; ++i)
	{
		if (!Fighters[i])
		{
			continue;
		}
		Comps[i] = Fighters[i]->FindComponentByClass<UGOTADuelFighterComponent>();
		if (!Comps[i])
		{
			Comps[i] = NewObject<UGOTADuelFighterComponent>(Fighters[i], TEXT("GOTA_DuelFighter"));
			Comps[i]->SetIsReplicated(true);
			Comps[i]->RegisterComponent();
		}
	}
	if (bVersusAI && Fighters[0] && Fighters[1])
	{
		// Spawn-time tuning of each fighter's own BPC_DamageSystem (before any damage). The
		// Blueprint remains the only thing that changes health during the round.
		const FGOTAChampionProfile Profile = FGOTAChampionProfile::ForDifficulty(Difficulty);
		FGOTABlueprintCombat::ConfigureHealth(Fighters[0], Profile.ChallengerMaxHealth);
		if (BossDefinition)
		{
			// Boss: its own definition drives body, health and brain; no Sovereign Champion director.
			UGOTABossComponent::ApplyToPawn(Fighters[1], BossDefinition);
			FGOTABlueprintCombat::ConfigureHealth(Fighters[1], BossDefinition->MaxHealth);
			if (ADuelGameState* GS = GetDuelGS())
			{
				GS->ChampionPawn = Fighters[1];
			}
		}
		else
		{
		FGOTABlueprintCombat::ConfigureHealth(Fighters[1], Profile.ChampionMaxHealth);
		UGOTAChampionDirectorComponent* Director = Fighters[1]->FindComponentByClass<UGOTAChampionDirectorComponent>();
		if (!Director)
		{
			Director = NewObject<UGOTAChampionDirectorComponent>(Fighters[1], TEXT("GOTA_ChampionDirector"));
			Director->RegisterComponent();
		}
		Director->InitDirector(Fighters[0], Difficulty);
		if (!FoeHeroId.IsNone())
		{
			const UGOTAHeroRegistry* Registry = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
			if (const UGOTAHeroDefinition* Foe = Registry ? Registry->FindHero(FoeHeroId) : nullptr)
			{
				UGOTAHeroBodyComponent::ApplyToPawn(Fighters[1], Foe);
				UGOTAHeroAbilityComponent::ApplyToPawn(Fighters[1], FoeHeroId);
			}
		}
		// Hero durability: scale the spawn-time health of fighters that play a hero (before any damage).
		const float BaseHealth[2] = { Profile.ChallengerMaxHealth, Profile.ChampionMaxHealth };
		for (int32 i = 0; i < 2; ++i)
		{
			const UGOTAHeroAbilityComponent* Ab = Fighters[i]->FindComponentByClass<UGOTAHeroAbilityComponent>();
			if (Ab && Ab->GetAbilitySet() && !FMath::IsNearlyEqual(Ab->GetAbilitySet()->MaxHealthScale, 1.f))
			{
				FGOTABlueprintCombat::ConfigureHealth(Fighters[i], BaseHealth[i] * Ab->GetAbilitySet()->MaxHealthScale);
			}
		}
		if (ADuelGameState* GS = GetDuelGS())
		{
			GS->ChampionPawn = Fighters[1];
		}
		} // boss / champion
	}
	if (Comps[0]) { Comps[0]->InitFighter(false, Fighters[1]); }
	if (Comps[1]) { Comps[1]->InitFighter(bVersusAI, Fighters[0]); }
}

UGOTAChampionDirectorComponent* ADuelGameMode::GetChampionDirector() const
{
	const APawn* Champion = AIFighterPawn.Get();
	return Champion ? Champion->FindComponentByClass<UGOTAChampionDirectorComponent>() : nullptr;
}

void ADuelGameMode::ResetFighterForRound(APawn* Pawn)
{
	if (!Pawn || !HasAuthority())
	{
		return;
	}

	if (Pawn->GetClass()->ImplementsInterface(UDuelFighterInterface::StaticClass()))
	{
		IDuelFighterInterface::Execute_ResetForDuelRound(Pawn, NormalizedMaxHealth);
	}

	// Soft heal via observed Stats when BP has not implemented the interface yet.
	if (UStatsComponent* Stats = Pawn->FindComponentByClass<UStatsComponent>())
	{
		Stats->ObserveHealth(NormalizedMaxHealth, NormalizedMaxHealth);
	}
}

void ADuelGameMode::SetCombatEnabledForAll(bool bEnabled)
{
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->bCombatEnabled = bEnabled;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		PC->ResetIgnoreMoveInput();
		if (!bEnabled)
		{
			PC->SetIgnoreMoveInput(true);
		}
		if (APawn* Pawn = PC->GetPawn())
		{
			if (Pawn->GetClass()->ImplementsInterface(UDuelFighterInterface::StaticClass()))
			{
				IDuelFighterInterface::Execute_SetDuelCombatEnabled(Pawn, bEnabled);
			}
		}
	}
}

void ADuelGameMode::BindCombatDeath(APawn* Pawn)
{
	if (!Pawn || !HasAuthority())
	{
		return;
	}
	UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>();
	if (!Combat)
	{
		return;
	}
	if (BoundCombatComponents.Contains(Combat))
	{
		return;
	}
	Combat->OnCharacterDied.AddDynamic(this, &ADuelGameMode::HandleCombatantDied);
	BoundCombatComponents.Add(Combat);
}

void ADuelGameMode::UnbindCombatDeath(APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}
	if (UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>())
	{
		Combat->OnCharacterDied.RemoveDynamic(this, &ADuelGameMode::HandleCombatantDied);
		BoundCombatComponents.Remove(Combat);
	}
}

void ADuelGameMode::HandleCombatantDied(const FCombatActorEvent& Event)
{
	if (!HasAuthority() || bMatchComplete || bRoundResolving)
	{
		return;
	}
	ADuelGameState* GS = GetDuelGS();
	if (!GS || GS->DuelPhase != EDuelPhase::InRound)
	{
		return;
	}

	AActor* DeadActor = Event.Target.Get();
	if (!DeadActor)
	{
		DeadActor = Event.Instigator.Get();
	}
	APawn* DeadPawn = Cast<APawn>(DeadActor);
	if (!DeadPawn)
	{
		return;
	}

	if (ADuelPlayerState* PS = GetDuelPS(DeadPawn->GetController()))
	{
		PS->SetEliminatedThisRound(true);
	}

	// Prefer Destroy so IsPawnAlive / poll also trip; BP may already destroy.
	if (IsValid(DeadPawn) && !DeadPawn->IsActorBeingDestroyed())
	{
		DeadPawn->Destroy();
	}

	EvaluateRound();
}

void ADuelGameMode::PresentHudToPlayer(APlayerController* PC)
{
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	// ADuelHUD BeginPlay owns widget creation when HUDClass is set.
}

void ADuelGameMode::PresentResultsToAll()
{
	// Results shown via ADuelHUD listening to MatchOutcome replication / multicast.
}

bool ADuelGameMode::IsPawnAlive(APawn* Pawn) const
{
	if (!Pawn || !IsValid(Pawn) || Pawn->IsActorBeingDestroyed())
	{
		return false;
	}
	if (ADuelPlayerState* PS = GetDuelPS(Pawn->GetController()))
	{
		if (PS->bIsEliminatedThisRound)
		{
			return false;
		}
	}
	// Observe BPC_DamageSystem (Blueprint health authority) — read-only.
	return !UGOTACombatantObserver::IsCombatantDefeated(Pawn);
}

float ADuelGameMode::GetObservedHealth(APawn* Pawn) const
{
	if (!Pawn)
	{
		return 0.f;
	}
	const FGOTACombatantVitals Vitals = UGOTACombatantObserver::ReadVitals(Pawn);
	if (Vitals.bHasDamageSystem)
	{
		// Normalize so fighters with different max health compare fairly on timeout.
		return Vitals.GetFraction() * NormalizedMaxHealth;
	}
	if (UStatsComponent* Stats = Pawn->FindComponentByClass<UStatsComponent>())
	{
		return Stats->GetHealth();
	}
	return NormalizedMaxHealth;
}

ADuelGameState* ADuelGameMode::GetDuelGS() const
{
	return GetGameState<ADuelGameState>();
}

ADuelPlayerState* ADuelGameMode::GetDuelPS(AController* C) const
{
	return C ? C->GetPlayerState<ADuelPlayerState>() : nullptr;
}

APlayerController* ADuelGameMode::GetControllerForSlot(int32 Slot) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (ADuelPlayerState* PS = GetDuelPS(PC))
			{
				if (PS->DuelSlot == Slot)
				{
					return PC;
				}
			}
		}
	}
	return nullptr;
}

// =====================================================================  Fight AI

APawn* ADuelGameMode::GetFighterPawn(int32 Slot) const
{
	if (bVersusAI && Slot == 1)
	{
		return AIFighterPawn.Get();
	}
	const APlayerController* PC = GetControllerForSlot(Slot);
	return PC ? PC->GetPawn() : nullptr;
}

ADuelPlayerState* ADuelGameMode::GetPSForSlot(int32 Slot) const
{
	if (const AGameStateBase* GS = GetGameState<AGameStateBase>())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			ADuelPlayerState* DuelPS = Cast<ADuelPlayerState>(PS);
			if (DuelPS && DuelPS->DuelSlot == Slot)
			{
				return DuelPS;
			}
		}
	}
	return nullptr;
}

AActor* ADuelGameMode::FindDuelStartForSlot(int32 Slot) const
{
	// Arena graybox exposes 4 starts. PvP uses opposite corners (0 and 2). Fight AI uses 1 and 2:
	// both stand on the host map's baked navmesh, which the champion needs to path (runtime
	// navmesh generation does not yet cover graybox-only ground — see GOTA_UPG_MENU_TEST_RESULTS).
	// Authored arenas: slots 0 and 1 face each other across the fighting floor (baked navmesh).
	const bool bAuthored = IsAuthoredArenaWorld(GetWorld());
	const int32 PreferredStart = bAuthored ? Slot : (bVersusAI ? (Slot == 0 ? 1 : 2) : (Slot == 0 ? 0 : 2));
	if (AArenaPlayerStart* Start = FindStartForSlot(PreferredStart))
	{
		return Start;
	}
	return FindStartForSlot(Slot);
}

void ADuelGameMode::EnsureAIParticipant()
{
	if (!bVersusAI || AIPlayerState)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIPlayerState = GetWorld()->SpawnActor<ADuelPlayerState>(ADuelPlayerState::StaticClass(), Params);
	if (AIPlayerState)
	{
		AIPlayerState->SetIsABot(true);
		AIPlayerState->SetPlayerName(AIChampionName.ToString());
		AIPlayerState->DuelSlot = 1;
	}
}

void ADuelGameMode::SpawnAIFighter()
{
	DespawnAIFighter();

	UClass* ChampionClass = AIChampionClass.LoadSynchronous();
	AActor* Start = FindDuelStartForSlot(1);
	AActor* WardenStart = FindDuelStartForSlot(0);
	if (!ChampionClass || !Start)
	{
		UE_LOG(LogTemp, Error, TEXT("Duel Fight AI: cannot spawn champion (class=%s start=%s)"),
			*AIChampionClass.ToString(), *GetNameSafe(Start));
		CompleteMatch(EDuelMatchOutcome::Draw);
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	// The champion enters the Warden's lane (clear ground on baked navmesh) and faces the challenger.
	FVector SpawnLoc = Start->GetActorLocation();
	FRotator SpawnRot = Start->GetActorRotation();
	if (WardenStart && !IsAuthoredArenaWorld(GetWorld()))
	{
		const FVector W = WardenStart->GetActorLocation();
		SpawnLoc = W + FVector(0.f, W.Y > 0.f ? -AIChampionSpawnDistance : AIChampionSpawnDistance, 0.f);
		SpawnRot = (W - SpawnLoc).Rotation();
		SpawnRot.Pitch = 0.f;
	}
	APawn* Champion = GetWorld()->SpawnActor<APawn>(ChampionClass, SpawnLoc, SpawnRot, Params);
	if (!Champion)
	{
		UE_LOG(LogTemp, Error, TEXT("Duel Fight AI: SpawnActor failed"));
		CompleteMatch(EDuelMatchOutcome::Draw);
		return;
	}
	if (!Champion->GetController())
	{
		Champion->SpawnDefaultController();
	}
	AIFighterPawn = Champion;
	BindCombatDeath(Champion);
	Champion->OnEndPlay.AddUniqueDynamic(this, &ADuelGameMode::HandleFighterEndPlay);
	SetAIBrainActive(false);
	UE_LOG(LogTemp, Log, TEXT("Duel Fight AI: champion %s spawned (controller %s)"),
		*GetNameSafe(Champion), *GetNameSafe(Champion->GetController()));
}

void ADuelGameMode::DespawnAIFighter()
{
	APawn* Champion = AIFighterPawn.Get();
	AIFighterPawn.Reset();
	if (!IsValid(Champion))
	{
		return;
	}
	UnbindCombatDeath(Champion);
	AController* Controller = Champion->GetController();
	DestroyFighter(Champion);
	if (IsValid(Controller) && !Controller->IsA<APlayerController>())
	{
		Controller->Destroy();
	}
}

void ADuelGameMode::SetAIBrainActive(bool bActive)
{
	APawn* Champion = AIFighterPawn.Get();
	AAIController* AIC = Champion ? Cast<AAIController>(Champion->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}
	if (UBrainComponent* Brain = AIC->GetBrainComponent())
	{
		if (bActive)
		{
			Brain->ResumeLogic(TEXT("DuelPhase"));
		}
		else
		{
			Brain->PauseLogic(TEXT("DuelPhase"));
		}
	}
	if (!bActive)
	{
		AIC->StopMovement();
	}
}

void ADuelGameMode::RemovePawnOwnedHud(APlayerController* PC)
{
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), true);
	for (UUserWidget* Widget : Widgets)
	{
		if (Widget && Widget->GetOwningPlayer() == PC && Widget->GetClass()->GetName().StartsWith(TEXT("W_PlayerHUD")))
		{
			Widget->RemoveFromParent();
		}
	}
}

void ADuelGameMode::HandleFighterEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
	const ADuelGameState* GS = GetDuelGS();
	UE_LOG(LogTemp, Log, TEXT("Duel fighter %s left play (reason %d) at %s phase=%d"),
		*GetNameSafe(Actor), static_cast<int32>(Reason),
		Actor ? *Actor->GetActorLocation().ToCompactString() : TEXT("-"),
		GS ? static_cast<int32>(GS->DuelPhase) : -1);
}

void ADuelGameMode::EnsureDuelNavigation()
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys || IsAuthoredArenaWorld(GetWorld()))
	{
		return; // authored arenas ship their own bounds + navmesh
	}

	// Always add arena-wide bounds: the host map's own bounds only cover its original layout.
	static const FName DuelNavTag(TEXT("GOTA_DuelArenaNavBounds"));
	bool bHasBounds = false;
	for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
	{
		bHasBounds |= It->ActorHasTag(DuelNavTag);
	}
	if (!bHasBounds)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ANavMeshBoundsVolume* Bounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(
			ANavMeshBoundsVolume::StaticClass(), FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, Params))
		{
			Bounds->Tags.Add(DuelNavTag);
			Bounds->SetActorScale3D(FVector(50.f, 50.f, 20.f));
			NavSys->OnNavigationBoundsUpdated(Bounds);
		}
	}

	// ThirdPersonMap's RecastNavMesh uses Dynamic runtime generation (Tools/GOTA_ConfigureArenaNav.py),
	// so the runtime-built arena gets navmesh once the arena-wide bounds above are registered.
	NavSys->Build();
	UE_LOG(LogTemp, Log, TEXT("Duel Fight AI: arena nav bounds registered, navmesh build requested (runtime generation %s)"),
		NavSys->SupportsNavigationGeneration() ? TEXT("supported") : TEXT("NOT supported — run Tools/GOTA_ConfigureArenaNav.py"));
}

void ADuelGameMode::AlertChampionToChallenger()
{
	APawn* Champion = AIFighterPawn.Get();
	APawn* Challenger = GetFighterPawn(0);
	if (!bVersusAI || !Champion || !Challenger)
	{
		return;
	}
	const FVector ChallengerLoc = Challenger->GetActorLocation();
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), ChallengerLoc, 1.f, Challenger, 0.f, NAME_None);
	UAISense_Damage::ReportDamageEvent(GetWorld(), Champion, Challenger, 0.f, ChallengerLoc, Champion->GetActorLocation());
	// Commit the champion to its challenger through its own controller API (AIC_Enemy_Base).
	if (AController* AIC = Champion->GetController())
	{
		if (UFunction* Engage = AIC->FindFunction(ChampionEngageFunction))
		{
			uint8* Params = static_cast<uint8*>(FMemory_Alloca(Engage->ParmsSize));
			FMemory::Memzero(Params, Engage->ParmsSize);
			Engage->InitializeStruct(Params);
			for (TFieldIterator<FProperty> P(Engage); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
			{
				if (FObjectProperty* Obj = CastField<FObjectProperty>(*P))
				{
					Obj->SetObjectPropertyValue_InContainer(Params, Challenger);
				}
			}
			AIC->ProcessEvent(Engage, Params);
			Engage->DestroyStruct(Params);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Duel Fight AI: champion alerted to %s"), *GetNameSafe(Challenger));
}

// =====================================================================  Hero selection (server)

FString ADuelGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Error = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	ADuelPlayerState* PS = NewPlayerController ? NewPlayerController->GetPlayerState<ADuelPlayerState>() : nullptr;
	const UGOTAHeroRegistry* Registry = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	if (!PS || !Registry)
	{
		return Error;
	}
	const FName Requested(*UGameplayStatics::ParseOption(Options, TEXT("Hero")));
	FText Reason;
	if (!Requested.IsNone() && Registry->ValidateSelection(Requested, EArenaMode::PvPDuel, Reason))
	{
		PS->HeroId = Requested;
	}
	else
	{
		PS->HeroId = Registry->GetDefaultHero(EArenaMode::PvPDuel);
		if (!Requested.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("Duel: hero '%s' rejected (%s) — using %s"), *Requested.ToString(), *Reason.ToString(), *PS->HeroId.ToString());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Duel: %s accepted hero %s"), *GetNameSafe(NewPlayerController), *PS->HeroId.ToString());
	return Error;
}

UClass* ADuelGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const ADuelPlayerState* PS = InController ? InController->GetPlayerState<ADuelPlayerState>() : nullptr;
	const UGOTAHeroRegistry* Registry = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	if (PS && Registry && !PS->HeroId.IsNone())
	{
		if (const UGOTAHeroDefinition* Hero = Registry->FindHero(PS->HeroId); Hero && UGOTAHeroRegistry::IsHeroSpawnable(Hero))
		{
			if (UClass* PawnClass = Hero->GameplayPawnClass.LoadSynchronous())
			{
				return PawnClass;
			}
		}
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ADuelGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);
	const ADuelPlayerState* PS = PlayerPawn ? PlayerPawn->GetPlayerState<ADuelPlayerState>() : nullptr;
	if (!PS && PlayerPawn && PlayerPawn->GetController())
	{
		PS = PlayerPawn->GetController()->GetPlayerState<ADuelPlayerState>();
	}
	const UGOTAHeroRegistry* Registry = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	if (PS && Registry && !PS->HeroId.IsNone())
	{
		UGOTAHeroBodyComponent::ApplyToPawn(PlayerPawn, Registry->FindHero(PS->HeroId));
		// Divine abilities resolved from the accepted (server-validated) HeroId — never from the client.
		UGOTAHeroAbilityComponent::ApplyToPawn(PlayerPawn, PS->HeroId);
	}
}

void ADuelGameMode::EquipFighterLoadout(APawn* Pawn)
{
	if (!Pawn || FighterLoadoutFunction.IsNone())
	{
		return;
	}
	UFunction* Fn = Pawn->FindFunction(FighterLoadoutFunction);
	if (Fn && Fn->ParmsSize == 0)
	{
		Pawn->ProcessEvent(Fn, nullptr);
		UE_LOG(LogTemp, Log, TEXT("Duel: %s equipped via %s"), *GetNameSafe(Pawn), *FighterLoadoutFunction.ToString());
	}
}

void ADuelGameMode::RemoveDeveloperPawns()
{
	TArray<AActor*> ToRemove;
	for (TActorIterator<AEQSTestingPawn> It(GetWorld()); It; ++It)
	{
		ToRemove.Add(*It);
	}
	for (AActor* Actor : ToRemove)
	{
		UE_LOG(LogTemp, Log, TEXT("Duel Fight AI: removed developer actor %s (was a perception target)"), *GetNameSafe(Actor));
		Actor->Destroy();
	}
}

void ADuelGameMode::DestroyFighter(APawn* Pawn)
{
	if (!IsValid(Pawn))
	{
		return;
	}
	TArray<AActor*> Attached;
	Pawn->GetAttachedActors(Attached, true, true);
	for (AActor* Actor : Attached)
	{
		if (IsValid(Actor) && !Actor->IsA<APawn>())
		{
			Actor->Destroy();
		}
	}
	Pawn->Destroy();
}
