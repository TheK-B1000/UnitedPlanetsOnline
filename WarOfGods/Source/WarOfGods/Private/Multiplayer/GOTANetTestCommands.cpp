// White Tiger Studios 2024
//
// Multiplayer diagnostics and test hooks (every mode: Duel, Horde, Story). Development / editor builds only.
//   gota.Net.Players            - one line per player: role, pawn, health, mode state, ping (any machine)
//   gota.Net.Hurt <idx> <amt>   - server: damage player <idx> (GameState PlayerArray order) via its own BP damage system
//   gota.Net.AuditRPCs          - list every loaded game class's network RPCs and replicated property counts
//   gota.Net.InspectDamage <s>  - damage-system properties, functions and delegates of the first actor whose class contains <s>

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Arenas/Duel/DuelPlayerState.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Arenas/Horde/HordePlayerState.h"
#include "Characters/GOTAPlayerCharacter.h"
#include "Combat/GOTACombatantObserver.h"
#include "Bosses/GOTABossComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
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
			if (const AGOTAPlayerCharacter* Guarded = Cast<AGOTAPlayerCharacter>(Pawn))
			{
				Mode += FString::Printf(TEXT(" rejectedRPCs=%d"), Guarded->GetRejectedRPCs());
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
		APawn* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (TActorIterator<APawn> It(W); It; ++It)
		{
			if (It->IsPlayerControlled() || !UGOTACombatantObserver::FindDamageSystem(*It) || UGOTACombatantObserver::IsCombatantDefeated(*It))
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
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(Fn->ParmsSize));
		FMemory::Memzero(Params, Fn->ParmsSize);
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
			UE_LOG(LogGOTANet, Log, TEXT("INSPECT  prop %s : %s%s"), *It->GetName(), *It->GetCPPType(), It->HasAnyPropertyFlags(CPF_Net) ? TEXT(" [replicated]") : TEXT(""));
		}
	}

	static FTSTicker::FDelegateHandle RemoteParryHandle;
	static double RemoteParryUntil = 0.0;
	static double RemoteParryNext = 0.0;
	static float RemoteParryLead = 0.35f;
	static int32 RemoteParryBlocks = 0;

	static bool TickRemoteParry(float)
	{
		UWorld* World = GameWorld();
		if (!World || World->GetTimeSeconds() > RemoteParryUntil)
		{
			UE_LOG(LogGOTANet, Log, TEXT("REMOTE parry finished blocks=%d"), RemoteParryBlocks);
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
		if (Now >= RemoteParryNext && Boss->GetIncomingImpact(Lead, bBlockable, bParryable)
			&& bParryable && Lead <= RemoteParryLead && BestDist < 700.f)
		{
			FGOTABlueprintCombat::Call(Me, TEXT("StartBlock"));
			RemoteParryNext = Now + 0.45;
			++RemoteParryBlocks;
			UE_LOG(LogGOTANet, Log, TEXT("REMOTE block lead=%.2f dist=%.0f (legitimate local input)"), Lead, BestDist);
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
		RemoteParryNext = 0.0;
		if (!RemoteParryHandle.IsValid())
		{
			RemoteParryHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickRemoteParry), 0.f);
		}
		UE_LOG(LogGOTANet, Log, TEXT("REMOTE parry armed for %.0fs lead=%.2f on the local pawn"), Seconds, RemoteParryLead);
	}

	static FAutoConsoleCommand CmdPlayers(TEXT("gota.Net.Players"), TEXT("Log every player's role, pawn, health and mode state."), FConsoleCommandWithArgsDelegate::CreateStatic(&Players));
	static FAutoConsoleCommand CmdHurt(TEXT("gota.Net.Hurt"), TEXT("Server: damage player <index> <amount> through its own damage system."), FConsoleCommandWithArgsDelegate::CreateStatic(&Hurt));
	static FAutoConsoleCommand CmdKillEnemy(TEXT("gota.Net.KillEnemy"), TEXT("Server: player <index> defeats the nearest living enemy (kill-credit test)."), FConsoleCommandWithArgsDelegate::CreateStatic(&KillEnemy));
	static FAutoConsoleCommand CmdForge(TEXT("gota.Net.ForgeParry"), TEXT("Exploit test (client): forged SR_ParryAttack at the farthest pawn; the server must reject it."), FConsoleCommandWithArgsDelegate::CreateStatic(&ForgeParry));
	static FAutoConsoleCommand CmdRemote(TEXT("gota.Net.RemoteParry"), TEXT("Client: time StartBlock on the local pawn against the nearest boss. Args: seconds lead."), FConsoleCommandWithArgsDelegate::CreateStatic(&RemoteParry));
	static FAutoConsoleCommand CmdAudit(TEXT("gota.Net.AuditRPCs"), TEXT("List loaded game classes' network RPCs and replicated properties."), FConsoleCommandWithArgsDelegate::CreateStatic(&AuditRPCs));
	static FAutoConsoleCommand CmdInspect(TEXT("gota.Net.InspectDamage"), TEXT("Damage-system props/delegates of the first pawn whose class contains <substring>."), FConsoleCommandWithArgsDelegate::CreateStatic(&InspectDamage));
}

#endif // !UE_BUILD_SHIPPING
