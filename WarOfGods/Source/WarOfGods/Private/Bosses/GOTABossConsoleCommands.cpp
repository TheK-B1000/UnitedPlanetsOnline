// White Tiger Studios 2024
// Boss encounter test commands (Tools/Tests/boss_*.txt). Non-shipping only.

#include "Bosses/GOTABossComponent.h"
#include "Arenas/Duel/DuelGameMode.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"

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
		ADuelGameMode* GM = World ? World->GetAuthGameMode<ADuelGameMode>() : nullptr;
		APawn* Pawn = GM ? GM->GetFighterPawn(1) : nullptr;
		return Pawn ? Pawn->FindComponentByClass<UGOTABossComponent>() : nullptr;
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
		UWorld* World = GameWorld();
		ADuelGameMode* GM = World ? World->GetAuthGameMode<ADuelGameMode>() : nullptr;
		APawn* Player = GM ? GM->GetFighterPawn(0) : nullptr;
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

	static FAutoConsoleCommand CmdDump(TEXT("gota.Boss.Dump"), TEXT("Log boss state."), FConsoleCommandWithArgsDelegate::CreateStatic(&Dump));
	static FAutoConsoleCommand CmdAttack(TEXT("gota.Boss.Attack"), TEXT("Force the boss's next attack: <AttackId>."), FConsoleCommandWithArgsDelegate::CreateStatic(&Attack));
	static FAutoConsoleCommand CmdAI(TEXT("gota.Boss.AI"), TEXT("Boss brain on/off: 1|0."), FConsoleCommandWithArgsDelegate::CreateStatic(&AI));
	static FAutoConsoleCommand CmdSkip(TEXT("gota.Boss.Skip"), TEXT("Skip the boss intro."), FConsoleCommandWithArgsDelegate::CreateStatic(&Skip));
}

#endif
