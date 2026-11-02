// White Tiger Studios 2024
// Boss encounter test commands (Tools/Tests/boss_*.txt). Non-shipping only.

#include "Bosses/GOTABossComponent.h"
#include "Bosses/GOTABossTypes.h"
#include "Arenas/Duel/DuelGameMode.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/GOTACombatLive.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "UObject/SoftObjectPath.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogGOTABossTest, Log, All);

namespace GOTABossTest
{
	static UWorld* GameWorld()
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.World() && (Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::PIE))
			{
				return Ctx.World();
			}
		}
		return nullptr;
	}

	static UGOTABossComponent* Boss()
	{
		UWorld* World = GameWorld();
		if (!World)
		{
			return nullptr;
		}
		if (ADuelGameMode* GM = World->GetAuthGameMode<ADuelGameMode>())
		{
			if (APawn* Pawn = GM->GetFighterPawn(1))
			{
				if (UGOTABossComponent* Comp = Pawn->FindComponentByClass<UGOTABossComponent>())
				{
					return Comp;
				}
			}
		}
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			if (UGOTABossComponent* Comp = It->FindComponentByClass<UGOTABossComponent>())
			{
				return Comp;
			}
		}
		return nullptr;
	}

	static APawn* LocalOrFirstPlayer()
	{
		UWorld* World = GameWorld();
		if (!World)
		{
			return nullptr;
		}
		if (ADuelGameMode* GM = World->GetAuthGameMode<ADuelGameMode>())
		{
			if (APawn* Pawn = GM->GetFighterPawn(0))
			{
				return Pawn;
			}
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					return Pawn;
				}
			}
		}
		return nullptr;
	}

	static void Dump(const TArray<FString>&)
	{
		UGOTABossComponent* B = Boss();
		if (!B)
		{
			UE_LOG(LogGOTABossTest, Warning, TEXT("BOSS none"));
			return;
		}
		const ACharacter* Char = Cast<ACharacter>(B->GetOwner());
		UAnimInstance* Anim = Char ? Char->GetMesh()->GetAnimInstance() : nullptr;
		UAnimMontage* M = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
		APawn* Player = LocalOrFirstPlayer();
		const FGOTACombatantVitals PV = UGOTACombatantObserver::ReadVitals(Player);
		UE_LOG(LogGOTABossTest, Log, TEXT("BOSS %s | mesh=%s anim=%s montage=%s speed=%.0f | player hp=%.1f/%.1f dist=%.0f"), *B->DescribeState(),
			Char ? *GetNameSafe(Char->GetMesh()->GetSkeletalMeshAsset()) : TEXT("-"), *GetNameSafe(Anim), *GetNameSafe(M), Char ? Char->GetVelocity().Size2D() : 0.f,
			PV.Health, PV.MaxHealth, (Player && Char) ? FVector::Dist2D(Player->GetActorLocation(), Char->GetActorLocation()) : -1.f);
	}

	static void Attack(const TArray<FString>& Args)
	{
		if (UGOTABossComponent* B = Boss(); B && Args.Num() > 0)
		{
			B->DebugForceAttack(FName(*Args[0]));
			UE_LOG(LogGOTABossTest, Log, TEXT("BOSS force attack %s"), *Args[0]);
		}
	}

	static void AI(const TArray<FString>& Args)
	{
		if (UGOTABossComponent* B = Boss(); B && Args.Num() > 0)
		{
			B->DebugSetAI(FCString::Atoi(*Args[0]) != 0);
		}
	}

	static void Skip(const TArray<FString>&)
	{
		if (UGOTABossComponent* B = Boss())
		{
			B->SkipIntro();
		}
	}

	/** gota.Boss.PlaceTarget <fwd> <right>: move the player to a point in the boss's frame (hit-arc tests). */
	static void PlaceTarget(const TArray<FString>& Args)
	{
		APawn* Player = LocalOrFirstPlayer();
		UGOTABossComponent* B = Boss();
		APawn* BossPawn = B ? Cast<APawn>(B->GetOwner()) : nullptr;
		if (!Player || !BossPawn || Args.Num() < 2)
		{
			return;
		}
		const FVector Fwd = BossPawn->GetActorForwardVector().GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd);
		FVector Loc = BossPawn->GetActorLocation() + Fwd * FCString::Atof(*Args[0]) + Right * FCString::Atof(*Args[1]);
		Loc.Z = Player->GetActorLocation().Z;
		Player->TeleportTo(Loc, (BossPawn->GetActorLocation() - Loc).Rotation(), false, true);
		UE_LOG(LogGOTABossTest, Log, TEXT("BOSS place target fwd=%s right=%s"), *Args[0], *Args[1]);
	}

	/** gota.Boss.Spawn <BossId>: server-only; spawn BP_Boss_<Id> with DA_Boss_<Id> for Story/Horde coop tests. */
	static void Spawn(const TArray<FString>& Args)
	{
		UWorld* World = GameWorld();
		if (!World || World->GetNetMode() == NM_Client || Args.Num() < 1)
		{
			UE_LOG(LogGOTABossTest, Warning, TEXT("BOSS spawn: server only, needs BossId"));
			return;
		}
		const FString BossId = Args[0];
		const FString ClassPath = FString::Printf(TEXT("/Game/Bosses/BP_Boss_%s.BP_Boss_%s_C"), *BossId, *BossId);
		const FString DefPath = FString::Printf(TEXT("/Game/Bosses/DA_Boss_%s.DA_Boss_%s"), *BossId, *BossId);
		UClass* BossClass = LoadClass<APawn>(nullptr, *ClassPath);
		UGOTABossDefinition* Def = LoadObject<UGOTABossDefinition>(nullptr, *DefPath);
		if (!BossClass || !Def)
		{
			UE_LOG(LogGOTABossTest, Warning, TEXT("BOSS spawn failed: class=%s def=%s"), *GetNameSafe(BossClass), *GetNameSafe(Def));
			return;
		}
		APawn* Anchor = LocalOrFirstPlayer();
		FVector Loc = Anchor ? Anchor->GetActorLocation() + Anchor->GetActorForwardVector() * 700.f : FVector(0, 0, 120);
		Loc.Z = Anchor ? Anchor->GetActorLocation().Z : 120.f;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APawn* BossPawn = World->SpawnActor<APawn>(BossClass, Loc, FRotator::ZeroRotator, Params);
		if (!BossPawn)
		{
			UE_LOG(LogGOTABossTest, Warning, TEXT("BOSS spawn actor failed"));
			return;
		}
		UGOTABossComponent::ApplyToPawn(BossPawn, Def);
		FGOTABlueprintCombat::ConfigureHealth(BossPawn, Def->MaxHealth);
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
			if (Pawn)
			{
				GOTACombatLive::EnsurePlayerCombat(Pawn, NAME_None);
			}
		}
		UE_LOG(LogGOTABossTest, Log, TEXT("BOSS spawned %s at %s hp=%.0f"), *BossId, *Loc.ToCompactString(), Def->MaxHealth);
	}

	/** gota.Test.Heal <slot> <amount>: heal a duel fighter through its own Blueprint Heal (long-fight tests). */
	static void Heal(const TArray<FString>& Args)
	{
		UWorld* World = GameWorld();
		ADuelGameMode* GM = World ? World->GetAuthGameMode<ADuelGameMode>() : nullptr;
		APawn* Pawn = (GM && Args.Num() > 1) ? GM->GetFighterPawn(FCString::Atoi(*Args[0])) : nullptr;
		if (Pawn)
		{
			const bool bOk = FGOTABlueprintCombat::Call(Pawn, TEXT("Heal"), nullptr, FString(), FCString::Atof(*Args[1]));
			UE_LOG(LogGOTABossTest, Log, TEXT("HEAL slot %s by %s ok=%d hp=%.0f"), *Args[0], *Args[1], bOk ? 1 : 0, UGOTACombatantObserver::ReadVitals(Pawn).Health);
		}
	}

	/** gota.Boss.PlaceOnAttack <delay> <fwd> <right>: once the boss's next attack starts, wait delay then PlaceTarget. */
	static FTSTicker::FDelegateHandle GPlaceTicker;
	static void PlaceOnAttack(const TArray<FString>& Args)
	{
		if (Args.Num() < 3)
		{
			return;
		}
		const float Delay = FCString::Atof(*Args[0]);
		const TArray<FString> PlaceArgs = { Args[1], Args[2] };
		FTSTicker::GetCoreTicker().RemoveTicker(GPlaceTicker);
		TSharedRef<double> StartedAt = MakeShared<double>(-1.0);
		GPlaceTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Delay, PlaceArgs, StartedAt](float)
		{
			UGOTABossComponent* B = Boss();
			if (!B)
			{
				return true;
			}
			const double Now = FPlatformTime::Seconds();
			if (*StartedAt < 0.0)
			{
				if (B->DescribeState().Contains(TEXT("state=Attacking")))
				{
					*StartedAt = Now;
					UE_LOG(LogGOTABossTest, Log, TEXT("BOSS attack seen: %s"), *B->DescribeState());
				}
				return true;
			}
			if (Now - *StartedAt >= Delay)
			{
				PlaceTarget(PlaceArgs);
				return false;
			}
			return true;
		}));
	}
	static FAutoConsoleCommand CmdPlaceOnAttack(TEXT("gota.Boss.PlaceOnAttack"), TEXT("After the boss's next attack starts, wait <delay>s then place the player at <fwd> <right>."), FConsoleCommandWithArgsDelegate::CreateStatic(&PlaceOnAttack));
	static FAutoConsoleCommand CmdPlaceTarget(TEXT("gota.Boss.PlaceTarget"), TEXT("Move the player into the boss's frame: <fwd> <right>."), FConsoleCommandWithArgsDelegate::CreateStatic(&PlaceTarget));
	static FAutoConsoleCommand CmdSpawn(TEXT("gota.Boss.Spawn"), TEXT("Server: spawn BP_Boss_<Id> with DA_Boss_<Id> near the local player."), FConsoleCommandWithArgsDelegate::CreateStatic(&Spawn));
	static FAutoConsoleCommand CmdHeal(TEXT("gota.Test.Heal"), TEXT("Heal a duel fighter: <slot> <amount>."), FConsoleCommandWithArgsDelegate::CreateStatic(&Heal));
	static FAutoConsoleCommand CmdDump(TEXT("gota.Boss.Dump"), TEXT("Log boss state."), FConsoleCommandWithArgsDelegate::CreateStatic(&Dump));
	static FAutoConsoleCommand CmdAttack(TEXT("gota.Boss.Attack"), TEXT("Force the boss's next attack: <AttackId>."), FConsoleCommandWithArgsDelegate::CreateStatic(&Attack));
	static FAutoConsoleCommand CmdAI(TEXT("gota.Boss.AI"), TEXT("Boss brain on/off: 1|0."), FConsoleCommandWithArgsDelegate::CreateStatic(&AI));
	static FAutoConsoleCommand CmdSkip(TEXT("gota.Boss.Skip"), TEXT("Skip the boss intro."), FConsoleCommandWithArgsDelegate::CreateStatic(&Skip));
}

#endif
