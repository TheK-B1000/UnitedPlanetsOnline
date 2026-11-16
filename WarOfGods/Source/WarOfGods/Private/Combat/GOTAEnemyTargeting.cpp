// White Tiger Studios 2024

#include "Combat/GOTAEnemyTargeting.h"

#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "Combat/GOTACombatantObserver.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAEnemyTargeting)

DEFINE_LOG_CATEGORY_STATIC(LogGOTATargeting, Log, All);

namespace
{
	const FName SetAttackingFn(TEXT("SetStateAsAttacking"));
	const FName SetSeekingFn(TEXT("SetStateAsSeeking"));

	FString Label(const APawn* Pawn)
	{
		if (!Pawn)
		{
			return TEXT("none");
		}
		const APlayerState* PS = Pawn->GetPlayerState();
		return PS ? PS->GetPlayerName() : Pawn->GetName();
	}

	FName ReadNameVar(const UObject* Obj, const TCHAR* Var)
	{
		const FNameProperty* P = Obj ? FindFProperty<FNameProperty>(Obj->GetClass(), Var) : nullptr;
		return P ? P->GetPropertyValue_InContainer(Obj) : NAME_None;
	}

	/** Call a Blueprint function whose only input is one FVector (SetStateAsSeeking(Location)). */
	bool CallWithVector(UObject* Obj, FName Function, const FVector& Value)
	{
		UFunction* Fn = Obj ? Obj->FindFunction(Function) : nullptr;
		if (!Fn)
		{
			return false;
		}
		uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Fn->ParmsSize, 1)));
		FMemory::Memzero(Params, Fn->ParmsSize);
		Fn->InitializeStruct(Params);
		for (TFieldIterator<FProperty> P(Fn); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
		{
			const FStructProperty* S = CastField<FStructProperty>(*P);
			if (S && S->Struct == TBaseStructure<FVector>::Get() && !P->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
			{
				*S->ContainerPtrToValuePtr<FVector>(Params) = Value;
			}
		}
		Obj->ProcessEvent(Fn, Params);
		Fn->DestroyStruct(Params);
		return true;
	}
}

UGOTAEnemyTargeting::UGOTAEnemyTargeting()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
	SetIsReplicatedByDefault(false);
}

UGOTAEnemyTargeting* UGOTAEnemyTargeting::EnsureOn(APawn* Pawn, float InEngageRadius, float InLeashRadius)
{
	if (!Pawn || !Pawn->HasAuthority())
	{
		return nullptr;
	}
	UGOTAEnemyTargeting* T = Pawn->FindComponentByClass<UGOTAEnemyTargeting>();
	if (!T)
	{
		T = NewObject<UGOTAEnemyTargeting>(Pawn, TEXT("GOTA_EnemyTargeting"));
		T->EngageRadius = InEngageRadius;
		T->LeashRadius = InLeashRadius;
		T->RegisterComponent();
	}
	return T;
}

void UGOTAEnemyTargeting::BeginPlay()
{
	Super::BeginPlay();
	Home = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

void UGOTAEnemyTargeting::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AAIController* AIC = BoundController.Get())
	{
		AIC->ReceiveMoveCompleted.RemoveDynamic(this, &UGOTAEnemyTargeting::HandleMoveCompleted);
	}
	Super::EndPlay(EndPlayReason);
}

void UGOTAEnemyTargeting::HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (Result == EPathFollowingResult::Blocked || Result == EPathFollowingResult::OffPath || Result == EPathFollowingResult::Invalid)
	{
		RecentMoveFailures.Add(GetWorld()->GetTimeSeconds());
	}
}

bool UGOTAEnemyTargeting::IsAttackMontagePlaying(const APawn* Self) const
{
	// Only an attack (a montage with a damage notify) counts as busy; ranged aim loops and reactions do not.
	const UAnimMontage* M = FGOTABlueprintCombat::GetActiveMontage(Self);
	return M && FGOTABlueprintCombat::FindNextImpactTime(M, 0.f) >= 0.f && !M->GetName().Contains(TEXT("Sheath"));
}

void UGOTAEnemyTargeting::Recover(AAIController* AIC, APawn* From, const TCHAR* Why, double Now)
{
	++Recoveries;
	RecoverUntil = Now + 6.0;
	RecentMoveFailures.Reset();
	APawn* Self = Cast<APawn>(GetOwner());
	// The closest point the enemy can actually reach towards a living player: the end of a partial path.
	FVector Goal = Home;
	const TCHAR* Where = TEXT("home");
	if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APawn* P = It->Get() ? It->Get()->GetPawn() : nullptr;
			if (!IsValid(P) || UGOTACombatantObserver::IsCombatantDefeated(P) || P == From)
			{
				continue;
			}
			const UNavigationPath* Path = Nav->FindPathToActorSynchronously(GetWorld(), Self->GetActorLocation(), P, 50.f, Self);
			if (Path && Path->IsValid() && Path->PathPoints.Num() > 1)
			{
				Goal = Path->PathPoints.Last();
				Where = TEXT("towards a player");
				break;
			}
		}
	}
	CallWithVector(AIC, SetSeekingFn, Goal);
	UE_LOG(LogGOTATargeting, Log, TEXT("%s: recovering (%s): moving %s to %s"), *GetNameSafe(Self), Why, Where, *Goal.ToCompactString());
}

bool UGOTAEnemyTargeting::ResolveKeys(AAIController* AIC)
{
	if (bKeysResolved)
	{
		return true;
	}
	StateKey = ReadNameVar(AIC, TEXT("StateKeyName"));
	TargetKey = ReadNameVar(AIC, TEXT("AttackTargetKeyName"));
	bKeysResolved = !StateKey.IsNone() && !TargetKey.IsNone() && AIC->FindFunction(SetAttackingFn);
	if (bKeysResolved)
	{
		UE_LOG(LogGOTATargeting, Log, TEXT("%s: targeting on (%s, keys %s/%s, engage %.0f, leash %.0f from %s)"), *GetNameSafe(GetOwner()),
			*GetNameSafe(AIC), *StateKey.ToString(), *TargetKey.ToString(), EngageRadius, LeashRadius, *Home.ToCompactString());
	}
	return bKeysResolved;
}

FString UGOTAEnemyTargeting::StateName(const UBlackboardComponent* BB) const
{
	const uint8 Value = BB->GetValueAsEnum(StateKey);
	const UBlackboardData* Asset = BB->GetBlackboardAsset();
	const FBlackboardEntry* Entry = Asset ? Asset->GetKey(Asset->GetKeyID(StateKey)) : nullptr;
	const UBlackboardKeyType_Enum* EnumKey = Entry ? Cast<UBlackboardKeyType_Enum>(Entry->KeyType) : nullptr;
	return EnumKey && EnumKey->EnumType ? EnumKey->EnumType->GetDisplayNameTextByValue(Value).ToString() : FString::FromInt(Value);
}

bool UGOTAEnemyTargeting::IsEligible(const APawn* Player, const APawn* Self, double Now) const
{
	if (!IsValid(Player) || UGOTACombatantObserver::IsCombatantDefeated(Player))
	{
		return false;
	}
	if (const double* Until = SkipUntil.Find(Player); Until && *Until > Now)
	{
		return false;
	}
	return FVector::Dist(Player->GetActorLocation(), Self->GetActorLocation()) <= EngageRadius
		&& FVector::Dist(Player->GetActorLocation(), Home) <= LeashRadius;
}

bool UGOTAEnemyTargeting::IsReachable(const APawn* Self, const APawn* Player) const
{
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav)
	{
		return true; // no navigation data: let the behavior tree try
	}
	const UNavigationPath* Path = Nav->FindPathToActorSynchronously(GetWorld(), Self->GetActorLocation(), const_cast<APawn*>(Player), 50.f,
		const_cast<APawn*>(Self));
	return Path && Path->IsValid() && !Path->IsPartial();
}

APawn* UGOTAEnemyTargeting::ChooseTarget(const APawn* Self, double Now) const
{
	TArray<APawn*> Candidates;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* P = It->Get() ? It->Get()->GetPawn() : nullptr;
		if (IsEligible(P, Self, Now))
		{
			Candidates.Add(P);
		}
	}
	const FVector From = Self->GetActorLocation();
	Candidates.Sort([&From](const APawn& A, const APawn& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From);
	});
	for (APawn* P : Candidates)
	{
		if (IsReachable(Self, P))
		{
			return P;
		}
	}
	return nullptr;
}

void UGOTAEnemyTargeting::Engage(AAIController* AIC, APawn* Target, const TCHAR* Why, double Now)
{
	FGOTABlueprintCombat::Call(AIC, SetAttackingFn, Target);
	++Acquisitions;
	LastTarget = Target;
	BestDistance = FVector::Dist(Target->GetActorLocation(), GetOwner()->GetActorLocation());
	LastProgressTime = Now;
	UE_LOG(LogGOTATargeting, Log, TEXT("%s: target %s (%s) dist %.0f"), *GetNameSafe(GetOwner()), *Label(Target), Why, BestDistance);
}

void UGOTAEnemyTargeting::ReturnHome(AAIController* AIC, const TCHAR* Why, double Now)
{
	LastHomeOrder = Now;
	if (FVector::Dist2D(GetOwner()->GetActorLocation(), Home) < 300.f)
	{
		return; // already there; wait for a player to become reachable
	}
	++HomeReturns;
	CallWithVector(AIC, SetSeekingFn, Home);
	UE_LOG(LogGOTATargeting, Log, TEXT("%s: no reachable player (%s); returning to %s"), *GetNameSafe(GetOwner()), Why, *Home.ToCompactString());
}

void UGOTAEnemyTargeting::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APawn* Self = Cast<APawn>(GetOwner());
	if (!Self || !Self->HasAuthority() || UGOTACombatantObserver::IsCombatantDefeated(Self))
	{
		return;
	}
	AAIController* AIC = Cast<AAIController>(Self->GetController());
	UBlackboardComponent* BB = AIC ? AIC->GetBlackboardComponent() : nullptr;
	if (!BB || !ResolveKeys(AIC))
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (AIC != BoundController.Get())
	{
		if (AAIController* Old = BoundController.Get())
		{
			Old->ReceiveMoveCompleted.RemoveDynamic(this, &UGOTAEnemyTargeting::HandleMoveCompleted);
		}
		BoundController = AIC;
		AIC->ReceiveMoveCompleted.AddUniqueDynamic(this, &UGOTAEnemyTargeting::HandleMoveCompleted);
	}
	RecentMoveFailures.RemoveAll([Now](double T) { return Now - T > 10.0; });
	const FString State = StateName(BB);
	if (State.Contains(TEXT("Frozen")) || State.Contains(TEXT("Dead")))
	{
		return; // statuses and death own the brain
	}
	APawn* Target = Cast<APawn>(BB->GetValueAsObject(TargetKey));
	const bool bAttacking = State.Contains(TEXT("Attack"));

	// Repeated path failures (the tree keeps asking for a move the navmesh cannot give): give up on that target for a
	// while and walk to a point that is reachable.
	if (RecentMoveFailures.Num() >= 4 && Now >= RecoverUntil)
	{
		const int32 Failures = RecentMoveFailures.Num();
		if (Target)
		{
			SkipUntil.Add(Target, Now + UnreachableSkipSeconds);
		}
		Recover(AIC, Target, *FString::Printf(TEXT("%d path failures in 10 s, target %s"), Failures, *Label(Target)), Now);
		return;
	}
	if (Now < RecoverUntil)
	{
		// Recovering: only a player who comes close ends it early.
		APawn* Near = ChooseTarget(Self, Now);
		if (Near && FVector::Dist(Near->GetActorLocation(), Self->GetActorLocation()) < 800.f)
		{
			RecoverUntil = -1.0;
			Engage(AIC, Near, TEXT("player came close while recovering"), Now);
		}
		return;
	}

	if (bAttacking && Target)
	{
		const bool bAlive = IsValid(Target) && !UGOTACombatantObserver::IsCombatantDefeated(Target);
		if (!bAlive)
		{
			if (APawn* Next = ChooseTarget(Self, Now))
			{
				Engage(AIC, Next, TEXT("previous target defeated"), Now);
			}
			else
			{
				ReturnHome(AIC, TEXT("previous target defeated"), Now);
			}
			return;
		}
		if (Target != LastTarget.Get())
		{
			// The tree chose this target itself (sight / damage sense): start watching progress.
			LastTarget = Target;
			BestDistance = FVector::Dist(Target->GetActorLocation(), Self->GetActorLocation());
			LastProgressTime = Now;
		}
		const float Dist = FVector::Dist(Target->GetActorLocation(), Self->GetActorLocation());
		const bool bBusy = IsAttackMontagePlaying(Self);
		if (Dist < BestDistance - 50.f || Dist < 450.f || bBusy)
		{
			BestDistance = FMath::Min(BestDistance, Dist);
			LastProgressTime = Now;
		}
		const bool bOutsideLeash = FVector::Dist(Target->GetActorLocation(), Home) > LeashRadius * 1.25f;
		if (bOutsideLeash || Now - LastProgressTime > PursuitTimeout)
		{
			++PursuitFailures;
			SkipUntil.Add(Target, Now + UnreachableSkipSeconds);
			UE_LOG(LogGOTATargeting, Log, TEXT("%s: pursuit of %s failed (%s, dist %.0f, best %.0f)"), *GetNameSafe(Self), *Label(Target),
				bOutsideLeash ? TEXT("left the leash") : TEXT("no progress"), Dist, BestDistance);
			if (APawn* Next = ChooseTarget(Self, Now))
			{
				Engage(AIC, Next, TEXT("previous target unreachable"), Now);
			}
			else
			{
				ReturnHome(AIC, TEXT("target unreachable"), Now);
			}
		}
		return;
	}

	// Passive, investigating or seeking: pick a target if one is eligible; otherwise go home (at most every 5 s).
	// A failed search (every candidate unreachable) is not repeated for 2 s: each candidate costs a path query.
	if (Now < NextSearchTime)
	{
		return;
	}
	if (APawn* Next = ChooseTarget(Self, Now))
	{
		Engage(AIC, Next, LastTarget.IsValid() ? TEXT("reacquired") : TEXT("acquired"), Now);
	}
	else
	{
		NextSearchTime = Now + 2.0;
		if (Now - LastHomeOrder > 5.0)
		{
			ReturnHome(AIC, TEXT("idle"), Now);
		}
	}
}
