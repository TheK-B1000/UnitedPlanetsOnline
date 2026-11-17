// White Tiger Studios 2024
//
// Multiplayer diagnostics and test hooks (every mode: Duel, Horde, Story). Development / editor builds only.
//   gota.Net.Players            - one line per player: role, pawn, health, mode state, ping (any machine)
//   gota.Net.Hurt <idx> <amt>   - server: damage player <idx> (GameState PlayerArray order) via its own BP damage system
//   gota.Net.AuditRPCs          - list every loaded game class's network RPCs and replicated property counts
//   gota.Net.InspectDamage <s>  - damage-system properties, functions and delegates of the first actor whose class contains <s>

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Abilities/GAS/GOTAAbilitySystemComponent.h"
#include "Abilities/GAS/GOTAGameplayTags.h"
#include "Abilities/GAS/GOTAStatusEffects.h"
#include "Abilities/GOTAHeroAbilityComponent.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Arenas/Horde/HordeEncounterDirector.h"
#include "Arenas/Horde/HordePlayerState.h"
#include "Characters/GOTAHeroBodyComponent.h"
#include "Characters/GOTAPlayerCharacter.h"
#include "Combat/GOTACombatLive.h"
#include "Combat/GOTACombatantObserver.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "Multiplayer/GotaSessionSubsystem.h"
#include "RHI.h"
#include "TimerManager.h"
#include "Bosses/GOTABossComponent.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Characters/StatsComponent.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTANet, Log, All);

namespace GOTANetTest
{
	static UWorld* GameWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	static const TCHAR* NetModeName(const UWorld* W)
	{
		switch (W ? W->GetNetMode() : NM_Standalone)
		{
		case NM_ListenServer: return TEXT("listen-server");
		case NM_DedicatedServer: return TEXT("dedicated-server");
		case NM_Client: return TEXT("client");
		default: return TEXT("standalone");
		}
	}

	static void Players(const TArray<FString>&)
	{
		UWorld* W = GameWorld();
		const AGameStateBase* GS = W ? W->GetGameState() : nullptr;
		if (!GS)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET no game state"));
			return;
		}
		UE_LOG(LogGOTANet, Log, TEXT("NET %s map=%s gs=%s players=%d"), NetModeName(W), *W->GetMapName(), *GS->GetClass()->GetName(), GS->PlayerArray.Num());
		for (int32 i = 0; i < GS->PlayerArray.Num(); ++i)
		{
			const APlayerState* PS = GS->PlayerArray[i];
			if (!PS)
			{
				continue;
			}
			const APawn* Pawn = PS->GetPawn();
			const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Pawn);
			FString Mode;
			if (const ADuelPlayerState* D = Cast<ADuelPlayerState>(PS))
			{
				Mode = FString::Printf(TEXT("duelSlot=%d hero=%s rounds=%d"), D->DuelSlot, *D->HeroId.ToString(), D->RoundsWon);
			}
			else if (const AHordePlayerState* H = Cast<AHordePlayerState>(PS))
			{
				Mode = FString::Printf(TEXT("eliminated=%d kills=%d elites=%d score=%d"), H->bIsEliminated ? 1 : 0, H->Eliminations, H->EliteEliminations, H->PersonalScore);
			}
			const UGOTADuelFighterComponent* F = Pawn ? Pawn->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr;
			if (F)
			{
				Mode += FString::Printf(TEXT(" dodgeProtect=%d opp=%s"), F->IsDodgeProtected() ? 1 : 0, *GetNameSafe(F->GetOpponent()));
			}
			Mode += FString::Printf(TEXT(" combatLive=%d"), GOTACombatLive::IsCombatLive(W) ? 1 : 0);
			if (const AGOTAPlayerCharacter* Guarded = Cast<AGOTAPlayerCharacter>(Pawn))
			{
				Mode += FString::Printf(TEXT(" rejectedRPCs=%d dilation=%.2f"), Guarded->GetRejectedRPCs(), Guarded->CustomTimeDilation);
			}
			if (const UGOTAHeroBodyComponent* Body = Pawn ? Pawn->FindComponentByClass<UGOTAHeroBodyComponent>() : nullptr)
			{
				const USkeletalMeshComponent* Mesh = Body->GetBodyMesh();
				Mode += FString::Printf(TEXT(" body=%s mesh=%s"), *Body->GetHeroId().ToString(), *GetNameSafe(Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr));
			}
			UE_LOG(LogGOTANet, Log, TEXT("NET  [%d] %s local=%d pawn=%s hp=%.0f/%.0f dead=%d mom=%.0f ping=%.0fms %s"), i, *PS->GetPlayerName(),
				(PS->GetPlayerController() && PS->GetPlayerController()->IsLocalController()) ? 1 : 0, *GetNameSafe(Pawn), V.Health, V.MaxHealth,
				UGOTACombatantObserver::IsCombatantDefeated(Pawn) && Pawn ? 1 : 0, F ? F->GetMomentum() : -1.f, PS->GetPingInMilliseconds(), *Mode);
		}
	}

	static void Hurt(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		const AGameStateBase* GS = W ? W->GetGameState() : nullptr;
		const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		const float Amount = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 50.f;
		if (!GS || W->GetNetMode() == NM_Client || !GS->PlayerArray.IsValidIndex(Index))
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET hurt: server only, valid player index required"));
			return;
		}
		APawn* Pawn = GS->PlayerArray[Index]->GetPawn();
		const bool bLanded = FGOTABlueprintCombat::ApplyDamage(Pawn, nullptr, Amount, false, false, TEXT("Stagger"));
		UE_LOG(LogGOTANet, Log, TEXT("NET hurt [%d] %s for %.0f (landed=%d) -> hp=%.0f"), Index, *GetNameSafe(Pawn), Amount, bLanded ? 1 : 0,
			UGOTACombatantObserver::ReadVitals(Pawn).Health);
	}

	/** Server: player <idx> defeats the nearest living enemy through its own damage system (causer = that player's pawn). */
	static void KillEnemy(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		const AGameStateBase* GS = W ? W->GetGameState() : nullptr;
		const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		if (!GS || W->GetNetMode() == NM_Client || !GS->PlayerArray.IsValidIndex(Index) || !GS->PlayerArray[Index]->GetPawn())
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET killenemy: server only, valid player index with a pawn required"));
			return;
		}
		APawn* Killer = GS->PlayerArray[Index]->GetPawn();
		// Optional filter: "boss" = only a boss, "adds" = anything but a boss (diagnostics of the real boss-death path).
		const FString Filter = Args.Num() > 1 ? Args[1].ToLower() : FString();
		APawn* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (TActorIterator<APawn> It(W); It; ++It)
		{
			if (It->IsPlayerControlled() || !UGOTACombatantObserver::FindDamageSystem(*It) || UGOTACombatantObserver::IsCombatantDefeated(*It))
			{
				continue;
			}
			const bool bIsBoss = It->FindComponentByClass<UGOTABossComponent>() != nullptr;
			if ((Filter == TEXT("boss") && !bIsBoss) || (Filter == TEXT("adds") && bIsBoss))
			{
				continue;
			}
			const float D = FVector::Dist(It->GetActorLocation(), Killer->GetActorLocation());
			if (D < BestDist)
			{
				BestDist = D;
				Best = *It;
			}
		}
		if (!Best)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET killenemy: no living enemy"));
			return;
		}
		const bool bLanded = FGOTABlueprintCombat::ApplyDamage(Best, Killer, 99999.f, false, false, TEXT("Stagger"));
		UE_LOG(LogGOTANet, Log, TEXT("NET killenemy: [%d] %s -> %s (landed=%d)"), Index, *GetNameSafe(Killer), *GetNameSafe(Best), bLanded ? 1 : 0);
	}

	/**
	 * Exploit test, run on a CLIENT: sends the Blueprint's SR_ParryAttack for its own pawn naming the farthest pawn as
	 * the target - what a modified client could do. The server's AGOTAPlayerCharacter must reject it.
	 */
	static void ForgeParry(const TArray<FString>&)
	{
		UWorld* W = GameWorld();
		APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
		APawn* Me = PC ? PC->GetPawn() : nullptr;
		UFunction* Fn = Me ? Me->FindFunction(TEXT("SR_ParryAttack")) : nullptr;
		if (!Fn)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET forgeparry: no local pawn with SR_ParryAttack"));
			return;
		}
		APawn* Far = nullptr;
		float FarDist = -1.f;
		for (TActorIterator<APawn> It(W); It; ++It)
		{
			const float D = FVector::Dist(It->GetActorLocation(), Me->GetActorLocation());
			if (*It != Me && D > FarDist)
			{
				FarDist = D;
				Far = *It;
			}
		}
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Fn->GetStructureSize(), 1)));
		FMemory::Memzero(Params, Fn->GetStructureSize());
		Fn->InitializeStruct(Params);
		for (TFieldIterator<FProperty> P(Fn); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
		{
			if (FObjectPropertyBase* O = CastField<FObjectPropertyBase>(*P))
			{
				O->SetObjectPropertyValue_InContainer(Params, Far);
			}
		}
		UE_LOG(LogGOTANet, Log, TEXT("NET forgeparry: %s sends SR_ParryAttack at %s (%.0f cm away, not blocking)"), *GetNameSafe(Me), *GetNameSafe(Far), FarDist);
		Me->ProcessEvent(Fn, Params);
		Fn->DestroyStruct(Params);
	}

	static bool IsGameClass(const UClass* C)
	{
		const FString Path = C->GetPathName();
		return Path.StartsWith(TEXT("/Game/")) || Path.StartsWith(TEXT("/Script/WarOfGods."));
	}

	static void AuditRPCs(const TArray<FString>&)
	{
		int32 Classes = 0;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* C = *It;
			if (!IsGameClass(C) || C->HasAnyClassFlags(CLASS_NewerVersionExists | CLASS_Deprecated) || C->GetName().StartsWith(TEXT("SKEL_")) || C->GetName().StartsWith(TEXT("REINST_")))
			{
				continue;
			}
			TArray<FString> Lines;
			for (TFieldIterator<UFunction> F(C, EFieldIteratorFlags::ExcludeSuper); F; ++F)
			{
				if (!F->HasAnyFunctionFlags(FUNC_Net))
				{
					continue;
				}
				const TCHAR* Kind = F->HasAnyFunctionFlags(FUNC_NetServer) ? TEXT("Server") : F->HasAnyFunctionFlags(FUNC_NetClient) ? TEXT("Client")
					: F->HasAnyFunctionFlags(FUNC_NetMulticast) ? TEXT("Multicast") : TEXT("Net");
				FString Params;
				for (TFieldIterator<FProperty> P(*F); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
				{
					Params += FString::Printf(TEXT("%s:%s "), *P->GetName(), *P->GetCPPType());
				}
				Lines.Add(FString::Printf(TEXT("    %s %s %s(%s)"), Kind, F->HasAnyFunctionFlags(FUNC_NetReliable) ? TEXT("Reliable") : TEXT("Unreliable"),
					*F->GetName(), *Params.TrimEnd()));
			}
			int32 Replicated = 0;
			FString RepNames;
			for (TFieldIterator<FProperty> P(C, EFieldIteratorFlags::ExcludeSuper); P; ++P)
			{
				if (P->HasAnyPropertyFlags(CPF_Net))
				{
					++Replicated;
					RepNames += P->GetName() + TEXT(" ");
				}
			}
			if (Lines.Num() == 0 && Replicated == 0)
			{
				continue;
			}
			++Classes;
			UE_LOG(LogGOTANet, Log, TEXT("RPCAUDIT %s (%d RPCs, %d replicated: %s)"), *C->GetPathName(), Lines.Num(), Replicated, *RepNames.Left(400));
			for (const FString& L : Lines)
			{
				UE_LOG(LogGOTANet, Log, TEXT("RPCAUDIT%s"), *L);
			}
		}
		UE_LOG(LogGOTANet, Log, TEXT("RPCAUDIT done: %d classes"), Classes);
	}

	static void InspectDamage(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		const FString Needle = Args.Num() > 0 ? Args[0] : TEXT("Enemy");
		AActor* Target = nullptr;
		for (TActorIterator<APawn> It(W); It && !Target; ++It)
		{
			if (It->GetClass()->GetName().Contains(Needle))
			{
				Target = *It;
			}
		}
		UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Target);
		if (Target)
		{
			UE_LOG(LogGOTANet, Log, TEXT("INSPECT actor %s CanBeDamaged=%d hidden=%d hp=%.1f at=%s"), *GetNameSafe(Target), Target->CanBeDamaged() ? 1 : 0, Target->IsHidden() ? 1 : 0,
				UGOTACombatantObserver::ReadVitals(Target).Health, *Target->GetActorLocation().ToCompactString());
			// The actor Blueprint's own variables (not inherited engine ones): state such as floating or teleporting.
			for (TFieldIterator<FProperty> It(Target->GetClass()); It; ++It)
			{
				if (It->GetOwnerClass() && It->GetOwnerClass()->IsNative())
				{
					continue;
				}
				const FStructProperty* StructProp = CastField<FStructProperty>(*It);
				const bool bDamageStruct = StructProp && StructProp->Struct && StructProp->Struct->GetName().Contains(TEXT("Damage"));
				if (It->IsA<FBoolProperty>() || It->IsA<FNumericProperty>() || It->IsA<FObjectPropertyBase>() || It->IsA<FEnumProperty>() || bDamageStruct)
				{
					FString Value;
					It->ExportTextItem_InContainer(Value, Target, nullptr, nullptr, PPF_None);
					UE_LOG(LogGOTANet, Log, TEXT("INSPECT  actor.%s = %s"), *It->GetName(), *Value.Left(bDamageStruct ? 600 : 100));
				}
			}
			if (const APawn* TargetPawn = Cast<APawn>(Target))
			{
				const AAIController* AIC = Cast<AAIController>(TargetPawn->GetController());
				if (const UBehaviorTreeComponent* BT = AIC ? Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent()) : nullptr)
				{
					UE_LOG(LogGOTANet, Log, TEXT("INSPECT  brain running=%d paused=%d task=%s"), BT->IsRunning() ? 1 : 0, BT->IsPaused() ? 1 : 0,
						*BT->DescribeActiveTasks().Replace(TEXT("\n"), TEXT(" | ")));
				}
				else
				{
					UE_LOG(LogGOTANet, Log, TEXT("INSPECT  brain none (controller %s)"), *GetNameSafe(TargetPawn->GetController()));
				}
			}
		}
		if (!DS)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("INSPECT no damage system on %s"), *GetNameSafe(Target));
			return;
		}
		UE_LOG(LogGOTANet, Log, TEXT("INSPECT %s damage system %s (replicated=%d)"), *GetNameSafe(Target), *DS->GetClass()->GetName(), DS->GetIsReplicated() ? 1 : 0);
		for (TFieldIterator<FProperty> It(DS->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			FString Sig;
			if (const FMulticastDelegateProperty* MD = CastField<FMulticastDelegateProperty>(*It))
			{
				for (TFieldIterator<FProperty> P(MD->SignatureFunction); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
				{
					Sig += FString::Printf(TEXT("%s:%s "), *P->GetName(), *P->GetCPPType());
				}
				UE_LOG(LogGOTANet, Log, TEXT("INSPECT  delegate %s(%s)"), *It->GetName(), *Sig.TrimEnd());
				continue;
			}
			FString Value;
			if (It->IsA<FBoolProperty>() || It->IsA<FNumericProperty>() || It->IsA<FEnumProperty>() || It->IsA<FNameProperty>() || It->IsA<FObjectPropertyBase>())
			{
				It->ExportTextItem_InContainer(Value, DS, nullptr, nullptr, PPF_None);
			}
			UE_LOG(LogGOTANet, Log, TEXT("INSPECT  prop %s : %s%s = %s"), *It->GetName(), *It->GetCPPType(), It->HasAnyPropertyFlags(CPF_Net) ? TEXT(" [replicated]") : TEXT(""), *Value.Left(120));
		}
	}

	static bool CallVector(UObject* Obj, FName Function, const FVector& Value)
	{
		UFunction* Fn = Obj ? Obj->FindFunction(Function) : nullptr;
		if (!Fn)
		{
			return false;
		}
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Fn->GetStructureSize(), 1)));
		FMemory::Memzero(Params, Fn->GetStructureSize());
		Fn->InitializeStruct(Params);
		for (TFieldIterator<FProperty> P(Fn); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
		{
			if (FStructProperty* SP = CastField<FStructProperty>(*P); SP && SP->Struct == TBaseStructure<FVector>::Get())
			{
				*SP->ContainerPtrToValuePtr<FVector>(Params) = Value;
			}
		}
		Obj->ProcessEvent(Fn, Params);
		Fn->DestroyStruct(Params);
		return true;
	}

	static FTSTicker::FDelegateHandle RemoteParryHandle;
	static double RemoteParryUntil = 0.0;
	static double RemoteParryNext = 0.0;
	static float RemoteParryLead = 0.35f;
	static int32 RemoteParryBlocks = 0;
	static int32 RemoteParryDodges = 0;
	static int32 RemoteCounters = 0;
	static bool bRemoteCounterDone = false;
	static FString RemoteMode = TEXT("parry");

	static bool TickRemoteParry(float)
	{
		UWorld* World = GameWorld();
		if (!World || World->GetTimeSeconds() > RemoteParryUntil)
		{
			UE_LOG(LogGOTANet, Log, TEXT("REMOTE parry finished blocks=%d dodges=%d counters=%d mode=%s"), RemoteParryBlocks, RemoteParryDodges, RemoteCounters, *RemoteMode);
			RemoteParryHandle.Reset();
			return false;
		}
		APlayerController* PC = World->GetFirstPlayerController();
		APawn* Me = PC ? PC->GetPawn() : nullptr;
		if (!Me)
		{
			return true;
		}
		UGOTABossComponent* Boss = nullptr;
		float BestDist = 2500.f;
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			if (UGOTABossComponent* Comp = It->FindComponentByClass<UGOTABossComponent>())
			{
				const float Dist = FVector::Dist2D(It->GetActorLocation(), Me->GetActorLocation());
				if (Dist < BestDist)
				{
					BestDist = Dist;
					Boss = Comp;
				}
			}
		}
		if (!Boss)
		{
			return true;
		}
		const double Now = World->GetTimeSeconds();
		float Lead = -1.f;
		bool bBlockable = false;
		bool bParryable = false;
		if (!bRemoteCounterDone && (RemoteMode == TEXT("both") || RemoteMode == TEXT("counter")))
		{
			if (UGOTADuelFighterComponent* Fighter = Me->FindComponentByClass<UGOTADuelFighterComponent>())
			{
				if (Fighter->CanDivineCounter() && Boss->IsOpen() && BestDist < 450.f)
				{
					Fighter->RequestDivineCounter();
					bRemoteCounterDone = true;
					++RemoteCounters;
					UE_LOG(LogGOTANet, Log, TEXT("REMOTE counter mom=%.0f dist=%.0f (legitimate local input)"), Fighter->GetMomentum(), BestDist);
				}
			}
		}
		const bool bIncoming = Boss->GetIncomingImpact(Lead, bBlockable, bParryable);
		const bool bWantDodge = RemoteMode == TEXT("dodge") || (RemoteMode == TEXT("both") && !bParryable);
		const bool bWantBlock = !bWantDodge && bBlockable && (RemoteMode == TEXT("parry") || RemoteMode == TEXT("both"));
		if (Now >= RemoteParryNext && bIncoming && Lead <= RemoteParryLead)
		{
			const FName AttackId = Boss->GetCurrentAttack() ? Boss->GetCurrentAttack()->AttackId : NAME_None;
			if (bWantBlock && BestDist < 700.f)
			{
				FGOTABlueprintCombat::Call(Me, TEXT("StartBlock"));
				RemoteParryNext = Now + 0.45;
				++RemoteParryBlocks;
				UE_LOG(LogGOTANet, Log, TEXT("REMOTE block lead=%.2f dist=%.0f attack=%s (legitimate local input)"), Lead, BestDist, *AttackId.ToString());
			}
			else if (bWantDodge && BestDist < 1300.f)
			{
				const FVector Away = (Me->GetActorLocation() - Boss->GetOwner()->GetActorLocation()).GetSafeNormal2D();
				const FVector Side = FVector::CrossProduct(Away, FVector::UpVector);
				const FVector Dest = Boss->IsCharging() ? Me->GetActorLocation() + Side * 500.f : Me->GetActorLocation() + Away * 380.f;
				CallVector(Me, TEXT("DodgeToLocation"), Dest);
				if (UGOTADuelFighterComponent* Fighter = Me->FindComponentByClass<UGOTADuelFighterComponent>())
				{
					Fighter->RequestDodgeNotify(); // BP IsInvincible is client-local; notify the server immediately
				}
				RemoteParryNext = Now + 0.45;
				++RemoteParryDodges;
				UE_LOG(LogGOTANet, Log, TEXT("REMOTE dodge lead=%.2f dist=%.0f attack=%s charging=%d (legitimate local input)"), Lead, BestDist, *AttackId.ToString(), Boss->IsCharging() ? 1 : 0);
			}
		}
		return true;
	}

	static void RemoteParry(const TArray<FString>& Args)
	{
		UWorld* World = GameWorld();
		if (!World)
		{
			return;
		}
		const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 25.f;
		if (Args.Num() > 1)
		{
			RemoteParryLead = FCString::Atof(*Args[1]);
		}
		RemoteParryUntil = World->GetTimeSeconds() + FMath::Max(Seconds, 1.f);
		RemoteParryBlocks = 0;
		RemoteParryDodges = 0;
		RemoteCounters = 0;
		bRemoteCounterDone = false;
		RemoteParryNext = 0.0;
		if (!RemoteParryHandle.IsValid())
		{
			RemoteParryHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickRemoteParry), 0.f);
		}
		UE_LOG(LogGOTANet, Log, TEXT("REMOTE parry armed for %.0fs lead=%.2f mode=%s on the local pawn"), Seconds, RemoteParryLead, *RemoteMode);
	}

	static void RemoteDefend(const TArray<FString>& Args)
	{
		if (Args.Num() > 0)
		{
			RemoteMode = Args[0].ToLower();
		}
		TArray<FString> Rest;
		if (Args.Num() > 1) { Rest.Add(Args[1]); }
		if (Args.Num() > 2) { Rest.Add(Args[2]); }
		RemoteParry(Rest);
	}

	static void Leave(const TArray<FString>&)
	{
		UWorld* World = GameWorld();
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		if (!World || !GI)
		{
			return;
		}
		if (UGotaSessionSubsystem* Sessions = GI->GetSubsystem<UGotaSessionSubsystem>())
		{
			Sessions->DestroyCurrentSession();
		}
		if (World->GetNetMode() != NM_Client)
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				if (APlayerController* PC = It->Get(); PC && !PC->IsLocalController())
				{
					PC->ClientReturnToMainMenuWithTextReason(NSLOCTEXT("GOTA", "HostLeft", "Host has left the game."));
				}
			}
		}
		UE_LOG(LogGOTANet, Log, TEXT("NET leave: clearing session and returning to the main menu"));
		TWeakObjectPtr<UGameInstance> WeakGI(GI);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, [WeakGI]()
		{
			if (UGameInstance* Live = WeakGI.Get())
			{
				Live->ReturnToMainMenu();
			}
		}, 0.5f, false);
	}

	static FTSTicker::FDelegateHandle PerfHandle;
	static double PerfUntil = 0.0;
	static int32 PerfSamples = 0;
	static int32 PerfHitch = 0;
	static float PerfFrameSum = 0.f;
	static float PerfFrameMax = 0.f;
	static float PerfGpuSum = 0.f;
	static float PerfGpuMax = 0.f;

	static void LogPerfSample(const TCHAR* Tag)
	{
		UWorld* World = GameWorld();
		if (!World)
		{
			return;
		}
		const FPlatformMemoryStats Mem = FPlatformMemory::GetStats();
		const float FrameMs = FApp::GetDeltaTime() * 1000.f;
		const float GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		int32 InBytes = 0;
		int32 OutBytes = 0;
		int32 InLost = 0;
		int32 OutLost = 0;
		auto AddConn = [&](const UNetConnection* Conn)
		{
			if (!Conn)
			{
				return;
			}
			InBytes += Conn->InTotalBytes;
			OutBytes += Conn->OutTotalBytes;
			InLost += Conn->InPacketsLost;
			OutLost += Conn->OutPacketsLost;
		};
		if (World->GetNetMode() == NM_Client)
		{
			if (const APlayerController* PC = World->GetFirstPlayerController())
			{
				AddConn(PC->NetConnection);
			}
		}
		else
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				if (const APlayerController* PC = It->Get())
				{
					AddConn(PC->NetConnection);
				}
			}
		}
		UE_LOG(LogGOTANet, Log, TEXT("%s frame=%.2fms gpu=%.2fms usedPhys=%.0fMB peakPhys=%.0fMB netIn=%dKB netOut=%dKB lostIn=%d lostOut=%d"),
			Tag, FrameMs, GpuMs, Mem.UsedPhysical / 1048576.0, Mem.PeakUsedPhysical / 1048576.0,
			InBytes / 1024, OutBytes / 1024, InLost, OutLost);
	}

	static bool TickPerf(float)
	{
		UWorld* World = GameWorld();
		const float FrameMs = FApp::GetDeltaTime() * 1000.f;
		const float GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		++PerfSamples;
		PerfFrameSum += FrameMs;
		PerfGpuSum += GpuMs;
		PerfFrameMax = FMath::Max(PerfFrameMax, FrameMs);
		PerfGpuMax = FMath::Max(PerfGpuMax, GpuMs);
		if (FrameMs >= 100.f)
		{
			++PerfHitch;
			UE_LOG(LogGOTANet, Warning, TEXT("PERF hitch frame=%.1fms gpu=%.1fms"), FrameMs, GpuMs);
		}
		if (!World || World->GetTimeSeconds() > PerfUntil)
		{
			UE_LOG(LogGOTANet, Log, TEXT("PERF summary samples=%d avgFrame=%.2fms maxFrame=%.2fms avgGpu=%.2fms maxGpu=%.2fms hitches=%d"),
				PerfSamples, PerfSamples ? PerfFrameSum / PerfSamples : 0.f, PerfFrameMax,
				PerfSamples ? PerfGpuSum / PerfSamples : 0.f, PerfGpuMax, PerfHitch);
			LogPerfSample(TEXT("PERF end"));
			PerfHandle.Reset();
			return false;
		}
		return true;
	}

	static void Perf(const TArray<FString>& Args)
	{
		UWorld* World = GameWorld();
		if (!World)
		{
			return;
		}
		const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.f;
		if (Seconds < 1.f)
		{
			LogPerfSample(TEXT("PERF"));
			return;
		}
		PerfUntil = World->GetTimeSeconds() + Seconds;
		PerfSamples = 0;
		PerfHitch = 0;
		PerfFrameSum = 0.f;
		PerfFrameMax = 0.f;
		PerfGpuSum = 0.f;
		PerfGpuMax = 0.f;
		if (!PerfHandle.IsValid())
		{
			PerfHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickPerf), 0.f);
		}
		UE_LOG(LogGOTANet, Log, TEXT("PERF sampling for %.0fs"), Seconds);
	}

	static APawn* PlayerPawn(int32 Index)
	{
		UWorld* W = GameWorld();
		const AGameStateBase* GS = W ? W->GetGameState() : nullptr;
		if (!GS || !GS->PlayerArray.IsValidIndex(Index) || !GS->PlayerArray[Index])
		{
			return nullptr;
		}
		return GS->PlayerArray[Index]->GetPawn();
	}

	/** Server: grant Divine Momentum to player <index> (Story/Horde/Duel PlayerArray order). */
	static void Momentum(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		const float Amount = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 50.f;
		if (!W || W->GetNetMode() == NM_Client)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET momentum: server only"));
			return;
		}
		APawn* Pawn = PlayerPawn(Index);
		UGOTADuelFighterComponent* F = Pawn ? Pawn->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr;
		if (!F)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET momentum: player %d has no fighter"), Index);
			return;
		}
		F->GrantMomentum(Amount, TEXT("net test grant"));
		UE_LOG(LogGOTANet, Log, TEXT("NET momentum [%d] +%.0f -> %.0f"), Index, Amount, F->GetMomentum());
	}

	/** Local or server-owned pawn: request Divine Counter for player <index> (must be the local controller's pawn on clients). */
	static void Counter(const TArray<FString>& Args)
	{
		const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : -1;
		UWorld* W = GameWorld();
		APawn* Pawn = nullptr;
		if (Index >= 0)
		{
			Pawn = PlayerPawn(Index);
		}
		else if (APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr)
		{
			Pawn = PC->GetPawn();
		}
		UGOTADuelFighterComponent* F = Pawn ? Pawn->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr;
		if (!F)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET counter: no fighter on target"));
			return;
		}
		F->RequestDivineCounter();
		UE_LOG(LogGOTANet, Log, TEXT("NET counter requested on %s mom=%.0f can=%d"), *GetNameSafe(Pawn), F->GetMomentum(), F->CanDivineCounter() ? 1 : 0);
	}

	/** Local: activate hero ability slot on the local pawn (or player <index> when server). */
	static void AbilityUse(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : -1;
		const int32 Slot = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
		APawn* Pawn = nullptr;
		if (Index >= 0)
		{
			Pawn = PlayerPawn(Index);
		}
		else if (APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr)
		{
			Pawn = PC->GetPawn();
		}
		UGOTAHeroAbilityComponent* A = Pawn ? Pawn->FindComponentByClass<UGOTAHeroAbilityComponent>() : nullptr;
		if (!A)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET ability: no abilities on %s"), *GetNameSafe(Pawn));
			return;
		}
		A->RequestActivate(Slot);
		UE_LOG(LogGOTANet, Log, TEXT("NET ability use on %s slot=%d"), *GetNameSafe(Pawn), Slot);
	}

	static double FightUntil = 0.0;
	static double FightNextAttack = 0.0;
	static double FightNextAbility = 0.0;
	static int32 FightAttacks = 0;
	static int32 FightAbilities = 0;
	static double FightStuckSince = 0.0;
	static double FightStrafeFlip = 0.0;
	static float FightBestDist = 0.f;
	static FVector FightStrafe = FVector::ZeroVector;
	static TWeakObjectPtr<APawn> FightStuckTarget;
	static FTSTicker::FDelegateHandle FightHandle;
	/** Targets the driver could not close on (ledges, no path): skipped until the time stored. */
	static TMap<TWeakObjectPtr<APawn>, double> FightSkipUntil;
	static int32 FightTargetSwitches = 0;
	/** Approach mode: walk into the fight but never attack (natural-defeat tests: only the enemies deal damage). */
	static bool bFightApproachOnly = false;
	static double FightLastBlock = -100.0;
	static int32 FightBlocks = 0;
	/** Dodge mode: answer incoming attacks with the hero's dodge (DodgeInDirection) instead of a block. */
	static bool bFightDodgeMode = false;
	static int32 FightDodges = 0;

	/** Like a player watching the enemy next to them: block when an enemy's attack montage is about to reach its damage
	 * notify. The montage replicates to every client; the Blueprint "Attacking" flag does not, so it is not used. */
	static bool TryFightBlock(UWorld* World, APawn* Me, double Now)
	{
		if (Now - FightLastBlock < 0.6)
		{
			return false;
		}
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Enemy = *It;
			if (Enemy == Me || Enemy->IsPlayerControlled() || UGOTACombatantObserver::IsCombatantDefeated(Enemy)
				|| FVector::Dist2D(Enemy->GetActorLocation(), Me->GetActorLocation()) > 400.f)
			{
				continue;
			}
			float Pos = 0.f;
			const UAnimMontage* M = FGOTABlueprintCombat::GetActiveMontage(Enemy, &Pos);
			const float Impact = FGOTABlueprintCombat::FindNextImpactTime(M, Pos);
			const float Lead = (M && Impact >= 0.f) ? Impact - Pos : -1.f;
			// The ground smash cannot be blocked or parried: like a player, dodge it in normal mode too.
			const bool bUnblockable = M && M->GetName().Contains(TEXT("GroundSmash"));
			if (bFightDodgeMode || bUnblockable)
			{
				// Dodge mode: roll away from any attack (ground smash included) just before its damage notify.
				if (Lead < 0.f || Lead > 0.3f || Now - FightLastBlock < 0.9)
				{
					continue;
				}
				FRotator Away = (Me->GetActorLocation() - Enemy->GetActorLocation()).Rotation();
				Away.Pitch = 0.f;
				if (APlayerController* PC = Cast<APlayerController>(Me->GetController()))
				{
					PC->SetControlRotation(Away);
				}
				Me->SetActorRotation(Away);
				FGOTABlueprintCombat::Call(Me, TEXT("DodgeInDirection"), nullptr, FString(), 300.0);
				FightLastBlock = Now;
				++FightDodges;
				UE_LOG(LogGOTANet, Log, TEXT("FIGHT dodge %s lead=%.2f %s on %s"), *GetNameSafe(Enemy), Lead, *GetNameSafe(M), *GetNameSafe(Me));
				return true;
			}
			if (Lead < 0.f || Lead > 0.35f || M->GetName().Contains(TEXT("GroundSmash")))
			{
				continue;
			}
			FRotator Face = (Enemy->GetActorLocation() - Me->GetActorLocation()).Rotation();
			Face.Pitch = 0.f;
			if (APlayerController* PC = Cast<APlayerController>(Me->GetController()))
			{
				PC->SetControlRotation(Face);
			}
			FGOTABlueprintCombat::Call(Me, TEXT("StartBlock"));
			FightLastBlock = Now;
			++FightBlocks;
			UE_LOG(LogGOTANet, Log, TEXT("FIGHT block %s lead=%.2f %s on %s"), *GetNameSafe(Enemy), Lead, *GetNameSafe(M), *GetNameSafe(Me));
			return true;
		}
		return false;
	}
	static int32 FightEvades = 0;
	static double FightEvadeUntil = 0.0;
	static FVector FightEvadeDir = FVector::ZeroVector;

	/** Living non-player combatant within 900 cm with the lowest health fraction (replicated vitals), or null. */
	static APawn* FindFocusTarget(UWorld* World, const APawn* Me, double Now)
	{
		APawn* Best = nullptr;
		float BestScore = TNumericLimits<float>::Max();
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* P = *It;
			if (P == Me || P->IsPlayerControlled() || !UGOTACombatantObserver::FindDamageSystem(P) || UGOTACombatantObserver::IsCombatantDefeated(P))
			{
				continue;
			}
			if (const double* Until = FightSkipUntil.Find(P); Until && *Until > Now)
			{
				continue;
			}
			const float Dist = FVector::Dist2D(P->GetActorLocation(), Me->GetActorLocation());
			if (Dist > 900.f)
			{
				continue;
			}
			float Fraction = 1.f;
			if (const UStatsComponent* Stats = P->FindComponentByClass<UStatsComponent>())
			{
				Fraction = Stats->GetMaxHealth() > 0.f ? Stats->GetHealth() / Stats->GetMaxHealth() : 1.f;
			}
			const float Score = Fraction * 1000.f + Dist * 0.2f; // damaged first, then near
			if (Score < BestScore)
			{
				BestScore = Score;
				Best = P;
			}
		}
		return Best;
	}

	/** Move straight away from an enemy within 450 cm whose spin or ground smash is winding up or running. */
	static bool TryFightEvade(UWorld* World, APawn* Me, APlayerController* PC, double Now)
	{
		if (Now < FightEvadeUntil)
		{
			Me->AddMovementInput(FightEvadeDir, 1.f);
			return true;
		}
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Enemy = *It;
			if (Enemy == Me || Enemy->IsPlayerControlled() || UGOTACombatantObserver::IsCombatantDefeated(Enemy)
				|| FVector::Dist2D(Enemy->GetActorLocation(), Me->GetActorLocation()) > 450.f)
			{
				continue;
			}
			const UAnimMontage* M = FGOTABlueprintCombat::GetActiveMontage(Enemy);
			if (M && (M->GetName().Contains(TEXT("Spinning")) || M->GetName().Contains(TEXT("GroundSmash"))))
			{
				FightEvadeDir = (Me->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
				FightEvadeUntil = Now + 1.6;
				++FightEvades;
				Me->AddMovementInput(FightEvadeDir, 1.f);
				UE_LOG(LogGOTANet, Log, TEXT("FIGHT evade %s from %s on %s"), *GetNameSafe(M), *GetNameSafe(Enemy), *GetNameSafe(Me));
				return true;
			}
		}
		return false;
	}

	/** Every living non-player pawn within 2500 cm and the montage this process sees it playing (client visibility). */
	static void Montages(const TArray<FString>&)
	{
		UWorld* W = GameWorld();
		const APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
		const APawn* Me = PC ? PC->GetPawn() : nullptr;
		if (!Me)
		{
			return;
		}
		int32 Seen = 0, Playing = 0;
		FString List;
		for (TActorIterator<APawn> It(W); It; ++It)
		{
			if (*It == Me || It->IsPlayerControlled() || UGOTACombatantObserver::IsCombatantDefeated(*It)
				|| FVector::Dist(It->GetActorLocation(), Me->GetActorLocation()) > 2500.f)
			{
				continue;
			}
			++Seen;
			const UAnimMontage* M = FGOTABlueprintCombat::GetActiveMontage(*It);
			Playing += M ? 1 : 0;
			List += FString::Printf(TEXT("%s:%s "), *It->GetName(), M ? *M->GetName() : TEXT("-"));
		}
		UE_LOG(LogGOTANet, Log, TEXT("MONTAGES role=%s enemies=%d playing=%d %s"), W->GetNetMode() == NM_Client ? TEXT("client") : TEXT("server"), Seen, Playing, *List.Left(600));
	}
	static FAutoConsoleCommand CmdMontages(TEXT("gota.Net.Montages"), TEXT("Log the montage this process sees on each nearby enemy (client attack readability)."), FConsoleCommandWithArgsDelegate::CreateStatic(&Montages));

	/** Provoke mode: one melee swing per enemy (Horde enemies only fight back once hurt), then no more attacks. */
	static bool bFightProvokeOnce = false;
	static TSet<TWeakObjectPtr<APawn>> FightProvoked;

	/** Nearest living non-player combatant that is not being skipped. */
	static APawn* FindFightTarget(UWorld* World, const APawn* Me, double Now)
	{
		APawn* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* P = *It;
			if (P == Me || P->IsPlayerControlled() || !UGOTACombatantObserver::FindDamageSystem(P) || UGOTACombatantObserver::IsCombatantDefeated(P))
			{
				continue;
			}
			if (const double* Until = FightSkipUntil.Find(P); Until && *Until > Now)
			{
				continue;
			}
			const float D = FVector::DistSquared(P->GetActorLocation(), Me->GetActorLocation());
			if (D < BestDist)
			{
				BestDist = D;
				Best = P;
			}
		}
		return Best;
	}

	/** Local pawn only. Walks at the current combat target, uses MeleeAttack and RequestActivate. Not a kill injector. */
	static bool TickFight(float)
	{
		UWorld* World = GameWorld();
		if (!World || World->GetTimeSeconds() > FightUntil)
		{
			UE_LOG(LogGOTANet, Log, TEXT("FIGHT finished attacks=%d abilities=%d blocks=%d dodges=%d evades=%d targetSwitches=%d"), FightAttacks, FightAbilities, FightBlocks, FightDodges, FightEvades, FightTargetSwitches);
			FightHandle.Reset();
			return false;
		}
		APlayerController* PC = World->GetFirstPlayerController();
		APawn* Me = PC ? PC->GetPawn() : nullptr;
		if (!Me || UGOTACombatantObserver::IsCombatantDefeated(Me) || !GOTACombatLive::IsCombatLive(World))
		{
			return true;
		}
		const double Now = World->GetTimeSeconds();
		APawn* Target = nullptr;
		if (const UGOTADuelFighterComponent* Fighter = Me->FindComponentByClass<UGOTADuelFighterComponent>())
		{
			Target = Fighter->GetOpponent();
		}
		// Horde: the fighter's "opponent" is whichever enemy spawned last, often 12 m away while others hit us. Like a
		// player, fight what is close: focus the most damaged enemy within 9 m (teammates converge on the same one).
		if (APawn* Focus = FindFocusTarget(World, Me, Now))
		{
			Target = Focus;
		}
		// Like a player reading a wind-up: step out of a spin or ground smash that is about to land.
		if (!bFightApproachOnly && !bFightDodgeMode && TryFightEvade(World, Me, PC, Now))
		{
			return true;
		}
		const double* Skip = Target ? FightSkipUntil.Find(Target) : nullptr;
		if (!IsValid(Target) || UGOTACombatantObserver::IsCombatantDefeated(Target) || (Skip && *Skip > Now))
		{
			// Horde has many enemies: walk to the nearest one this driver can actually reach. A duel has one opponent,
			// which is never skipped for long (it is also the only candidate).
			Target = FindFightTarget(World, Me, Now);
			if (!Target)
			{
				Target = GOTACombatLive::FindCombatTarget(Me);
			}
		}
		if (!Target)
		{
			return true;
		}
		if (!bFightApproachOnly && TryFightBlock(World, Me, Now))
		{
			return true; // defending this frame: no swing, no move
		}
		const FVector To = Target->GetActorLocation() - Me->GetActorLocation();
		const float Dist = To.Size2D();
		if (FightStuckTarget.Get() != Target)
		{
			FightStuckTarget = Target;
			FightBestDist = Dist;
			FightStuckSince = Now;
			FightStrafe = FVector::ZeroVector;
		}
		else if (Dist < FightBestDist - 20.f)
		{
			FightBestDist = Dist;
			FightStuckSince = Now;
			FightStrafe = FVector::ZeroVector;
		}
		// No closer for 6 s (pinned on geometry, target on a ledge): leave it for 12 s and take the next nearest.
		if (Now - FightStuckSince > 6.0 && Dist > 110.f)
		{
			FightSkipUntil.Add(Target, Now + 12.0);
			++FightTargetSwitches;
			UE_LOG(LogGOTANet, Log, TEXT("FIGHT skip %s dist=%.0f vert=%.0f for 12s on %s"), *GetNameSafe(Target), Dist, To.Z, *GetNameSafe(Me));
			FightStuckTarget = nullptr;
			return true;
		}
		FVector Move = To.GetSafeNormal2D();
		// Straight-line input pins the pawn on arena geometry (rc6fight3 held 135 cm with los=0). Strafe until the gap closes.
		if (Now - FightStuckSince > 1.0 && Dist > 110.f)
		{
			if (FightStrafe.IsNearlyZero() || Now >= FightStrafeFlip)
			{
				const FVector Side = FVector::CrossProduct(Move, FVector::UpVector);
				FightStrafe = FightStrafe.IsNearlyZero() || FVector::DotProduct(FightStrafe, Side) > 0.f ? -Side : Side;
				FightStrafeFlip = Now + 2.5;
				UE_LOG(LogGOTANet, Log, TEXT("FIGHT strafe dist=%.0f on %s"), Dist, *GetNameSafe(Me));
			}
			Move = (Move + FightStrafe * 1.4f).GetSafeNormal();
		}
		Me->AddMovementInput(Move, 1.f);
		FRotator Face = To.Rotation();
		Face.Pitch = 0.f;
		PC->SetControlRotation(Face);
		const bool bMaySwing = !bFightApproachOnly && (!bFightProvokeOnce || !FightProvoked.Contains(Target));
		if (bMaySwing && Dist < 260.f && Now >= FightNextAttack)
		{
			if (bFightProvokeOnce)
			{
				FightProvoked.Add(Target);
				// The Blueprint swing misses enemies standing on the arena step; the hero's first ability reaches them.
				if (UGOTAHeroAbilityComponent* Abilities = Me->FindComponentByClass<UGOTAHeroAbilityComponent>(); Abilities && Abilities->GetNumAbilities() > 0)
				{
					Abilities->RequestActivate(0);
				}
				UE_LOG(LogGOTANet, Log, TEXT("FIGHT provoke %s dist=%.0f on %s"), *GetNameSafe(Target), Dist, *GetNameSafe(Me));
			}
			FGOTABlueprintCombat::Call(Me, TEXT("MeleeAttack"));
			FightNextAttack = Now + 0.85;
			++FightAttacks;
		}
		if (!bFightApproachOnly && !bFightProvokeOnce && Dist < 500.f && Now >= FightNextAbility)
		{
			if (UGOTAHeroAbilityComponent* Abilities = Me->FindComponentByClass<UGOTAHeroAbilityComponent>())
			{
				const int32 Count = Abilities->GetNumAbilities();
				if (Count > 0)
				{
					const int32 Slot = FightAbilities % Count;
					Abilities->RequestActivate(Slot);
					++FightAbilities;
					UE_LOG(LogGOTANet, Log, TEXT("FIGHT ability slot=%d dist=%.0f on %s"), Slot, Dist, *GetNameSafe(Me));
				}
			}
			FightNextAbility = Now + 3.0;
		}
		return true;
	}

	static void Fight(const TArray<FString>& Args)
	{
		UWorld* World = GameWorld();
		if (!World)
		{
			return;
		}
		const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 90.f;
		FightUntil = World->GetTimeSeconds() + FMath::Max(Seconds, 1.f);
		FightNextAttack = 0.0;
		FightNextAbility = 0.0;
		FightAttacks = 0;
		FightAbilities = 0;
		FightStuckSince = 0.0;
		FightStrafeFlip = 0.0;
		FightBestDist = 0.f;
		FightStrafe = FVector::ZeroVector;
		FightStuckTarget = nullptr;
		FightSkipUntil.Reset();
		FightTargetSwitches = 0;
		FightBlocks = 0;
		FightEvades = 0;
		FightEvadeUntil = 0.0;
		FightLastBlock = -100.0;
		bFightApproachOnly = Args.Num() > 1 && Args[1].Equals(TEXT("approach"), ESearchCase::IgnoreCase);
		bFightProvokeOnce = Args.Num() > 1 && Args[1].Equals(TEXT("provoke"), ESearchCase::IgnoreCase);
		bFightDodgeMode = Args.Num() > 1 && Args[1].Equals(TEXT("dodge"), ESearchCase::IgnoreCase);
		FightDodges = 0;
		FightProvoked.Reset();
		if (!FightHandle.IsValid())
		{
			FightHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickFight), 0.f);
		}
		UE_LOG(LogGOTANet, Log, TEXT("FIGHT armed %.0fs %s"), Seconds, bFightApproachOnly ? TEXT("(approach only: walks into the fight, never attacks)")
			: bFightProvokeOnce ? TEXT("(provoke: one melee swing and one first-slot ability per enemy, then nothing)") : TEXT("(local MeleeAttack + RequestActivate, no injected kills)"));
	}

	static double WatchLimit = 15.0;
	static double WatchCloseSince = -1.0;
	static int32 WatchLanded = -1;
	static int32 WatchDefeated = -1;
	static bool bWatchReported = false;
	static FTSTicker::FDelegateHandle WatchHandle;

	static bool TickWatch(float)
	{
		UWorld* World = GameWorld();
		if (!World || World->GetNetMode() == NM_Client)
		{
			return true;
		}
		int32 Defeated = 0;
		for (TActorIterator<AHordeEncounterDirector> It(World); It; ++It)
		{
			Defeated = It->GetDefeatedCount();
			break;
		}
		const FGOTAAbilityResolveStats Stats = GOTAGetAbilityResolveStats();
		APawn* NearEnemy = nullptr;
		APawn* NearPlayer = nullptr;
		float BestHoriz = TNumericLimits<float>::Max();
		float Vertical = 0.f;
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Enemy = *It;
			if (!Enemy || Enemy->IsPlayerControlled() || !UGOTACombatantObserver::FindDamageSystem(Enemy)
				|| UGOTACombatantObserver::IsCombatantDefeated(Enemy))
			{
				continue;
			}
			for (FConstPlayerControllerIterator PCIt = World->GetPlayerControllerIterator(); PCIt; ++PCIt)
			{
				APawn* Player = PCIt->Get() ? PCIt->Get()->GetPawn() : nullptr;
				if (!Player)
				{
					continue;
				}
				const FVector Delta = Enemy->GetActorLocation() - Player->GetActorLocation();
				const float Horiz = FVector(Delta.X, Delta.Y, 0.f).Size();
				if (Horiz < BestHoriz)
				{
					BestHoriz = Horiz;
					Vertical = Delta.Z;
					NearEnemy = Enemy;
					NearPlayer = Player;
				}
			}
		}
		const bool bClose = NearEnemy && NearPlayer && BestHoriz <= 500.f;
		if (!bClose)
		{
			WatchCloseSince = -1.0;
			return true;
		}
		const double Now = World->GetTimeSeconds();
		if (WatchLanded < 0 || Stats.Landed != WatchLanded || Defeated != WatchDefeated)
		{
			WatchLanded = Stats.Landed;
			WatchDefeated = Defeated;
			WatchCloseSince = Now;
			return true;
		}
		if (WatchCloseSince < 0.0)
		{
			WatchCloseSince = Now;
		}
		if (!bWatchReported && Now - WatchCloseSince >= WatchLimit)
		{
			bWatchReported = true;
			bool bLos = false;
			if (NearPlayer && NearEnemy)
			{
				FCollisionQueryParams Params(SCENE_QUERY_STAT(GOTAWatchLOS), false, NearPlayer);
				Params.AddIgnoredActor(NearEnemy);
				bLos = !World->LineTraceTestByChannel(
					NearPlayer->GetActorLocation() + FVector(0, 0, 50),
					NearEnemy->GetActorLocation() + FVector(0, 0, 50),
					ECC_WorldStatic, Params);
			}
			UE_LOG(LogGOTANet, Warning, TEXT("HORDE STALL no damage for %.0fs nearest=%s horiz=%.0f vert=%.0f los=%d defeated=%d abilityHits=%d abilityMisses=%d"),
				WatchLimit, *GetNameSafe(NearEnemy), BestHoriz, Vertical, bLos ? 1 : 0, Defeated, Stats.Landed, Stats.Missed);
			WatchHandle.Reset();
			return false;
		}
		return true;
	}

	static void Watch(const TArray<FString>& Args)
	{
		UWorld* World = GameWorld();
		if (!World || World->GetNetMode() == NM_Client)
		{
			return;
		}
		WatchLimit = Args.Num() > 0 ? FMath::Max(5.f, FCString::Atof(*Args[0])) : 15.f;
		WatchCloseSince = -1.0;
		WatchLanded = -1;
		WatchDefeated = -1;
		bWatchReported = false;
		if (!WatchHandle.IsValid())
		{
			WatchHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickWatch), 0.25f);
		}
		UE_LOG(LogGOTANet, Log, TEXT("HORDE WATCH armed %.0fs (stall if a player stays within 500cm and nothing lands)"), WatchLimit);
	}

	static FAutoConsoleCommand CmdFight(TEXT("gota.Net.Fight"), TEXT("Local pawn fights the current target with MeleeAttack and real ability slots. Args: seconds [approach = walk in, never attack | provoke = one swing per enemy, no abilities]."), FConsoleCommandWithArgsDelegate::CreateStatic(&Fight));
	static FAutoConsoleCommand CmdWatch(TEXT("gota.Horde.Watch"), TEXT("Server: log HORDE STALL if players stay within 500cm of a living enemy and neither kills nor ability hits advance. Args: seconds (min 5)."), FConsoleCommandWithArgsDelegate::CreateStatic(&Watch));
	static FAutoConsoleCommand CmdPlayers(TEXT("gota.Net.Players"), TEXT("Log every player's role, pawn, health and mode state."), FConsoleCommandWithArgsDelegate::CreateStatic(&Players));
	static FAutoConsoleCommand CmdHurt(TEXT("gota.Net.Hurt"), TEXT("Server: damage player <index> <amount> through its own damage system."), FConsoleCommandWithArgsDelegate::CreateStatic(&Hurt));
	static FAutoConsoleCommand CmdKillEnemy(TEXT("gota.Net.KillEnemy"), TEXT("Server: player <index> defeats the nearest living enemy through its damage system (kill-credit test). Optional: boss | adds."), FConsoleCommandWithArgsDelegate::CreateStatic(&KillEnemy));
	static FAutoConsoleCommand CmdForge(TEXT("gota.Net.ForgeParry"), TEXT("Exploit test (client): forged SR_ParryAttack at the farthest pawn; the server must reject it."), FConsoleCommandWithArgsDelegate::CreateStatic(&ForgeParry));
	static FAutoConsoleCommand CmdRemote(TEXT("gota.Net.RemoteParry"), TEXT("Client: time StartBlock on the local pawn against the nearest boss. Args: seconds lead."), FConsoleCommandWithArgsDelegate::CreateStatic(&RemoteParry));
	static FAutoConsoleCommand CmdDefend(TEXT("gota.Net.RemoteDefend"), TEXT("Client: parry, dodge, or both against the nearest boss. Args: parry|dodge|both [seconds] [lead]."), FConsoleCommandWithArgsDelegate::CreateStatic(&RemoteDefend));
	/** pN = player N's pawn, eN = the living enemy nearest player N, b = the first living boss. */
	static APawn* ResolveTarget(const FString& Arg)
	{
		UWorld* W = GameWorld();
		if (!W || Arg.IsEmpty())
		{
			return nullptr;
		}
		if (Arg == TEXT("b"))
		{
			for (TActorIterator<APawn> It(W); It; ++It)
			{
				if (It->FindComponentByClass<UGOTABossComponent>() && !UGOTACombatantObserver::IsCombatantDefeated(*It))
				{
					return *It;
				}
			}
			return nullptr;
		}
		APawn* Player = PlayerPawn(FCString::Atoi(*Arg.Mid(1)));
		return Arg[0] == TEXT('e') ? GOTACombatLive::FindCombatTarget(Player) : Player;
	}

	/**
	 * gota.Net.Policy <pN|eN|b> <Slow,Stun,Burn|none> [Status durationScale magnitudeScale]: server test. Replaces the
	 * target's status policy: the listed GOTA.Immunity.* aspects, plus one optional per-status resistance.
	 */
	static void NetPolicy(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		APawn* Target = Args.Num() >= 2 && W && W->GetNetMode() != NM_Client ? ResolveTarget(Args[0]) : nullptr;
		UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::EnsureOnPawn(Target);
		if (!ASC)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET policy: server only, args <pN|eN|b> <Slow,Stun,Burn|none> [Status durScale magScale]"));
			return;
		}
		FGOTAStatusPolicy Policy;
		TArray<FString> Names;
		Args[1].ParseIntoArray(Names, TEXT(","));
		for (const FString& Name : Names)
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*(TEXT("GOTA.Immunity.") + Name)), false);
			if (Tag.IsValid())
			{
				Policy.Immunities.AddTag(Tag);
			}
		}
		if (Args.Num() >= 5)
		{
			const int64 Value = StaticEnum<EGOTAStatus>()->GetValueByNameString(Args[2]);
			if (Value != INDEX_NONE)
			{
				FGOTAStatusResistance Rule;
				Rule.Status = static_cast<EGOTAStatus>(Value);
				Rule.DurationScale = FCString::Atof(*Args[3]);
				Rule.MagnitudeScale = FCString::Atof(*Args[4]);
				Policy.Resistances.Add(Rule);
			}
		}
		ASC->ApplyStatusPolicy(Policy, TEXT("test"));
		UE_LOG(LogGOTANet, Log, TEXT("NET policy on %s: immune=[%s] resistances=%d"), *GetNameSafe(Target), *ASC->DescribeImmunities(), Policy.Resistances.Num());
	}

	/**
	 * gota.Net.GAS: any machine. One line of ability-system totals for this process: ability system components, active
	 * Burning statuses (by tag, so it also counts on clients), live flame effects, and the server's status-damage and
	 * ability-hit counters. For Horde performance captures and cleanup checks.
	 */
	static void GasReport(const TArray<FString>&)
	{
		UWorld* W = GameWorld();
		if (!W)
		{
			return;
		}
		int32 ASCs = 0, Burning = 0, Statuses = 0, Flames = 0;
		for (TActorIterator<APawn> It(W); It; ++It)
		{
			if (const UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(*It))
			{
				++ASCs;
				Burning += ASC->HasMatchingGameplayTag(GOTATags::Status_Burning) ? 1 : 0;
				Statuses += ASC->HasMatchingGameplayTag(GOTATags::Status) ? 1 : 0;
			}
			TInlineComponentArray<UParticleSystemComponent*> Emitters(*It);
			for (const UParticleSystemComponent* PSC : Emitters)
			{
				Flames += PSC && PSC->ComponentHasTag(TEXT("GOTA_BurnFX")) ? 1 : 0;
			}
		}
		const GOTAGAS::FStatusDamageStats Dot = GOTAGAS::GetStatusDamageStats();
		const FGOTAAbilityResolveStats Hits = GOTAGetAbilityResolveStats();
		UE_LOG(LogGOTANet, Log, TEXT("GAS %s ascs=%d withStatus=%d burning=%d flames=%d burnTicks=%d burnDamage=%.1f abilityHits=%d abilityMisses=%d"),
			NetModeName(W), ASCs, Statuses, Burning, Flames, Dot.Ticks, Dot.Damage, Hits.Landed, Hits.Missed);
	}

	/**
	 * gota.Net.Status <pN|eN|b> <Status> <seconds> <magnitude> [pM]: server, any mode. pN = player N's pawn, eN = the living
	 * enemy nearest player N, b = the first living boss. The optional pM makes player M the source (who gets resist
	 * feedback). Goes through UGOTAHeroAbilityComponent::ApplyStatus, the path every ability uses.
	 */
	static void NetStatus(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		if (!W || W->GetNetMode() == NM_Client || Args.Num() < 4 || (Args[0] != TEXT("b") && Args[0].Len() < 2))
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET status: server only, args <pN|eN> <Status> <seconds> <magnitude>"));
			return;
		}
		AActor* AllSource = Args.Num() > 4 && Args[4].StartsWith(TEXT("p")) ? PlayerPawn(FCString::Atoi(*Args[4].Mid(1))) : nullptr;
		const int64 AllValue = StaticEnum<EGOTAStatus>()->GetValueByNameString(Args[1]);
		if (Args[0] == TEXT("ea") && AllValue != INDEX_NONE)
		{
			// Every living non-player combatant at once (concurrent-status load for performance runs).
			int32 Count = 0;
			for (TActorIterator<APawn> It(W); It; ++It)
			{
				if (It->IsPlayerControlled() || !UGOTACombatantObserver::FindDamageSystem(*It) || UGOTACombatantObserver::IsCombatantDefeated(*It))
				{
					continue;
				}
				UGOTAHeroAbilityComponent::ApplyStatusTo(*It, static_cast<EGOTAStatus>(AllValue), FCString::Atof(*Args[2]), FCString::Atof(*Args[3]), AllSource);
				++Count;
			}
			UE_LOG(LogGOTANet, Log, TEXT("NET status %s on all %d living enemies from %s"), *Args[1], Count, *GetNameSafe(AllSource));
			return;
		}
		APawn* Target = ResolveTarget(Args[0]);
		AActor* Source = Args.Num() > 4 && Args[4].StartsWith(TEXT("p")) ? PlayerPawn(FCString::Atoi(*Args[4].Mid(1))) : nullptr;
		const int64 Value = StaticEnum<EGOTAStatus>()->GetValueByNameString(Args[1]);
		if (!Target || Value == INDEX_NONE)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET status: no target for %s or unknown status %s"), *Args[0], *Args[1]);
			return;
		}
		UGOTAHeroAbilityComponent::ApplyStatusTo(Target, static_cast<EGOTAStatus>(Value), FCString::Atof(*Args[2]), FCString::Atof(*Args[3]), Source);
		UE_LOG(LogGOTANet, Log, TEXT("NET status %s %s on %s from %s"), *Args[1], *Args[0], *GetNameSafe(Target), *GetNameSafe(Source));
	}
	/**
	 * gota.Net.SpawnEnemies <count> [radius=2200]: server stress test. Spawns plain BP_Enemy_Melee in a ring around player 0,
	 * outside the Horde wave director (waves, score and victory are not touched). For replication-cost measurements.
	 */
	static void SpawnEnemies(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		APawn* Anchor = PlayerPawn(0);
		UClass* EnemyClass = LoadClass<APawn>(nullptr, TEXT("/Game/Enemies/MeleeEnemy/BP_Enemy_Melee.BP_Enemy_Melee_C"));
		if (!W || W->GetNetMode() == NM_Client || !Anchor || !EnemyClass || Args.Num() < 1)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET spawn enemies: server only, needs <count>"));
			return;
		}
		const int32 Count = FMath::Clamp(FCString::Atoi(*Args[0]), 1, 64);
		const float Radius = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 2200.f;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		int32 Spawned = 0;
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector Dir = FRotator(0.f, 360.f * i / Count, 0.f).Vector();
			FVector Loc = Anchor->GetActorLocation() + Dir * Radius;
			Loc.Z = Anchor->GetActorLocation().Z;
			if (APawn* Enemy = W->SpawnActor<APawn>(EnemyClass, Loc, (-Dir).Rotation(), Params))
			{
				if (!Enemy->GetController())
				{
					Enemy->SpawnDefaultController();
				}
				++Spawned;
			}
		}
		UE_LOG(LogGOTANet, Log, TEXT("NET spawned %d/%d enemies at radius %.0f"), Spawned, Count, Radius);
	}
	/** Test harness only (never balance evidence): heal every living player through its own damage system's Heal once a
	 * second, so test bots survive long enough to exercise enemy pursuit, target changes, deaths and kill credit. Enemies
	 * are not touched. Args: <seconds> [hp per second = 15]; 0 stops. */
	static FTSTicker::FDelegateHandle SustainHandle;
	static double SustainUntil = 0.0;
	static float SustainRate = 15.f;
	static int32 SustainHeals = 0;
	static void Sustain(const TArray<FString>& Args)
	{
		const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 120.f;
		SustainRate = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 15.f;
		FTSTicker::GetCoreTicker().RemoveTicker(SustainHandle);
		SustainHandle.Reset();
		if (Seconds <= 0.f)
		{
			UE_LOG(LogGOTANet, Log, TEXT("NET sustain off (%d heals)"), SustainHeals);
			return;
		}
		SustainUntil = FPlatformTime::Seconds() + Seconds;
		SustainHeals = 0;
		SustainHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			UWorld* W = GameWorld();
			if (!W || FPlatformTime::Seconds() > SustainUntil)
			{
				UE_LOG(LogGOTANet, Log, TEXT("NET sustain ended (%d heals)"), SustainHeals);
				SustainHandle.Reset();
				return false;
			}
			if (W->GetNetMode() == NM_Client)
			{
				return true;
			}
			for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
			{
				APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
				const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Pawn);
				if (!Pawn || !V.bHasDamageSystem || V.bIsDead || V.Health <= 0.f || V.Health >= V.MaxHealth)
				{
					continue;
				}
				if (UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Pawn))
				{
					FGOTABlueprintCombat::Call(DS, TEXT("Heal"), nullptr, FString(), FMath::Min(SustainRate, V.MaxHealth - V.Health));
					++SustainHeals;
				}
			}
			return true;
		}), 1.f);
		UE_LOG(LogGOTANet, Log, TEXT("NET sustain %.0fs at %.0f hp/s on every living player (test harness, enemies unchanged)"), Seconds, SustainRate);
	}
	/** Server test: the living AI enemy nearest player <index> performs its Blueprint attack <Function> on that player
	 * (e.g. SpinningAttack, ShortRangeAttack, GroundSmashAttack). The enemy's own attack code, montage and damage run. */
	static void EnemyAttack(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		if (!W || W->GetNetMode() == NM_Client || Args.Num() < 1)
		{
			UE_LOG(LogGOTANet, Warning, TEXT("NET enemy attack: server only, args <Function> [playerIndex]"));
			return;
		}
		APawn* Player = PlayerPawn(Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0);
		APawn* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (TActorIterator<APawn> It(W); Player && It; ++It)
		{
			if (It->IsPlayerControlled() || !UGOTACombatantObserver::FindDamageSystem(*It) || UGOTACombatantObserver::IsCombatantDefeated(*It)
				|| !It->FindFunction(FName(*Args[0])))
			{
				continue;
			}
			const float D = FVector::Dist(It->GetActorLocation(), Player->GetActorLocation());
			if (D < BestDist)
			{
				BestDist = D;
				Best = *It;
			}
		}
		const bool bCalled = Best && FGOTABlueprintCombat::Call(Best, FName(*Args[0]), Player);
		UE_LOG(LogGOTANet, Log, TEXT("NET enemy attack %s by %s on %s dist=%.0f called=%d"), *Args[0], *GetNameSafe(Best), *GetNameSafe(Player),
			Best ? BestDist : -1.f, bCalled ? 1 : 0);
	}
	static FAutoConsoleCommand CmdEnemyAttack(TEXT("gota.Net.EnemyAttack"), TEXT("Server test: the enemy nearest player [index] performs Blueprint attack <Function> on that player."), FConsoleCommandWithArgsDelegate::CreateStatic(&EnemyAttack));

	/** Local test: the local pawn holds block (StartBlock every 0.3 s), walks away from the nearest enemy, or stops. */
	static FTSTicker::FDelegateHandle HoldHandle;
	static FString HoldMode;
	static double HoldUntil = 0.0;
	static double HoldNextBlock = 0.0;
	static void Hold(const TArray<FString>& Args)
	{
		HoldMode = Args.Num() > 0 ? Args[0].ToLower() : TEXT("stop");
		HoldUntil = FPlatformTime::Seconds() + (Args.Num() > 1 ? FCString::Atof(*Args[1]) : 10.0);
		FTSTicker::GetCoreTicker().RemoveTicker(HoldHandle);
		HoldHandle.Reset();
		UE_LOG(LogGOTANet, Log, TEXT("NET hold %s"), *HoldMode);
		if (HoldMode == TEXT("stop"))
		{
			return;
		}
		HoldHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			UWorld* W = GameWorld();
			APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
			APawn* Me = PC ? PC->GetPawn() : nullptr;
			if (!Me || FPlatformTime::Seconds() > HoldUntil)
			{
				if (Me && HoldMode == TEXT("block"))
				{
					FGOTABlueprintCombat::Call(Me, TEXT("StopBlock"));
				}
				UE_LOG(LogGOTANet, Log, TEXT("NET hold %s ended"), *HoldMode);
				HoldHandle.Reset();
				return false;
			}
			APawn* Enemy = nullptr;
			float BestDist = TNumericLimits<float>::Max();
			for (TActorIterator<APawn> It(W); It; ++It)
			{
				if (*It != Me && !It->IsPlayerControlled() && UGOTACombatantObserver::FindDamageSystem(*It) && !UGOTACombatantObserver::IsCombatantDefeated(*It))
				{
					const float D = FVector::Dist(It->GetActorLocation(), Me->GetActorLocation());
					if (D < BestDist)
					{
						BestDist = D;
						Enemy = *It;
					}
				}
			}
			if (HoldMode == TEXT("block"))
			{
				if (Enemy)
				{
					FRotator Face = (Enemy->GetActorLocation() - Me->GetActorLocation()).Rotation();
					Face.Pitch = 0.f;
					PC->SetControlRotation(Face);
				}
				if (FPlatformTime::Seconds() >= HoldNextBlock)
				{
					FGOTABlueprintCombat::Call(Me, TEXT("StartBlock"));
					HoldNextBlock = FPlatformTime::Seconds() + 0.3;
				}
			}
			else if (HoldMode == TEXT("away") && Enemy)
			{
				FVector Away = Me->GetActorLocation() - Enemy->GetActorLocation();
				Away.Z = 0.f;
				Me->AddMovementInput(Away.GetSafeNormal(), 1.f);
			}
			return true;
		}), 0.f);
	}
	static FAutoConsoleCommand CmdHold(TEXT("gota.Net.Hold"), TEXT("Local test: <block|away|stop> [seconds]: hold block facing the nearest enemy, or walk away from it."), FConsoleCommandWithArgsDelegate::CreateStatic(&Hold));
	static FAutoConsoleCommand CmdSustain(TEXT("gota.Net.Sustain"), TEXT("Server test harness: heal living players <hp/s> (default 15) for <seconds> through their own Heal; 0 stops. Not balance evidence."), FConsoleCommandWithArgsDelegate::CreateStatic(&Sustain));
	static FAutoConsoleCommand CmdSpawnEnemies(TEXT("gota.Net.SpawnEnemies"), TEXT("Server stress test: spawn <count> plain melee enemies around player 0 [radius]."), FConsoleCommandWithArgsDelegate::CreateStatic(&SpawnEnemies));
	static FAutoConsoleCommand CmdPolicy(TEXT("gota.Net.Policy"), TEXT("Server test: replace a status policy on <pN|eN|b>: <Slow,Stun,Burn|none> [Status durScale magScale]."), FConsoleCommandWithArgsDelegate::CreateStatic(&NetPolicy));
	static FAutoConsoleCommand CmdGas(TEXT("gota.Net.GAS"), TEXT("Log ability-system totals: components, Burning statuses, flame effects, burn ticks and damage."), FConsoleCommandWithArgsDelegate::CreateStatic(&GasReport));
	static FAutoConsoleCommand CmdStatus(TEXT("gota.Net.Status"), TEXT("Server: apply a status to <pN|eN|b|ea> (player N / enemy nearest player N / boss / every living enemy): <Status> <seconds> <magnitude> [source pM]."), FConsoleCommandWithArgsDelegate::CreateStatic(&NetStatus));
	static FAutoConsoleCommand CmdMomentum(TEXT("gota.Net.Momentum"), TEXT("Server: grant Divine Momentum to player <index> [amount=50]."), FConsoleCommandWithArgsDelegate::CreateStatic(&Momentum));
	static FAutoConsoleCommand CmdCounter(TEXT("gota.Net.Counter"), TEXT("Request Divine Counter on local pawn, or player <index>."), FConsoleCommandWithArgsDelegate::CreateStatic(&Counter));
	static FAutoConsoleCommand CmdAbility(TEXT("gota.Net.Ability"), TEXT("Activate ability slot on local pawn, or player <index> <slot>."), FConsoleCommandWithArgsDelegate::CreateStatic(&AbilityUse));
	static FAutoConsoleCommand CmdLeave(TEXT("gota.Net.Leave"), TEXT("Clear the session and return this process to the main menu. A listen host tells clients to return first."), FConsoleCommandWithArgsDelegate::CreateStatic(&Leave));
	static FAutoConsoleCommand CmdPerf(TEXT("gota.Net.Perf"), TEXT("Log frame, GPU, memory, and net totals. Optional seconds samples a window and counts hitches over 100ms."), FConsoleCommandWithArgsDelegate::CreateStatic(&Perf));
	static FAutoConsoleCommand CmdAudit(TEXT("gota.Net.AuditRPCs"), TEXT("List loaded game classes' network RPCs and replicated properties."), FConsoleCommandWithArgsDelegate::CreateStatic(&AuditRPCs));
	static FAutoConsoleCommand CmdInspect(TEXT("gota.Net.InspectDamage"), TEXT("Damage-system props/delegates of the first pawn whose class contains <substring>."), FConsoleCommandWithArgsDelegate::CreateStatic(&InspectDamage));
}

#endif // !UE_BUILD_SHIPPING
