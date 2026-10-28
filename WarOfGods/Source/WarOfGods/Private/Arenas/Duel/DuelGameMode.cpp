// White Tiger Studios 2024

#include "Arenas/Duel/DuelGameMode.h"

#include "Arenas/ArenaPlayerStart.h"
#include "Arenas/Duel/DuelFighterInterface.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/DuelHUD.h"
#include "Arenas/Duel/DuelMatchDefinition.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "Characters/StatsComponent.h"
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
}

void ADuelGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	ArenaMode = EArenaMode::PvPDuel;
	SoftMaxPlayers = 2;
	Super::InitGame(MapName, Options, ErrorMessage);
	ArenaMode = EArenaMode::PvPDuel;
	SoftMaxPlayers = 2;
	bAutoStartEncounterDirector = false;

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
		GS->SetDuelPhase(EDuelPhase::WaitingForPlayers);
	}

	GetWorldTimerManager().SetTimer(
		RoundWatchTimerHandle,
		this,
		&ADuelGameMode::EvaluateRound,
		0.25f,
		true);
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

	if (bAutoStartWhenFull && GetNumPlayers() >= SoftMaxPlayers)
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
	if (bRequireBothPlayers && GetNumPlayers() < SoftMaxPlayers)
	{
		UE_LOG(LogTemp, Log, TEXT("Duel waiting for %d players (have %d)"), SoftMaxPlayers, GetNumPlayers());
		return;
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

	APlayerController* PC0 = GetControllerForSlot(0);
	APlayerController* PC1 = GetControllerForSlot(1);
	APawn* Pawn0 = PC0 ? PC0->GetPawn() : nullptr;
	APawn* Pawn1 = PC1 ? PC1->GetPawn() : nullptr;
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

	APlayerController* PC0 = GetControllerForSlot(0);
	APlayerController* PC1 = GetControllerForSlot(1);
	const bool bAlive0 = PC0 && IsPawnAlive(PC0->GetPawn());
	const bool bAlive1 = PC1 && IsPawnAlive(PC1->GetPawn());

	if (ADuelPlayerState* PS0 = GetDuelPS(PC0)) { PS0->SetEliminatedThisRound(!bAlive0); }
	if (ADuelPlayerState* PS1 = GetDuelPS(PC1)) { PS1->SetEliminatedThisRound(!bAlive1); }

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
	GS->SetDuelPhase(EDuelPhase::RoundEnd);
	GS->LastRoundWinnerSlot = WinningSlot;

	if (bScored && WinningSlot == 0)
	{
		++GS->ScorePlayer0;
		if (ADuelPlayerState* PS = GetDuelPS(GetControllerForSlot(0)))
		{
			PS->AddRoundWin();
			PS->AddElimination();
		}
		if (ADuelPlayerState* PS = GetDuelPS(GetControllerForSlot(1)))
		{
			PS->AddDeath();
		}
	}
	else if (bScored && WinningSlot == 1)
	{
		++GS->ScorePlayer1;
		if (ADuelPlayerState* PS = GetDuelPS(GetControllerForSlot(1)))
		{
			PS->AddRoundWin();
			PS->AddElimination();
		}
		if (ADuelPlayerState* PS = GetDuelPS(GetControllerForSlot(0)))
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
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->SetDuelPhase(EDuelPhase::ReturningToLobby);
	}
	const FString MapPackage = LobbyMap.GetLongPackageName();
	UGameplayStatics::OpenLevel(this, FName(*MapPackage), true);
}

void ADuelGameMode::RespawnFighters()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		APawn* Pawn = PC->GetPawn();
		if (!IsPawnAlive(Pawn))
		{
			RestartPlayer(PC);
			Pawn = PC->GetPawn();
		}
		else if (AActor* Start = ChoosePlayerStart_Implementation(PC))
		{
			Pawn->SetActorLocationAndRotation(Start->GetActorLocation(), Start->GetActorRotation());
			if (ACharacter* Char = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
				{
					Move->StopMovementImmediately();
				}
			}
		}

		if (Pawn)
		{
			BindCombatDeath(Pawn);
			ResetFighterForRound(Pawn);
			if (APlayerController* Owning = Cast<APlayerController>(Pawn->GetController()))
			{
				Owning->SetViewTarget(Pawn);
			}
		}
	}
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
	return true;
}

float ADuelGameMode::GetObservedHealth(APawn* Pawn) const
{
	if (!Pawn)
	{
		return 0.f;
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
