// White Tiger Studios 2024

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	void LaunchDuelCommand()
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
			UE_LOG(LogTemp, Warning, TEXT("gota.Duel: no game world"));
			return;
		}

		UGameplayStatics::OpenLevel(
			World,
			FName(TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap")),
			true,
			TEXT("game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?listen"));
		UE_LOG(LogTemp, Log, TEXT("gota.Duel -> Titan's Quarry 1v1 PvP (best of 3)"));
	}

	FAutoConsoleCommand LaunchDuelCmd(
		TEXT("gota.Duel"),
		TEXT("Travel to Titan's Quarry 1v1 PvP Duel (best of 3). Use PIE Listen Server + 2 players."),
		FConsoleCommandDelegate::CreateStatic(&LaunchDuelCommand));
}
