// White Tiger Studios 2024

#include "Arenas/ArenaGameMode.h"

#include "Arenas/ArenaEncounterDirector.h"
#include "Arenas/ArenaGameState.h"
#include "Arenas/ArenaGrayboxBuilder.h"
#include "Arenas/ArenaPlayerStart.h"
#include "EngineUtils.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"

AArenaGameMode::AArenaGameMode()
{
	GameStateClass = AArenaGameState::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	// DefaultPawnClass left unset so project/BP GameMode overrides or map defaults can supply BP_Player.
}

void AArenaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (UWorld* World = GetWorld())
	{
		// Soft cap for 4-player co-op / skirmish.
		if (AGameSession* Session = GameSession)
		{
			Session->MaxPlayers = SoftMaxPlayers;
		}
	}
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		EnsureGraybox();
		if (bAutoStartEncounterDirector)
		{
			EnsureEncounterDirector();
		}

		if (AArenaGameState* GS = GetGameState<AArenaGameState>())
		{
			GS->ActiveArenaId = ArenaId;
			GS->ActiveMode = ArenaMode;
			GS->SetArenaPhase(EArenaPhase::WaitingForPlayers);
		}
	}
}

void AArenaGameMode::EnsureGraybox()
{
	if (!bAutoBuildGraybox || SpawnedGraybox)
	{
		return;
	}

	for (TActorIterator<AArenaGrayboxBuilder> It(GetWorld()); It; ++It)
	{
		SpawnedGraybox = *It;
		SpawnedGraybox->Layout = GrayboxLayout;
		SpawnedGraybox->BuildLayout();
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedGraybox = GetWorld()->SpawnActor<AArenaGrayboxBuilder>(
		AArenaGrayboxBuilder::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (SpawnedGraybox)
	{
		SpawnedGraybox->Layout = GrayboxLayout;
		SpawnedGraybox->bBuildOnBeginPlay = true;
		SpawnedGraybox->BuildLayout();
	}
}

void AArenaGameMode::EnsureEncounterDirector()
{
	if (EncounterDirector)
	{
		return;
	}

	for (TActorIterator<AArenaEncounterDirector> It(GetWorld()); It; ++It)
	{
		EncounterDirector = *It;
		return;
	}

	FActorSpawnParameters Params;
	EncounterDirector = GetWorld()->SpawnActor<AArenaEncounterDirector>(
		AArenaEncounterDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (EncounterDirector && SpawnedGraybox)
	{
		EncounterDirector->SetActorLocation(SpawnedGraybox->GetActorLocation());
	}
}

AActor* AArenaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const int32 Slot = NextPlayerSlot % SoftMaxPlayers;
	if (AArenaPlayerStart* Start = FindStartForSlot(Slot))
	{
		++NextPlayerSlot;
		return Start;
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

AArenaPlayerStart* AArenaGameMode::FindStartForSlot(int32 Slot) const
{
	for (TActorIterator<AArenaPlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerSlot == Slot)
		{
			return *It;
		}
	}
	return nullptr;
}

void AArenaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AArenaGameState* GS = GetGameState<AArenaGameState>())
	{
		const int32 NumPlayers = GetNumPlayers();
		if (NumPlayers >= 1 && GS->ArenaPhase == EArenaPhase::WaitingForPlayers)
		{
			GS->SetArenaPhase(EArenaPhase::Active);
		}
		UE_LOG(LogTemp, Log, TEXT("Arena PostLogin players=%d/%d"), NumPlayers, SoftMaxPlayers);
	}
}
