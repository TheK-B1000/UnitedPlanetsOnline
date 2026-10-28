// White Tiger Studios 2024

#include "Arenas/ArenaConsoleCommands.h"

#include "Arenas/ArenaGameMode.h"
#include "Arenas/ArenaGrayboxBuilder.h"
#include "Arenas/ArenaTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	AArenaGameMode* GetArenaGameMode()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* World = Context.World())
			{
				if (AArenaGameMode* GM = World->GetAuthGameMode<AArenaGameMode>())
				{
					return GM;
				}
			}
		}
		return nullptr;
	}

	AArenaGrayboxBuilder* GetOrSpawnGraybox(UWorld* World)
	{
		for (TActorIterator<AArenaGrayboxBuilder> It(World); It; ++It)
		{
			return *It;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<AArenaGrayboxBuilder>(AArenaGrayboxBuilder::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}

	void BuildLayoutCommand(const TArray<FString>& Args)
	{
		UWorld* World = nullptr;
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld())
				{
					World = Context.World();
					break;
				}
			}
		}
		if (!World || !World->GetAuthGameMode())
		{
			UE_LOG(LogTemp, Warning, TEXT("gota.BuildArena: no authority game world"));
			return;
		}

		EArenaGrayboxLayout Layout = EArenaGrayboxLayout::TitansQuarry;
		if (Args.Num() > 0)
		{
			const FString& Arg = Args[0];
			auto Match = [&Arg](const TCHAR* Name) { return Arg.Equals(Name, ESearchCase::IgnoreCase); };

			if (Match(TEXT("Story")) || Match(TEXT("Fallen")) || Match(TEXT("FallenSanctuary"))
				|| Match(TEXT("Prologue")) || Match(TEXT("Sanctuary")))
			{
				Layout = EArenaGrayboxLayout::FallenSanctuary;
			}
			else if (Match(TEXT("Dominion")) || Match(TEXT("Shattered")))
			{
				Layout = EArenaGrayboxLayout::ShatteredDominion;
			}
			else if (Match(TEXT("Ashfall")) || Match(TEXT("Crucible")))
			{
				Layout = EArenaGrayboxLayout::AshfallCrucible;
			}
			else if (Match(TEXT("Frostbound")) || Match(TEXT("Frost")) || Match(TEXT("Citadel")))
			{
				Layout = EArenaGrayboxLayout::FrostboundCitadel;
			}
			else if (Match(TEXT("Verdant")) || Match(TEXT("Forest")))
			{
				Layout = EArenaGrayboxLayout::VerdantSanctuary;
			}
			else if (Match(TEXT("Sunscar")) || Match(TEXT("Desert")) || Match(TEXT("Expanse")))
			{
				Layout = EArenaGrayboxLayout::SunscarExpanse;
			}
			else if (Match(TEXT("Stormbreak")) || Match(TEXT("Coast")) || Match(TEXT("Storm")))
			{
				Layout = EArenaGrayboxLayout::StormbreakCoast;
			}
			else
			{
				Layout = EArenaGrayboxLayout::TitansQuarry;
			}
		}

		if (AArenaGrayboxBuilder* Builder = GetOrSpawnGraybox(World))
		{
			Builder->Layout = Layout;
			Builder->BuildLayout();
			UE_LOG(LogTemp, Log, TEXT("gota.BuildArena -> layout %d"), static_cast<int32>(Layout));
		}

		if (AArenaGameMode* GM = GetArenaGameMode())
		{
			GM->GrayboxLayout = Layout;
		}
	}

	FAutoConsoleCommand BuildArenaCmd(
		TEXT("gota.BuildArena"),
		TEXT("Rebuild arena graybox. Usage: gota.BuildArena Story|Quarry|Ashfall|Frostbound|Verdant|Sunscar|Stormbreak|Dominion"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&BuildLayoutCommand));

	void LaunchHordeCommand()
	{
		UWorld* World = nullptr;
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld())
				{
					World = Context.World();
					break;
				}
			}
		}
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("gota.Horde: no game world"));
			return;
		}

		UGameplayStatics::OpenLevel(
			World,
			FName(TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap")),
			true,
			TEXT("game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry?listen"));
		UE_LOG(LogTemp, Log, TEXT("gota.Horde -> Titan's Quarry Wrath of the Gods"));
	}

	FAutoConsoleCommand LaunchHordeCmd(
		TEXT("gota.Horde"),
		TEXT("Travel to Titan's Quarry Wrath of the Gods (PvE Horde, 5 waves + boss)"),
		FConsoleCommandDelegate::CreateStatic(&LaunchHordeCommand));
}

void FArenaConsoleCommands::Register()
{
	// FAutoConsoleCommand registers on construction.
}

void FArenaConsoleCommands::Unregister()
{
}
