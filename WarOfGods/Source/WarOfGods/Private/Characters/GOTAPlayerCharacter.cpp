// White Tiger Studios 2024

#include "Characters/GOTAPlayerCharacter.h"

#include "Combat/GOTACombatantObserver.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTANetGuard, Log, All);

namespace GOTANetGuard
{
	static const FName ParryAttack(TEXT("SR_ParryAttack"));
	static const FName OnBlocked(TEXT("SR_OnBlocked"));

	static bool ReadBool(const UObject* Obj, FName Name)
	{
		const FBoolProperty* P = Obj ? FindFProperty<FBoolProperty>(Obj->GetClass(), Name) : nullptr;
		return P && P->GetPropertyValue_InContainer(Obj);
	}
}

AGOTAPlayerCharacter::AGOTAPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGOTAPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Server: remember when this pawn was last defending, so a riposte request that arrives one round trip
	// after the block is still honoured.
	if (HasAuthority() && IsDefendingNow())
	{
		LastDefendingTime = GetWorld()->GetTimeSeconds();
	}
	NeutralizeUnsafeGlobalSlowMo();
}

void AGOTAPlayerCharacter::NeutralizeUnsafeGlobalSlowMo()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Standalone)
	{
		return; // solo: keep BP_Player's authored feel
	}
	const float Dil = UGameplayStatics::GetGlobalTimeDilation(World);
	if (FMath::IsNearlyEqual(Dil, 1.f))
	{
		return;
	}
	// Convert to actor-local presentation for the controlling player only; never slow the shared sim.
	if (IsLocallyControlled() && Dil < 1.f && LocalParrySlowMoSeconds > 0.f)
	{
		CustomTimeDilation = Dil;
		TWeakObjectPtr<AGOTAPlayerCharacter> WeakThis(this);
		World->GetTimerManager().SetTimer(LocalSlowMoHandle, FTimerDelegate::CreateLambda([WeakThis]()
		{
			if (AGOTAPlayerCharacter* Self = WeakThis.Get())
			{
				Self->CustomTimeDilation = 1.f;
			}
		}), LocalParrySlowMoSeconds, false);
	}
	UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	if (!bLoggedGlobalSlowMoClamp)
	{
		bLoggedGlobalSlowMoClamp = true;
		UE_LOG(LogGOTANetGuard, Warning,
			TEXT("%s: clamped SetGlobalTimeDilation in multiplayer (converted to CustomTimeDilation locally)"),
			*GetName());
	}
}

bool AGOTAPlayerCharacter::IsDefendingNow() const
{
	const UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(this);
	return GOTANetGuard::ReadBool(DS, TEXT("IsBlocking")) || GOTANetGuard::ReadBool(this, TEXT("IsWithinParryWindow"));
}

AActor* AGOTAPlayerCharacter::ReadActorParam(const UFunction* Function, void* Parms)
{
	for (TFieldIterator<FProperty> P(Function); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
	{
		if (const FObjectPropertyBase* O = CastField<FObjectPropertyBase>(*P))
		{
			return Cast<AActor>(O->GetObjectPropertyValue_InContainer(Parms));
		}
	}
	return nullptr;
}

bool AGOTAPlayerCharacter::ValidateServerRPC(const UFunction* Function, void* Parms)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const FName Name = Function->GetFName();
	static const FName CombatActions[] = { TEXT("SR_MeleeAttack"), TEXT("SR_ShootMagicSpell"), TEXT("SR_StartBlock"), GOTANetGuard::ParryAttack, GOTANetGuard::OnBlocked };
	bool bCombatAction = false;
	for (const FName& A : CombatActions)
	{
		bCombatAction |= Name == A;
	}
	if (bCombatAction && UGOTACombatantObserver::IsCombatantDefeated(this))
	{
		return false; // the dead cannot attack, block or parry
	}
	const bool bDefending = IsDefendingNow() || Now - LastDefendingTime <= ParryGraceSeconds;
	if (Name == GOTANetGuard::ParryAttack || Name == GOTANetGuard::OnBlocked)
	{
		AActor* Target = ReadActorParam(Function, Parms);
		const float Range = Name == GOTANetGuard::ParryAttack ? ParryRiposteRange : BlockedCauserRange;
		if (!IsValid(Target) || Target == this || UGOTACombatantObserver::IsCombatantDefeated(Target)
			|| FVector::Dist(Target->GetActorLocation(), GetActorLocation()) > Range || !bDefending)
		{
			return false;
		}
		if (Name == GOTANetGuard::ParryAttack)
		{
			if (Now - LastRiposteTime < ParryRiposteCooldown)
			{
				return false;
			}
			LastRiposteTime = Now;
		}
	}
	return true;
}

void AGOTAPlayerCharacter::ProcessEvent(UFunction* Function, void* Parms)
{
	if (Function && Function->HasAnyFunctionFlags(FUNC_NetServer) && HasAuthority() && GetWorld() && GetWorld()->IsGameWorld()
		&& Function->GetName().StartsWith(TEXT("SR_")) && !ValidateServerRPC(Function, Parms))
	{
		++RejectedRPCs;
		UE_LOG(LogGOTANetGuard, Warning, TEXT("%s: rejected %s (failed server validation, %d rejected so far)"), *GetName(), *Function->GetName(), RejectedRPCs);
		return;
	}
	Super::ProcessEvent(Function, Parms);
}
