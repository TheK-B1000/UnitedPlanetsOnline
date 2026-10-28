// White Tiger Studios 2024

#include "Combat/CombatTimingComponent.h"
#include "Combat/CombatTimingConfig.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace GOTACombatTimingConsole
{
	static UCombatTimingComponent* FindLocalTiming()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		UWorld* World = nullptr;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.World() && Ctx.World()->IsGameWorld())
			{
				World = Ctx.World();
				break;
			}
		}
		if (!World)
		{
			return nullptr;
		}
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				return Pawn->FindComponentByClass<UCombatTimingComponent>();
			}
		}
		return nullptr;
	}

	static void DumpTimingCommand()
	{
		UCombatTimingComponent* Timing = FindLocalTiming();
		if (!Timing)
		{
			UE_LOG(LogTemp, Warning, TEXT("gota.Combat.DumpTiming: no UCombatTimingComponent on local pawn"));
			return;
		}
		const UCombatTimingConfig* Cfg = Timing->GetActiveConfig();
		UE_LOG(LogTemp, Log, TEXT("gota.Combat.DumpTiming Guard=%d Invuln=%d ParryWindow=%.3f Perfect=%.3f DodgeIFrames=%.3f-%.3f DivineCost=%.1f"),
			static_cast<int32>(Timing->GetGuardState()),
			Timing->IsInvulnerable() ? 1 : 0,
			Cfg ? Cfg->ParryWindowSeconds : -1.f,
			Cfg ? Cfg->PerfectParryWindowSeconds : -1.f,
			Cfg ? Cfg->DodgeIFrameStartSeconds : -1.f,
			Cfg ? Cfg->DodgeIFrameEndSeconds : -1.f,
			Cfg ? Cfg->DivineCounterCost : -1.f);
	}

	static void ForceParryCommand()
	{
		if (UCombatTimingComponent* Timing = FindLocalTiming())
		{
			Timing->RequestParry();
			UE_LOG(LogTemp, Log, TEXT("gota.Combat.ForceParry"));
		}
	}

	static void ForceDodgeCommand()
	{
		if (UCombatTimingComponent* Timing = FindLocalTiming())
		{
			Timing->RequestDodge(FVector::ZeroVector);
			UE_LOG(LogTemp, Log, TEXT("gota.Combat.ForceDodge"));
		}
	}

	static void ForceDivineCommand()
	{
		if (UCombatTimingComponent* Timing = FindLocalTiming())
		{
			const bool bOk = Timing->RequestDivineCounter(nullptr);
			UE_LOG(LogTemp, Log, TEXT("gota.Combat.ForceDivine CounterStarted=%d (authority only returns true)"), bOk ? 1 : 0);
		}
	}

	static void ResolveTestCommand()
	{
		if (UCombatTimingComponent* Timing = FindLocalTiming())
		{
			const FCombatDefenseResolveResult Result = Timing->ResolveIncomingAttack(nullptr, FName(TEXT("ConsoleTest")), false);
			UE_LOG(LogTemp, Log, TEXT("gota.Combat.ResolveTest Outcome=%d Cancel=%d Age=%.3f"),
				static_cast<int32>(Result.Outcome), Result.bCancelDamage ? 1 : 0, Result.DefenseAgeSeconds);
		}
	}

	FAutoConsoleCommand DumpCmd(
		TEXT("gota.Combat.DumpTiming"),
		TEXT("Log local pawn combat timing windows / guard state"),
		FConsoleCommandDelegate::CreateStatic(&DumpTimingCommand));

	FAutoConsoleCommand ParryCmd(
		TEXT("gota.Combat.ForceParry"),
		TEXT("Request timed parry on local pawn"),
		FConsoleCommandDelegate::CreateStatic(&ForceParryCommand));

	FAutoConsoleCommand DodgeCmd(
		TEXT("gota.Combat.ForceDodge"),
		TEXT("Request dodge on local pawn"),
		FConsoleCommandDelegate::CreateStatic(&ForceDodgeCommand));

	FAutoConsoleCommand DivineCmd(
		TEXT("gota.Combat.ForceDivine"),
		TEXT("Request divine counter on local pawn"),
		FConsoleCommandDelegate::CreateStatic(&ForceDivineCommand));

	FAutoConsoleCommand ResolveCmd(
		TEXT("gota.Combat.ResolveTest"),
		TEXT("Resolve a fake incoming attack against local timing state"),
		FConsoleCommandDelegate::CreateStatic(&ResolveTestCommand));
}
