// White Tiger Studios 2024

#include "Arenas/ArenaConsoleCommands.h"

#include "Arenas/ArenaGameMode.h"
#include "Arenas/ArenaGrayboxBuilder.h"
#include "Arenas/ArenaTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

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
			if (Arg.Equals(TEXT("Dominion"), ESearchCase::IgnoreCase) || Arg.Equals(TEXT("Shattered"), ESearchCase::IgnoreCase))
			{
				Layout = EArenaGrayboxLayout::ShatteredDominionThreeBiome;
			}
			else if (Arg.Equals(TEXT("Quarry"), ESearchCase::IgnoreCase) || Arg.Equals(TEXT("Titans"), ESearchCase::IgnoreCase))
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
		TEXT("Rebuild arena graybox. Usage: gota.BuildArena Quarry|Dominion"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&BuildLayoutCommand));
}

void FArenaConsoleCommands::Register()
{
	// FAutoConsoleCommand registers on construction.
}

void FArenaConsoleCommands::Unregister()
{
}
