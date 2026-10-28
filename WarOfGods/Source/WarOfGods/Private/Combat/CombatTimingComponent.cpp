// White Tiger Studios 2024

#include "Combat/CombatTimingComponent.h"

#include "Characters/StatsComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/CombatTimingConfig.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTACombatTiming, Log, All);

UCombatTimingComponent::UCombatTimingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCombatTimingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!TimingConfig && !RuntimeFallbackConfig)
	{
		RuntimeFallbackConfig = NewObject<UCombatTimingConfig>(this, TEXT("DefaultCombatTiming"));
	}
}

void UCombatTimingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatTimingComponent, GuardState);
	DOREPLIFETIME(UCombatTimingComponent, LastDodgeDirection);
}

const UCombatTimingConfig* UCombatTimingComponent::GetActiveConfig() const
{
	if (TimingConfig)
	{
		return TimingConfig;
	}
	return RuntimeFallbackConfig;
}

float UCombatTimingComponent::NowSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

UCombatComponent* UCombatTimingComponent::FindCombat() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UCombatComponent>() : nullptr;
}

UStatsComponent* UCombatTimingComponent::FindStats() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UStatsComponent>() : nullptr;
}

void UCombatTimingComponent::OnRep_GuardState()
{
	OnGuardStateChanged.Broadcast(GuardState);
}

void UCombatTimingComponent::SetGuardState(ECombatGuardState NewState)
{
	if (GuardState == NewState)
	{
		return;
	}
	GuardState = NewState;
	OnGuardStateChanged.Broadcast(GuardState);
}

bool UCombatTimingComponent::IsInTimedParryWindow() const
{
	const UCombatTimingConfig* Cfg = GetActiveConfig();
	if (!Cfg || ParryStartServerTime < 0.f)
	{
		return false;
	}
	const float Age = NowSeconds() - ParryStartServerTime;
	return Age >= 0.f && Age <= Cfg->ParryWindowSeconds + Cfg->LatencyToleranceSeconds;
}

bool UCombatTimingComponent::IsInvulnerable() const
{
	const UCombatTimingConfig* Cfg = GetActiveConfig();
	if (!Cfg)
	{
		return false;
	}

	if (GuardState == ECombatGuardState::DivineCounter)
	{
		return true;
	}

	if (GuardState == ECombatGuardState::Dodging && DodgeStartServerTime >= 0.f)
	{
		const float Age = NowSeconds() - DodgeStartServerTime;
		return Age >= Cfg->DodgeIFrameStartSeconds && Age <= Cfg->DodgeIFrameEndSeconds;
	}

	return false;
}

void UCombatTimingComponent::RequestParry()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		BeginParryInternal(NowSeconds());
	}
	else
	{
		Server_RequestParry(NowSeconds());
		// Local predict for responsiveness; server reconciles via GuardState rep.
		BeginParryInternal(NowSeconds());
	}
}

void UCombatTimingComponent::Server_RequestParry_Implementation(float ClientTimeSeconds)
{
	const UCombatTimingConfig* Cfg = GetActiveConfig();
	const float ServerNow = NowSeconds();
	float Accepted = ServerNow;
	if (Cfg)
	{
		const float Drift = FMath::Abs(ServerNow - ClientTimeSeconds);
		if (Drift <= Cfg->LatencyToleranceSeconds)
		{
			Accepted = FMath::Min(ClientTimeSeconds, ServerNow);
		}
	}
	BeginParryInternal(Accepted);
}

void UCombatTimingComponent::BeginParryInternal(float ServerTimeSeconds)
{
	if (GuardState == ECombatGuardState::Dodging || GuardState == ECombatGuardState::DivineCounter)
	{
		return;
	}

	const UCombatTimingConfig* Cfg = GetActiveConfig();
	if (!Cfg)
	{
		return;
	}

	ParryStartServerTime = ServerTimeSeconds;
	SetGuardState(ECombatGuardState::Parrying);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ParryExpireHandle);
		World->GetTimerManager().SetTimer(
			ParryExpireHandle,
			FTimerDelegate::CreateUObject(this, &UCombatTimingComponent::ClearTransientDefense),
			Cfg->ParryWindowSeconds + Cfg->ParryRecoverySeconds,
			false);
	}
}

void UCombatTimingComponent::RequestBlock(bool bPressed)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		bBlockHeld = bPressed;
		if (!bPressed && GuardState == ECombatGuardState::Blocking)
		{
			SetGuardState(ECombatGuardState::Idle);
		}
		else if (bPressed && GuardState == ECombatGuardState::Idle)
		{
			SetGuardState(ECombatGuardState::Blocking);
		}
	}
	else
	{
		bBlockHeld = bPressed;
		Server_RequestBlock(bPressed);
		if (bPressed && GuardState == ECombatGuardState::Idle)
		{
			SetGuardState(ECombatGuardState::Blocking);
		}
		else if (!bPressed && GuardState == ECombatGuardState::Blocking)
		{
			SetGuardState(ECombatGuardState::Idle);
		}
	}
}

void UCombatTimingComponent::Server_RequestBlock_Implementation(bool bPressed)
{
	bBlockHeld = bPressed;
	if (!bPressed)
	{
		if (GuardState == ECombatGuardState::Blocking)
		{
			SetGuardState(ECombatGuardState::Idle);
		}
		return;
	}

	if (GuardState == ECombatGuardState::Idle)
	{
		SetGuardState(ECombatGuardState::Blocking);
	}
}

void UCombatTimingComponent::RequestDodge(FVector WorldDirection)
{
	FVector Dir = WorldDirection;
	Dir.Z = 0.f;
	if (Dir.IsNearlyZero())
	{
		Dir = GetOwner() ? GetOwner()->GetActorForwardVector() : FVector::ForwardVector;
		Dir.Z = 0.f;
	}
	Dir.Normalize();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		BeginDodgeInternal(Dir, NowSeconds());
	}
	else
	{
		Server_RequestDodge(Dir, NowSeconds());
		BeginDodgeInternal(Dir, NowSeconds());
	}
}

void UCombatTimingComponent::Server_RequestDodge_Implementation(FVector_NetQuantizeNormal WorldDirection, float ClientTimeSeconds)
{
	const UCombatTimingConfig* Cfg = GetActiveConfig();
	float Accepted = NowSeconds();
	if (Cfg)
	{
		const float Drift = FMath::Abs(Accepted - ClientTimeSeconds);
		if (Drift <= Cfg->LatencyToleranceSeconds)
		{
			Accepted = FMath::Min(ClientTimeSeconds, Accepted);
		}
	}
	BeginDodgeInternal(WorldDirection, Accepted);
}

void UCombatTimingComponent::BeginDodgeInternal(FVector WorldDirection, float ServerTimeSeconds)
{
	const UCombatTimingConfig* Cfg = GetActiveConfig();
	if (!Cfg)
	{
		return;
	}

	if (GuardState == ECombatGuardState::DivineCounter)
	{
		return;
	}

	LastDodgeDirection = WorldDirection.GetSafeNormal();
	DodgeStartServerTime = ServerTimeSeconds;
	ParryStartServerTime = -1.f;
	SetGuardState(ECombatGuardState::Dodging);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DodgeExpireHandle);
		World->GetTimerManager().SetTimer(
			DodgeExpireHandle,
			FTimerDelegate::CreateUObject(this, &UCombatTimingComponent::ClearTransientDefense),
			Cfg->DodgeDurationSeconds,
			false);
	}
}

bool UCombatTimingComponent::RequestDivineCounter(AActor* OptionalTarget)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return BeginDivineCounterInternal(OptionalTarget);
	}

	Server_RequestDivineCounter(OptionalTarget);
	return false;
}

void UCombatTimingComponent::Server_RequestDivineCounter_Implementation(AActor* OptionalTarget)
{
	BeginDivineCounterInternal(OptionalTarget);
}

bool UCombatTimingComponent::BeginDivineCounterInternal(AActor* OptionalTarget)
{
	const UCombatTimingConfig* Cfg = GetActiveConfig();
	UStatsComponent* Stats = FindStats();
	if (!Cfg || !Stats)
	{
		return false;
	}

	if (!Stats->ConsumeDivineMomentum(Cfg->DivineCounterCost))
	{
		UE_LOG(LogGOTACombatTiming, Verbose, TEXT("Divine counter denied — insufficient momentum on %s"),
			*GetNameSafe(GetOwner()));
		return false;
	}

	DivineCounterStartServerTime = NowSeconds();
	ParryStartServerTime = -1.f;
	DodgeStartServerTime = -1.f;
	SetGuardState(ECombatGuardState::DivineCounter);
	OnDivineCounterStarted.Broadcast(OptionalTarget, Cfg->DivineCounterCost);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DivineExpireHandle);
		World->GetTimerManager().SetTimer(
			DivineExpireHandle,
			FTimerDelegate::CreateUObject(this, &UCombatTimingComponent::ClearTransientDefense),
			Cfg->DivineCounterActiveSeconds,
			false);
	}
	return true;
}

void UCombatTimingComponent::ClearTransientDefense()
{
	ParryStartServerTime = -1.f;
	DodgeStartServerTime = -1.f;
	DivineCounterStartServerTime = -1.f;

	if (bBlockHeld)
	{
		SetGuardState(ECombatGuardState::Blocking);
	}
	else
	{
		SetGuardState(ECombatGuardState::Idle);
	}
}

void UCombatTimingComponent::GrantMomentum(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}
	if (UStatsComponent* Stats = FindStats())
	{
		Stats->AddDivineMomentum(Amount);
	}
}

void UCombatTimingComponent::BroadcastResolve(const FCombatDefenseResolveResult& Result)
{
	OnDefenseResolved.Broadcast(Result);
	UE_LOG(LogGOTACombatTiming, Log,
		TEXT("DefenseResolve Owner=%s Outcome=%d Cancel=%d Age=%.3f Momentum=+%.1f AttackId=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(Result.Outcome),
		Result.bCancelDamage ? 1 : 0,
		Result.DefenseAgeSeconds,
		Result.DivineMomentumGranted,
		*Result.AttackId.ToString());
}

void UCombatTimingComponent::NotifyCombatObservers(AActor* Attacker, FName AttackId, ECombatDefenseOutcome Outcome)
{
	UCombatComponent* Combat = FindCombat();
	if (!Combat)
	{
		return;
	}

	switch (Outcome)
	{
	case ECombatDefenseOutcome::Blocked:
		Combat->NotifyBlockSucceeded(Attacker, AttackId);
		break;
	case ECombatDefenseOutcome::Parried:
		Combat->NotifyParrySucceeded(Attacker, false, AttackId);
		break;
	case ECombatDefenseOutcome::PerfectParried:
		Combat->NotifyParrySucceeded(Attacker, true, AttackId);
		break;
	case ECombatDefenseOutcome::Dodged:
		// Non-perfect dodge: still evade; reuse perfect dodge notify only for perfect.
		break;
	case ECombatDefenseOutcome::PerfectDodged:
		Combat->NotifyPerfectDodge(Attacker, AttackId);
		break;
	case ECombatDefenseOutcome::DivineCountered:
		Combat->NotifyParrySucceeded(Attacker, true, AttackId);
		break;
	default:
		break;
	}
}

FCombatDefenseResolveResult UCombatTimingComponent::ResolveIncomingAttack(AActor* Attacker, FName AttackId, bool bUnparryable)
{
	FCombatDefenseResolveResult Result;
	Result.AttackId = AttackId;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		BroadcastResolve(Result);
		return Result;
	}

	// Prefer authority resolution; autonomous proxy may predict cancel locally for feel.
	const bool bAuthority = OwnerActor->HasAuthority();

	if (AttackId != NAME_None && AttackId == LastResolvedAttackId)
	{
		Result.Outcome = ECombatDefenseOutcome::None;
		Result.bCancelDamage = false;
		return Result;
	}

	const UCombatTimingConfig* Cfg = GetActiveConfig();
	if (!Cfg)
	{
		BroadcastResolve(Result);
		return Result;
	}

	const float ServerNow = NowSeconds();

	// 06 — Divine counter window: auto-perfect, cancel damage.
	if (GuardState == ECombatGuardState::DivineCounter && DivineCounterStartServerTime >= 0.f)
	{
		const float Age = ServerNow - DivineCounterStartServerTime;
		if (Age <= Cfg->DivineCounterActiveSeconds + Cfg->LatencyToleranceSeconds)
		{
			Result.Outcome = ECombatDefenseOutcome::DivineCountered;
			Result.bCancelDamage = true;
			Result.DefenseAgeSeconds = Age;
			if (bAuthority)
			{
				LastResolvedAttackId = AttackId;
				NotifyCombatObservers(Attacker, AttackId, Result.Outcome);
			}
			BroadcastResolve(Result);
			return Result;
		}
	}

	// 05 — Dodge i-frames.
	if (GuardState == ECombatGuardState::Dodging && DodgeStartServerTime >= 0.f)
	{
		const float Age = ServerNow - DodgeStartServerTime;
		const float MinAge = FMath::Max(0.f, Cfg->DodgeIFrameStartSeconds - Cfg->LatencyToleranceSeconds);
		const float MaxAge = Cfg->DodgeIFrameEndSeconds + Cfg->LatencyToleranceSeconds;
		if (Age >= MinAge && Age <= MaxAge)
		{
			const bool bPerfect = Age <= (Cfg->DodgeIFrameStartSeconds + Cfg->PerfectDodgeWindowSeconds);
			Result.Outcome = bPerfect ? ECombatDefenseOutcome::PerfectDodged : ECombatDefenseOutcome::Dodged;
			Result.bCancelDamage = true;
			Result.DefenseAgeSeconds = Age;
			Result.DivineMomentumGranted = bPerfect ? Cfg->PerfectDodgeDivineMomentum : Cfg->NormalDodgeDivineMomentum;
			if (bAuthority)
			{
				LastResolvedAttackId = AttackId;
				GrantMomentum(Result.DivineMomentumGranted);
				NotifyCombatObservers(Attacker, AttackId, Result.Outcome);
			}
			BroadcastResolve(Result);
			return Result;
		}
	}

	if (bUnparryable)
	{
		BroadcastResolve(Result);
		return Result;
	}

	// 04 — Timed parry / perfect parry.
	if (ParryStartServerTime >= 0.f)
	{
		const float Age = ServerNow - ParryStartServerTime;
		const float MaxParry = Cfg->ParryWindowSeconds + Cfg->LatencyToleranceSeconds;
		if (Age >= 0.f && Age <= MaxParry)
		{
			const bool bPerfect = Age <= (Cfg->PerfectParryWindowSeconds + Cfg->LatencyToleranceSeconds * 0.5f);
			Result.Outcome = bPerfect ? ECombatDefenseOutcome::PerfectParried : ECombatDefenseOutcome::Parried;
			Result.bCancelDamage = true;
			Result.DefenseAgeSeconds = Age;
			Result.DivineMomentumGranted = bPerfect ? Cfg->PerfectParryDivineMomentum : Cfg->NormalParryDivineMomentum;
			if (bAuthority)
			{
				LastResolvedAttackId = AttackId;
				GrantMomentum(Result.DivineMomentumGranted);
				NotifyCombatObservers(Attacker, AttackId, Result.Outcome);
				// Brief recovery then sustain block if held.
				ParryStartServerTime = -1.f;
				if (UWorld* World = GetWorld())
				{
					World->GetTimerManager().ClearTimer(ParryExpireHandle);
					World->GetTimerManager().SetTimer(
						ParryExpireHandle,
						FTimerDelegate::CreateUObject(this, &UCombatTimingComponent::ClearTransientDefense),
						Cfg->ParryRecoverySeconds,
						false);
				}
			}
			BroadcastResolve(Result);
			return Result;
		}
	}

	// Sustain block after timed window.
	if (Cfg->bAllowSustainBlock && (bBlockHeld || GuardState == ECombatGuardState::Blocking))
	{
		Result.Outcome = ECombatDefenseOutcome::Blocked;
		Result.bCancelDamage = true;
		Result.DefenseAgeSeconds = 0.f;
		if (bAuthority)
		{
			LastResolvedAttackId = AttackId;
			NotifyCombatObservers(Attacker, AttackId, Result.Outcome);
		}
		BroadcastResolve(Result);
		return Result;
	}

	BroadcastResolve(Result);
	return Result;
}
