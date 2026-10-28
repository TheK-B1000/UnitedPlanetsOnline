// White Tiger Studios 2024

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Story/GOTAStoryGameMode.h"

namespace
{
	void LaunchStoryCommand(const TArray<FString>& Args)
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
			UE_LOG(LogTemp, Warning, TEXT("gota.Story: no game world"));
			return;
		}

		FString Options = TEXT("game=/Script/WarOfGods.GOTAStoryGameMode?Mode=Story");
		if (Args.Num() > 0)
		{
			Options += FString::Printf(TEXT("?Checkpoint=%s"), *Args[0]);
		}

		// Prefer dedicated prologue map when present.
		FString MapName = TEXT("/Game/Story/Maps/L_GOTA_Story_Prologue");
		if (!FPackageName::DoesPackageExist(MapName))
		{
			MapName = TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap");
			UE_LOG(LogTemp, Log, TEXT("gota.Story: prologue map missing — using ThirdPersonMap fallback"));
		}

		UGameplayStatics::OpenLevel(World, FName(*MapName), true, Options);
		UE_LOG(LogTemp, Log, TEXT("gota.Story -> Chapter 1 Fallen Sanctuary"));
	}

	void StoryReturnMenuCommand()
	{
		if (!GEngine)
		{
			return;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* World = Context.World())
			{
				if (AGOTAStoryGameMode* GM = World->GetAuthGameMode<AGOTAStoryGameMode>())
				{
					GM->RequestReturnToMenu();
					return;
				}
			}
		}
	}

	FAutoConsoleCommand LaunchStoryCmd(
		TEXT("gota.Story"),
		TEXT("Travel to Chapter 1 Fallen Sanctuary. Optional: gota.Story DivineShrine"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&LaunchStoryCommand));

	FAutoConsoleCommand StoryMenuCmd(
		TEXT("gota.StoryReturnMenu"),
		TEXT("Return to main menu from Story GameMode"),
		FConsoleCommandDelegate::CreateStatic(&StoryReturnMenuCommand));
}
