// White Tiger Studios 2024

#include "Arenas/ArenaConsoleCommands.h"

#include "Arenas/ArenaGameMode.h"
#include "Arenas/ArenaGrayboxBuilder.h"
#include "Arenas/ArenaTypes.h"
#include "Arenas/ArenaVisualDressDefaults.h"
#include "Arenas/ArenaVisualDressDefinition.h"
#include "Arenas/Horde/HordeGameMode.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
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
			// Always rebind so layout switches do not keep a stale biome DA on VisualDress.
			const FSoftObjectPath DressPath = ArenaVisualDressDefaults::GetCanonicalDressPath(Layout);
			if (DressPath.IsValid())
			{
				Builder->VisualDress = TSoftObjectPtr<UArenaVisualDressDefinition>(DressPath);
			}
			Builder->BuildLayout();
			UE_LOG(LogTemp, Log, TEXT("gota.BuildArena -> layout %d dress=%s"),
				static_cast<int32>(Layout),
				Builder->VisualDress.IsNull() ? TEXT("(transient/none)") : *Builder->VisualDress.ToSoftObjectPath().ToString());
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

	AHordeGameMode* GetHordeGameMode()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* World = Context.World())
			{
				if (AHordeGameMode* GM = World->GetAuthGameMode<AHordeGameMode>())
				{
					return GM;
				}
			}
		}
		return nullptr;
	}

	void HordeDumpCommand()
	{
		if (AHordeGameMode* GM = GetHordeGameMode())
		{
			GM->DevDumpStatus();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("gota.Horde.Dump: no HordeGameMode authority world"));
		}
	}

	void HordeForceClearCommand()
	{
		if (AHordeGameMode* GM = GetHordeGameMode())
		{
			GM->DevForceClearWave();
			GM->DevDumpStatus();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("gota.Horde.ForceClear: no HordeGameMode authority world"));
		}
	}

	void HordeSkipPrepCommand()
	{
		if (AHordeGameMode* GM = GetHordeGameMode())
		{
			GM->DevSkipPrepare();
			GM->DevDumpStatus();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("gota.Horde.SkipPrep: no HordeGameMode authority world"));
		}
	}

	FAutoConsoleCommand HordeDumpCmd(
		TEXT("gota.Horde.Dump"),
		TEXT("Log Horde phase / wave / enemy counts"),
		FConsoleCommandDelegate::CreateStatic(&HordeDumpCommand));

	FAutoConsoleCommand HordeForceClearCmd(
		TEXT("gota.Horde.ForceClear"),
		TEXT("Destroy current wave enemies and advance clear (dev)"),
		FConsoleCommandDelegate::CreateStatic(&HordeForceClearCommand));

	FAutoConsoleCommand HordeSkipPrepCmd(
		TEXT("gota.Horde.SkipPrep"),
		TEXT("Skip Waiting/Preparing/Intermission into next wave (dev)"),
		FConsoleCommandDelegate::CreateStatic(&HordeSkipPrepCommand));

	void ValidateAllArenaDressCommand()
	{
		static const EArenaGrayboxLayout Layouts[] = {
			EArenaGrayboxLayout::TitansQuarry,
			EArenaGrayboxLayout::AshfallCrucible,
			EArenaGrayboxLayout::FrostboundCitadel,
			EArenaGrayboxLayout::VerdantSanctuary,
			EArenaGrayboxLayout::SunscarExpanse,
			EArenaGrayboxLayout::StormbreakCoast,
			EArenaGrayboxLayout::ShatteredDominion,
			EArenaGrayboxLayout::FallenSanctuary,
		};

		int32 Ok = 0;
		int32 SoftOk = 0;
		int32 SoftMiss = 0;
		for (EArenaGrayboxLayout Layout : Layouts)
		{
			const FSoftObjectPath Path = ArenaVisualDressDefaults::GetCanonicalDressPath(Layout);
			UArenaVisualDressDefinition* Dress = Cast<UArenaVisualDressDefinition>(Path.TryLoad());
			if (!Dress)
			{
				UE_LOG(LogTemp, Error, TEXT("gota.ValidateArenaDress FAIL layout=%d path=%s"),
					static_cast<int32>(Layout), *Path.ToString());
				continue;
			}
			++Ok;

			auto CheckPool = [&](const TArray<TSoftObjectPtr<UStaticMesh>>& Pool, const TCHAR* Label)
			{
				for (const TSoftObjectPtr<UStaticMesh>& Soft : Pool)
				{
					if (Soft.IsNull())
					{
						continue;
					}
					if (Soft.LoadSynchronous())
					{
						++SoftOk;
					}
					else
					{
						++SoftMiss;
						UE_LOG(LogTemp, Warning, TEXT("  missing %s soft %s"), Label, *Soft.ToSoftObjectPath().ToString());
					}
				}
			};
			CheckPool(Dress->CliffMeshPool, TEXT("cliff"));
			CheckPool(Dress->DebrisMeshPool, TEXT("debris"));
			CheckPool(Dress->PillarMeshPool, TEXT("pillar"));
			for (const FArenaDressMeshSlot& Slot : Dress->DressSlots)
			{
				if (Slot.Mesh.IsNull())
				{
					continue;
				}
				if (Slot.Mesh.LoadSynchronous())
				{
					++SoftOk;
				}
				else
				{
					++SoftMiss;
					UE_LOG(LogTemp, Warning, TEXT("  missing slot %s soft %s"),
						*Slot.SlotId.ToString(), *Slot.Mesh.ToSoftObjectPath().ToString());
				}
			}
			UE_LOG(LogTemp, Log, TEXT("gota.ValidateArenaDress OK %s cliffs=%d debris=%d pillars=%d slots=%d"),
				*Dress->GetName(), Dress->CliffMeshPool.Num(), Dress->DebrisMeshPool.Num(),
				Dress->PillarMeshPool.Num(), Dress->DressSlots.Num());
		}
		UE_LOG(LogTemp, Log, TEXT("gota.ValidateArenaDress summary DAs=%d/8 softOk=%d softMiss=%d"),
			Ok, SoftOk, SoftMiss);
	}

	FAutoConsoleCommand ValidateArenaDressCmd(
		TEXT("gota.ValidateArenaDress"),
		TEXT("Load all DA_Dress_* assets and soft-resolve mesh pools/slots"),
		FConsoleCommandDelegate::CreateStatic(&ValidateAllArenaDressCommand));
}

void FArenaConsoleCommands::Register()
{
	// FAutoConsoleCommand registers on construction.
}

void FArenaConsoleCommands::Unregister()
{
}
