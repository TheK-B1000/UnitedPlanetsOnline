// White Tiger Studios 2024

#include "Arenas/Duel/DuelGameMode.h"

#include "Arenas/ArenaPlayerStart.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADuelGameMode::ADuelGameMode()
{
	GameStateClass = ADuelGameState::StaticClass();
	PlayerStateClass = ADuelPlayerState::StaticClass();
	ArenaMode = EArenaMode::PvPDuel;
	ArenaId = EArenaId::TitansQuarry;
	GrayboxLayout = EArenaGrayboxLayout::TitansQuarry;
	SoftMaxPlayers = 2;
	bAutoStartEncounterDirector = false;
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

	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->ActiveMode = EArenaMode::PvPDuel;
		GS->ActiveArenaId = ArenaId;
		GS->RoundsToWin = RoundsToWin;
		GS->CurrentRound = 0;
		GS->ScorePlayer0 = 0;
		GS->ScorePlayer1 = 0;
		GS->MatchOutcome = EDuelMatchOutcome::None;
		GS->SetDuelPhase(EDuelPhase::WaitingForPlayers);
	}

	GetWorldTimerManager().SetTimer(
		RoundWatchTimerHandle,
		this,
		&ADuelGameMode::EvaluateRound,
		0.25f,
		true);
}

void ADuelGameMode::PostLogin(APlayerController* NewPlayer)
{
	AGameModeBase::PostLogin(NewPlayer);
	AssignDuelSlots();

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
	Super::Logout(Exiting);
	if (!HasAuthority() || bMatchComplete)
	{
		return;
	}
	if (ADuelGameState* GS = GetDuelGS())
	{
		if (GS->DuelPhase == EDuelPhase::InRound || GS->DuelPhase == EDuelPhase::Countdown)
		{
			// Opponent disconnect → remaining fighter wins the match.
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

	// Prefer opposite corners: slot 0 → PlayerStart 0, slot 1 → PlayerStart 2
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
	if (GS->DuelPhase != EDuelPhase::WaitingForPlayers && GS->DuelPhase != EDuelPhase::MatchEnd)
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
	GS->BroadcastScore();
	bMatchComplete = false;
	BeginCountdown();
}

void ADuelGameMode::BeginCountdown()
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS)
	{
		return;
	}

	++GS->CurrentRound;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ADuelPlayerState* PS = GetDuelPS(It->Get()))
		{
			PS->SetEliminatedThisRound(false);
		}
	}

	RespawnFighters();
	GS->PhaseDeadlineServerTime = GS->GetServerWorldTimeSeconds() + CountdownSeconds;
	GS->SetDuelPhase(EDuelPhase::Countdown);

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::BeginRound, CountdownSeconds, false);
}

void ADuelGameMode::BeginRound()
{
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->SetDuelPhase(EDuelPhase::InRound);
	}
}

void ADuelGameMode::EvaluateRound()
{
	if (!HasAuthority() || bMatchComplete)
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
		EndRound(0);
	}
	else if (!bAlive0 && bAlive1)
	{
		EndRound(1);
	}
	else if (!bAlive0 && !bAlive1)
	{
		// Mutual KO — no score, restart countdown.
		GS->SetDuelPhase(EDuelPhase::RoundEnd);
		GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::BeginCountdown, RoundEndDelay, false);
	}
}

void ADuelGameMode::EndRound(int32 WinningSlot)
{
	ADuelGameState* GS = GetDuelGS();
	if (!GS || GS->DuelPhase != EDuelPhase::InRound)
	{
		return;
	}

	GS->SetDuelPhase(EDuelPhase::RoundEnd);
	if (WinningSlot == 0)
	{
		++GS->ScorePlayer0;
		if (ADuelPlayerState* PS = GetDuelPS(GetControllerForSlot(0)))
		{
			PS->AddRoundWin();
		}
	}
	else
	{
		++GS->ScorePlayer1;
		if (ADuelPlayerState* PS = GetDuelPS(GetControllerForSlot(1)))
		{
			PS->AddRoundWin();
		}
	}
	GS->BroadcastScore();

	UE_LOG(LogTemp, Log, TEXT("Duel round %d end — score %d:%d (to %d)"),
		GS->CurrentRound, GS->ScorePlayer0, GS->ScorePlayer1, GS->RoundsToWin);

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
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ADuelGameMode::BeginCountdown, RoundEndDelay, false);
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
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->MatchOutcome = Outcome;
		GS->OnDuelMatchOutcomeChanged.Broadcast(Outcome);
		GS->SetDuelPhase(EDuelPhase::MatchEnd);
	}
	UE_LOG(LogTemp, Log, TEXT("Duel match complete outcome=%d"), static_cast<int32>(Outcome));
}

void ADuelGameMode::RequestRematch()
{
	if (!HasAuthority())
	{
		return;
	}
	bMatchComplete = false;
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->SetDuelPhase(EDuelPhase::WaitingForPlayers);
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
		}
		else if (AActor* Start = ChoosePlayerStart_Implementation(PC))
		{
			Pawn->SetActorLocationAndRotation(Start->GetActorLocation(), Start->GetActorRotation());
		}
	}
}

bool ADuelGameMode::IsPawnAlive(APawn* Pawn) const
{
	return Pawn && IsValid(Pawn) && !Pawn->IsActorBeingDestroyed();
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
