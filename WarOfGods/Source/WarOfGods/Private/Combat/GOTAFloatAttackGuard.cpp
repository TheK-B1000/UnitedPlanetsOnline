// White Tiger Studios 2024

#include "Combat/GOTAFloatAttackGuard.h"

#include "AIController.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTNode.h"
#include "Combat/GOTACombatantObserver.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAFloatAttackGuard)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAFloatGuard, Log, All);

namespace
{
	const FName StopFloatEvent(TEXT("StopFloat"));
	const FName StartFloatEvent(TEXT("StartFloat"));

	bool ClassNameIs(const UObject* Node, const TCHAR* Prefix)
	{
		return Node && Node->GetClass()->GetName().StartsWith(Prefix);
	}
}

UGOTAFloatAttackGuard::UGOTAFloatAttackGuard()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicatedByDefault(false);
}

UGOTAFloatAttackGuard* UGOTAFloatAttackGuard::EnsureOn(APawn* Pawn)
{
	if (!Pawn || !Pawn->HasAuthority() || !Pawn->FindFunction(StopFloatEvent) || !Pawn->FindFunction(StartFloatEvent))
	{
		return nullptr;
	}
	if (UGOTAFloatAttackGuard* Existing = Pawn->FindComponentByClass<UGOTAFloatAttackGuard>())
	{
		return Existing;
	}
	UGOTAFloatAttackGuard* Guard = NewObject<UGOTAFloatAttackGuard>(Pawn, TEXT("GOTA_FloatAttackGuard"));
	Guard->RegisterComponent();
	UE_LOG(LogGOTAFloatGuard, Log, TEXT("%s: float attack guard attached"), *GetNameSafe(Pawn));
	return Guard;
}

UBehaviorTreeComponent* UGOTAFloatAttackGuard::GetBrain() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AAIController* AIC = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	return AIC ? Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent()) : nullptr;
}

void UGOTAFloatAttackGuard::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const UBehaviorTreeComponent* Brain = GetBrain();
	const UBTNode* Active = Brain && Brain->IsRunning() ? Brain->GetActiveNode() : nullptr;

	if (ClassNameIs(Active, TEXT("BTT_Boss_StartFloat")))
	{
		if (!FloatSequence.IsValid())
		{
			FloatSequence = Active->GetParentNode();
			FloatStartTime = GetWorld()->GetTimeSeconds();
		}
		return;
	}
	if (!FloatSequence.IsValid())
	{
		return;
	}
	if (ClassNameIs(Active, TEXT("BTT_Boss_StopFloat")))
	{
		FloatSequence.Reset(); // the attack is landing by itself
		return;
	}
	if (UGOTACombatantObserver::IsCombatantDefeated(GetOwner()))
	{
		Land(TEXT("died mid-attack"));
		return;
	}
	if (!Active || Active->GetParentNode() != FloatSequence.Get())
	{
		// The tree left the float sequence without reaching StopFloat: target died, left or changed, branch aborted.
		Land(Active ? *FString::Printf(TEXT("sequence aborted, now %s"), *Active->GetNodeName()) : TEXT("brain stopped or idle"));
	}
}

void UGOTAFloatAttackGuard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FloatSequence.IsValid() && GetOwner() && GetOwner()->HasAuthority() && EndPlayReason == EEndPlayReason::Destroyed)
	{
		Land(TEXT("destroyed mid-attack"));
	}
	Super::EndPlay(EndPlayReason);
}

void UGOTAFloatAttackGuard::Land(const TCHAR* Why)
{
	FloatSequence.Reset();
	AActor* Owner = GetOwner();
	const double Airborne = GetWorld() ? GetWorld()->GetTimeSeconds() - FloatStartTime : 0.0;
	// The Blueprint's own landing: lowers the floating platform and clears IsInvincible one tick later.
	const bool bCalled = FGOTABlueprintCombat::Call(Owner, StopFloatEvent);
	UE_LOG(LogGOTAFloatGuard, Log, TEXT("%s: float attack ended without StopFloat (%s) after %.1fs; landing (called=%d)"),
		*GetNameSafe(Owner), Why, Airborne, bCalled ? 1 : 0);
}
