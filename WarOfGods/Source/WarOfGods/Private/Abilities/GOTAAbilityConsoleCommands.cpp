// White Tiger Studios 2024
// Test commands for hero divine abilities (used by Tools/Tests/ability_*.txt). Non-shipping only.

#include "Abilities/GOTAHeroAbilityComponent.h"
#include "Arenas/Duel/DuelGameMode.h"
#include "Characters/StatsComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Containers/Ticker.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "EngineUtils.h"
#include "Characters/GOTAHeroBodyComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Arenas/Duel/GOTADuelCombat.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogGOTAAbilityTest, Log, All);

namespace GOTAAbilityTest
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

	static UGOTAHeroAbilityComponent* Abilities(int32 FighterSlot)
	{
		UWorld* World = GameWorld();
		ADuelGameMode* GM = World ? World->GetAuthGameMode<ADuelGameMode>() : nullptr;
		APawn* Pawn = GM ? GM->GetFighterPawn(FighterSlot) : nullptr;
		return Pawn ? Pawn->FindComponentByClass<UGOTAHeroAbilityComponent>() : nullptr;
	}

	/** gota.Ability.Use <fighter> <slot>: same path as the key press (server-validated). */
	static void Use(const TArray<FString>& Args)
	{
		const int32 Fighter = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		const int32 Slot = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
		if (UGOTAHeroAbilityComponent* A = Abilities(Fighter))
		{
			UE_LOG(LogGOTAAbilityTest, Log, TEXT("ABILITY use fighter=%d slot=%d (%s)"), Fighter, Slot, A->GetAbility(Slot) ? *A->GetAbility(Slot)->AbilityId.ToString() : TEXT("?"));
			A->RequestActivate(Slot);
		}
		else
		{
			UE_LOG(LogGOTAAbilityTest, Warning, TEXT("ABILITY use: fighter %d has no abilities"), Fighter);
		}
	}

	/** gota.Ability.Dump: one line per fighter (abilities, cooldowns, status, health, momentum). */
	static void Dump(const TArray<FString>& Args)
	{
		for (int32 F = 0; F < 2; ++F)
		{
			UWorld* World = GameWorld();
			ADuelGameMode* GM = World ? World->GetAuthGameMode<ADuelGameMode>() : nullptr;
			APawn* Pawn = GM ? GM->GetFighterPawn(F) : nullptr;
			const UGOTAHeroAbilityComponent* A = Pawn ? Pawn->FindComponentByClass<UGOTAHeroAbilityComponent>() : nullptr;
			const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Pawn);
			const UStatsComponent* Stats = Pawn ? Pawn->FindComponentByClass<UStatsComponent>() : nullptr;
			UE_LOG(LogGOTAAbilityTest, Log, TEXT("ABILITY f%d %s hp=%.1f/%.1f momentum=%.0f at=%s %s"), F, *GetNameSafe(Pawn), V.Health, V.MaxHealth,
				Stats ? Stats->GetDivineMomentum() : -1.f, Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("-"),
				A ? *A->DescribeState() : TEXT("(no abilities)"));
			if (const ACharacter* Char = Cast<ACharacter>(Pawn))
			{
				const UGOTAHeroBodyComponent* BodyComp = Char->FindComponentByClass<UGOTAHeroBodyComponent>();
				USkeletalMeshComponent* Body = BodyComp ? BodyComp->GetBodyMesh() : nullptr;
				UAnimInstance* Anim = Body ? Body->GetAnimInstance() : nullptr;
				UAnimMontage* M = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
				UE_LOG(LogGOTAAbilityTest, Log, TEXT("ABILITY f%d body=%s anim=%s montage=%s pos=%.2f slotWeight=%.2f"), F, Body ? *GetNameSafe(Body->GetSkeletalMeshAsset()) : TEXT("-"),
					*GetNameSafe(Anim), *GetNameSafe(M), M ? Anim->Montage_GetPosition(M) : -1.f, Anim ? Anim->GetSlotMontageGlobalWeight(FName(TEXT("DefaultSlot"))) : -1.f);
				const UCharacterMovementComponent* Move = Char->GetCharacterMovement();
				UE_LOG(LogGOTAAbilityTest, Log, TEXT("ABILITY f%d move walk=%.0f timeDilation=%.2f meshAnimRate=%.2f"), F, Move ? Move->MaxWalkSpeed : -1.f,
					Char->CustomTimeDilation, Char->GetMesh() ? Char->GetMesh()->GlobalAnimRateScale : -1.f);
			}
		}
	}

	/** gota.Ability.AI <slot> | off | on: force the AI fighter's next ability, or toggle its ability brain. */
	static void AI(const TArray<FString>& Args)
	{
		UGOTAHeroAbilityComponent* A = Abilities(1);
		if (!A || Args.Num() == 0)
		{
			return;
		}
		if (Args[0] == TEXT("off") || Args[0] == TEXT("on"))
		{
			A->bAIEnabled = Args[0] == TEXT("on");
			UE_LOG(LogGOTAAbilityTest, Log, TEXT("ABILITY AI %s"), A->bAIEnabled ? TEXT("on") : TEXT("off"));
			return;
		}
		A->DebugAIUse(FCString::Atoi(*Args[0]));
	}

	/** gota.Ability.Place <dist> [freeze 0|1]: put fighter 1 <dist> in front of fighter 0, facing it (test setup). */
	static void Place(const TArray<FString>& Args)
	{
		UWorld* World = GameWorld();
		ADuelGameMode* GM = World ? World->GetAuthGameMode<ADuelGameMode>() : nullptr;
		APawn* A = GM ? GM->GetFighterPawn(0) : nullptr;
		APawn* B = GM ? GM->GetFighterPawn(1) : nullptr;
		if (!A || !B)
		{
			return;
		}
		const float Dist = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 250.f;
		const FVector Fwd = A->GetActorForwardVector().GetSafeNormal2D();
		// Optional 3rd arg: sideways offset (cm, + = right of fighter 0) for sidestep / miss tests.
		const float Side = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.f;
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd);
		const FVector Loc = A->GetActorLocation() + Fwd * Dist + Right * Side;
		B->TeleportTo(Loc, (-Fwd).Rotation(), false, true);
		if (Args.Num() > 1)
		{
			const bool bFreeze = FCString::Atoi(*Args[1]) != 0;
			if (AAIController* AIC = Cast<AAIController>(B->GetController()))
			{
				AIC->StopMovement();
				if (UBrainComponent* Brain = AIC->GetBrainComponent())
				{
					if (bFreeze) { Brain->PauseLogic(TEXT("test freeze")); } else { Brain->ResumeLogic(TEXT("test freeze")); }
				}
			}
			if (UActorComponent* Director = B->FindComponentByClass<UGOTAChampionDirectorComponent>())
			{
				Director->SetComponentTickEnabled(!bFreeze);
			}
		}
		UE_LOG(LogGOTAAbilityTest, Log, TEXT("ABILITY place fighter1 %.0f in front of fighter0 -> %s"), Dist, *B->GetActorLocation().ToCompactString());
	}

	/** gota.Ability.Slomo <x>: global time dilation for frame-by-frame capture (cheat-free 'slomo'). */
	static void Slomo(const TArray<FString>& Args)
	{
		if (UWorld* World = GameWorld())
		{
			const float X = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1.f;
			UGameplayStatics::SetGlobalTimeDilation(World, FMath::Clamp(X, 0.0001f, 20.f));
			UE_LOG(LogGOTAAbilityTest, Log, TEXT("ABILITY slomo %.3f"), X);
		}
	}
	static FAutoConsoleCommand CmdSlomo(TEXT("gota.Ability.Slomo"), TEXT("Test: global time dilation <x>."), FConsoleCommandWithArgsDelegate::CreateStatic(&Slomo));

	/** gota.Ability.Nav: runtime navigation data report (actors, tiles, bounds, projection at fighters). */
	static void NavReport(const TArray<FString>& Args)
	{
		UWorld* World = GameWorld();
		if (!World)
		{
			return;
		}
		UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		int32 NavActors = 0, Tiles = 0, Bounds = 0;
		for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
		{
			++NavActors;
			Tiles += It->GetNavMeshTilesCount();
			UE_LOG(LogGOTAAbilityTest, Log, TEXT("NAV %s at %s tiles=%d runtimeGen=%d"), *It->GetName(), *It->GetActorLocation().ToCompactString(), It->GetNavMeshTilesCount(), (int32)It->GetRuntimeGenerationMode());
		}
		for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It)
		{
			++Bounds;
			UE_LOG(LogGOTAAbilityTest, Log, TEXT("NAV bounds %s extent=%s"), *It->GetName(), *It->GetComponentsBoundingBox(true).GetExtent().ToCompactString());
		}
		UE_LOG(LogGOTAAbilityTest, Log, TEXT("NAV navsys=%d navActors=%d tiles=%d bounds=%d building=%d"), Nav ? 1 : 0, NavActors, Tiles, Bounds, Nav && Nav->IsNavigationBuildInProgress() ? 1 : 0);
	}
	static FAutoConsoleCommand CmdNav(TEXT("gota.Ability.Nav"), TEXT("Log runtime navigation data state."), FConsoleCommandWithArgsDelegate::CreateStatic(&NavReport));

	/** gota.Ability.PlaceOnCast <delay> <dist> [side]: when fighter 0's next cast starts, wait delay (real s) then Place. */
	static FTSTicker::FDelegateHandle GPlaceOnCast;
	static void PlaceOnCast(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			return;
		}
		const float Delay = FCString::Atof(*Args[0]);
		TArray<FString> PlaceArgs = { Args[1], TEXT("1") };
		if (Args.Num() > 2)
		{
			PlaceArgs.Add(Args[2]);
		}
		FTSTicker::GetCoreTicker().RemoveTicker(GPlaceOnCast);
		TSharedRef<double> Started = MakeShared<double>(-1.0);
		GPlaceOnCast = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Delay, PlaceArgs, Started](float)
		{
			UGOTAHeroAbilityComponent* A = Abilities(0);
			if (!A)
			{
				return true;
			}
			const double Now = FPlatformTime::Seconds();
			if (*Started < 0.0)
			{
				if (A->IsBusy())
				{
					*Started = Now;
				}
				return true;
			}
			if (Now - *Started >= Delay)
			{
				Place(PlaceArgs);
				return false;
			}
			return true;
		}));
	}
	/** gota.Ability.Status <fighter> <None|Unbreakable|Veiled|Dread|Slowed> <seconds> <magnitude>: server-side status (tests). */
	static void Status(const TArray<FString>& Args)
	{
		if (Args.Num() < 4)
		{
			return;
		}
		UWorld* World = GameWorld();
		ADuelGameMode* GM = World ? World->GetAuthGameMode<ADuelGameMode>() : nullptr;
		APawn* Pawn = GM ? GM->GetFighterPawn(FCString::Atoi(*Args[0])) : nullptr;
		const int64 Value = StaticEnum<EGOTAStatus>()->GetValueByNameString(Args[1]);
		if (!Pawn || Value == INDEX_NONE)
		{
			return;
		}
		UGOTAHeroAbilityComponent* A = Pawn->FindComponentByClass<UGOTAHeroAbilityComponent>();
		if (!A)
		{
			A = NewObject<UGOTAHeroAbilityComponent>(Pawn, TEXT("GOTAHeroAbilities"));
			A->RegisterComponent();
		}
		A->ApplyStatus(static_cast<EGOTAStatus>(Value), FCString::Atof(*Args[2]), FCString::Atof(*Args[3]), nullptr);
	}
	static FAutoConsoleCommand CmdStatus(TEXT("gota.Ability.Status"), TEXT("Apply a status: <fighter> <Status> <seconds> <magnitude>."), FConsoleCommandWithArgsDelegate::CreateStatic(&Status));
	static FAutoConsoleCommand CmdPlaceOnCast(TEXT("gota.Ability.PlaceOnCast"), TEXT("After fighter 0's next cast starts, wait <delay>s then Place <dist> (frozen) [side]."), FConsoleCommandWithArgsDelegate::CreateStatic(&PlaceOnCast));
	static FAutoConsoleCommand CmdPlace(TEXT("gota.Ability.Place"), TEXT("Test: place fighter 1 <dist> in front of fighter 0 [freeze 0|1] [side cm]."), FConsoleCommandWithArgsDelegate::CreateStatic(&Place));
	static FAutoConsoleCommand CmdUse(TEXT("gota.Ability.Use"), TEXT("Use hero ability: <fighter 0|1> <slot 0-3>."), FConsoleCommandWithArgsDelegate::CreateStatic(&Use));
	static FAutoConsoleCommand CmdDump(TEXT("gota.Ability.Dump"), TEXT("Log both fighters' ability state."), FConsoleCommandWithArgsDelegate::CreateStatic(&Dump));
	static FAutoConsoleCommand CmdAI(TEXT("gota.Ability.AI"), TEXT("AI fighter abilities: <slot> to force | on | off."), FConsoleCommandWithArgsDelegate::CreateStatic(&AI));
}

#endif
