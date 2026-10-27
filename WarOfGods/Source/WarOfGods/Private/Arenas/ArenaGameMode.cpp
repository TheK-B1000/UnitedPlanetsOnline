// White Tiger Studios 2024

#include "Arenas/ArenaGameMode.h"

#include "Arenas/ArenaEncounterDirector.h"
#include "Arenas/ArenaGameState.h"
#include "Arenas/ArenaGrayboxBuilder.h"
#include "Arenas/ArenaPlayerStart.h"
#include "EngineUtils.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AArenaGameMode::AArenaGameMode()
{
	GameStateClass = AArenaGameState::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();

	SoftPlayerClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/Player/BP_Player.BP_Player_C")));
	SoftPlayerControllerClass = TSoftClassPtr<APlayerController>(FSoftObjectPath(TEXT("/Game/Framework/BP_WarOfGods_PlayerController.BP_WarOfGods_PlayerController_C")));
	SoftEnemyClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/Enemies/MeleeEnemy/BP_Enemy_Melee.BP_Enemy_Melee_C")));

	// Fallback constructor helpers (editor / cooked soft paths).
	static ConstructorHelpers::FClassFinder<APawn> PlayerFinder(TEXT("/Game/Player/BP_Player"));
	if (PlayerFinder.Succeeded())
	{
		DefaultPawnClass = PlayerFinder.Class;
	}
	else
	{
		static ConstructorHelpers::FClassFinder<APawn> TPFinder(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
		if (TPFinder.Succeeded())
		{
			DefaultPawnClass = TPFinder.Class;
		}
	}

	static ConstructorHelpers::FClassFinder<APlayerController> PCFinder(TEXT("/Game/Framework/BP_WarOfGods_PlayerController"));
	if (PCFinder.Succeeded())
	{
		PlayerControllerClass = PCFinder.Class;
	}
}

void AArenaGameMode::ResolveSoftClasses()
{
	if (!DefaultPawnClass && SoftPlayerClass.IsValid())
	{
		DefaultPawnClass = SoftPlayerClass.LoadSynchronous();
	}
	else if (!DefaultPawnClass && !SoftPlayerClass.IsNull())
	{
		DefaultPawnClass = SoftPlayerClass.LoadSynchronous();
	}

	if (SoftPlayerControllerClass.IsValid() || !SoftPlayerControllerClass.IsNull())
	{
		if (UClass* PCClass = SoftPlayerControllerClass.LoadSynchronous())
		{
			PlayerControllerClass = PCClass;
		}
	}
}

void AArenaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	ResolveSoftClasses();
	Super::InitGame(MapName, Options, ErrorMessage);

	if (AGameSession* Session = GameSession)
	{
		Session->MaxPlayers = SoftMaxPlayers;
	}

	// URL option: ?Arena=Dominion or ?Arena=Quarry
	if (UGameplayStatics::HasOption(Options, TEXT("Arena")))
	{
		const FString ArenaOpt = UGameplayStatics::ParseOption(Options, TEXT("Arena"));
		if (ArenaOpt.Equals(TEXT("Dominion"), ESearchCase::IgnoreCase) || ArenaOpt.Equals(TEXT("Shattered"), ESearchCase::IgnoreCase))
		{
			GrayboxLayout = EArenaGrayboxLayout::ShatteredDominionThreeBiome;
			ArenaId = EArenaId::ShatteredDominion;
		}
		else
		{
			GrayboxLayout = EArenaGrayboxLayout::TitansQuarry;
			ArenaId = EArenaId::TitansQuarry;
		}
	}
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();
	ResolveSoftClasses();

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

void AArenaGameMode::SetGrayboxLayout(EArenaGrayboxLayout NewLayout)
{
	GrayboxLayout = NewLayout;
	if (SpawnedGraybox)
	{
		SpawnedGraybox->Layout = NewLayout;
		SpawnedGraybox->BuildLayout();
	}
	else if (HasAuthority())
	{
		EnsureGraybox();
	}
}

void AArenaGameMode::EnsureGraybox()
{
	if (!bAutoBuildGraybox)
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
	for (TActorIterator<AArenaEncounterDirector> It(GetWorld()); It; ++It)
	{
		EncounterDirector = *It;
		break;
	}

	if (!EncounterDirector)
	{
		FActorSpawnParameters Params;
		EncounterDirector = GetWorld()->SpawnActor<AArenaEncounterDirector>(
			AArenaEncounterDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}

	if (EncounterDirector)
	{
		if (SpawnedGraybox)
		{
			EncounterDirector->SetActorLocation(SpawnedGraybox->GetActorLocation());
		}

		if (!EncounterDirector->EnemyClass)
		{
			UClass* Enemy = SoftEnemyClass.LoadSynchronous();
			if (!Enemy)
			{
				Enemy = StaticLoadClass(APawn::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_Enemy_Melee.BP_Enemy_Melee_C"));
			}
			if (Enemy)
			{
				EncounterDirector->EnemyClass = Enemy;
				UE_LOG(LogTemp, Log, TEXT("ArenaEncounterDirector EnemyClass set to %s"), *GetNameSafe(Enemy));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Could not load BP_Enemy_Melee — waves will log only."));
			}
		}
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
