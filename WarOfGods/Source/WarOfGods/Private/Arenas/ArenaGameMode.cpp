// White Tiger Studios 2024

#include "Arenas/ArenaGameMode.h"

#include "Arenas/ArenaEncounterDirector.h"
#include "Arenas/ArenaGameState.h"
#include "Arenas/ArenaGrayboxBuilder.h"
#include "Misc/PackageName.h"
#include "Arenas/ArenaVisualDressDefaults.h"
#include "Arenas/ArenaVisualDressDefinition.h"
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

	// URL: ?Arena=Story|Fallen|Quarry|Ashfall|...  and optional ?Mode=Story
	if (UGameplayStatics::HasOption(Options, TEXT("Arena")))
	{
		const FString A = UGameplayStatics::ParseOption(Options, TEXT("Arena"));
		auto Match = [&A](const TCHAR* Name) { return A.Equals(Name, ESearchCase::IgnoreCase); };

		if (Match(TEXT("Story")) || Match(TEXT("Fallen")) || Match(TEXT("FallenSanctuary"))
			|| Match(TEXT("Prologue")) || Match(TEXT("Sanctuary")))
		{
			GrayboxLayout = EArenaGrayboxLayout::FallenSanctuary;
			ArenaId = EArenaId::FallenSanctuary;
			ArenaMode = EArenaMode::Story;
		}
		else if (Match(TEXT("Dominion")) || Match(TEXT("Shattered")))
		{
			GrayboxLayout = EArenaGrayboxLayout::ShatteredDominion;
			ArenaId = EArenaId::ShatteredDominion;
		}
		else if (Match(TEXT("Ashfall")) || Match(TEXT("Crucible")))
		{
			GrayboxLayout = EArenaGrayboxLayout::AshfallCrucible;
			ArenaId = EArenaId::AshfallCrucible;
		}
		else if (Match(TEXT("Frostbound")) || Match(TEXT("Frost")) || Match(TEXT("Citadel")))
		{
			GrayboxLayout = EArenaGrayboxLayout::FrostboundCitadel;
			ArenaId = EArenaId::FrostboundCitadel;
		}
		else if (Match(TEXT("Verdant")) || Match(TEXT("Forest")))
		{
			GrayboxLayout = EArenaGrayboxLayout::VerdantSanctuary;
			ArenaId = EArenaId::VerdantSanctuary;
		}
		else if (Match(TEXT("Sunscar")) || Match(TEXT("Desert")) || Match(TEXT("Expanse")))
		{
			GrayboxLayout = EArenaGrayboxLayout::SunscarExpanse;
			ArenaId = EArenaId::SunscarExpanse;
		}
		else if (Match(TEXT("Stormbreak")) || Match(TEXT("Coast")) || Match(TEXT("Storm")))
		{
			GrayboxLayout = EArenaGrayboxLayout::StormbreakCoast;
			ArenaId = EArenaId::StormbreakCoast;
		}
		else
		{
			GrayboxLayout = EArenaGrayboxLayout::TitansQuarry;
			ArenaId = EArenaId::TitansQuarry;
		}
	}

	if (UGameplayStatics::HasOption(Options, TEXT("Mode")))
	{
		const FString ModeOpt = UGameplayStatics::ParseOption(Options, TEXT("Mode"));
		if (ModeOpt.Equals(TEXT("Story"), ESearchCase::IgnoreCase))
		{
			ArenaMode = EArenaMode::Story;
			if (ArenaId == EArenaId::TitansQuarry && GrayboxLayout == EArenaGrayboxLayout::TitansQuarry)
			{
				GrayboxLayout = EArenaGrayboxLayout::FallenSanctuary;
				ArenaId = EArenaId::FallenSanctuary;
			}
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
		const FSoftObjectPath DressPath = ArenaVisualDressDefaults::GetCanonicalDressPath(NewLayout);
		if (DressPath.IsValid())
		{
			SpawnedGraybox->VisualDress = TSoftObjectPtr<UArenaVisualDressDefinition>(DressPath);
		}
		SpawnedGraybox->BuildLayout();
	}
	else if (HasAuthority())
	{
		EnsureGraybox();
	}
}

const FName AArenaGameMode::AuthoredArenaTag(TEXT("GOTA_AuthoredArena"));

bool AArenaGameMode::IsAuthoredArenaWorld(const UWorld* World)
{
	if (!World)
	{
		return false;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(AuthoredArenaTag))
		{
			return true;
		}
	}
	return false;
}

FString AArenaGameMode::GetAuthoredArenaMap(EArenaId Arena)
{
	const TCHAR* Name = nullptr;
	switch (Arena)
	{
	case EArenaId::TitansQuarry: Name = TEXT("TitansQuarry"); break;
	case EArenaId::AshfallCrucible: Name = TEXT("AshfallCrucible"); break;
	case EArenaId::FrostboundCitadel: Name = TEXT("FrostboundCitadel"); break;
	case EArenaId::VerdantSanctuary: Name = TEXT("VerdantSanctuary"); break;
	case EArenaId::SunscarExpanse: Name = TEXT("SunscarExpanse"); break;
	case EArenaId::StormbreakCoast: Name = TEXT("StormbreakCoast"); break;
	case EArenaId::ShatteredDominion: Name = TEXT("ShatteredDominion"); break;
	default: return FString();
	}
	const FString Package = FString::Printf(TEXT("/Game/Arenas/Maps/L_Arena_%s"), Name);
	return FPackageName::DoesPackageExist(Package) ? Package : FString();
}

void AArenaGameMode::EnsureGraybox()
{
	if (!bAutoBuildGraybox)
	{
		return;
	}
	if (IsAuthoredArenaWorld(GetWorld()))
	{
		UE_LOG(LogTemp, Log, TEXT("Arena: authored level detected — runtime graybox skipped"));
		return;
	}

	auto BindCanonicalDress = [](AArenaGrayboxBuilder* Builder, EArenaGrayboxLayout Layout)
	{
		if (!Builder || !Builder->VisualDress.IsNull())
		{
			return;
		}
		const FSoftObjectPath DressPath = ArenaVisualDressDefaults::GetCanonicalDressPath(Layout);
		if (DressPath.IsValid())
		{
			Builder->VisualDress = TSoftObjectPtr<UArenaVisualDressDefinition>(DressPath);
		}
	};

	for (TActorIterator<AArenaGrayboxBuilder> It(GetWorld()); It; ++It)
	{
		SpawnedGraybox = *It;
		SpawnedGraybox->Layout = GrayboxLayout;
		BindCanonicalDress(SpawnedGraybox, GrayboxLayout);
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
		BindCanonicalDress(SpawnedGraybox, GrayboxLayout);
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
