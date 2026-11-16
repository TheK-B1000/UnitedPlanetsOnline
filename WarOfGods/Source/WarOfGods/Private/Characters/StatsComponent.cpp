// White Tiger Studios 2024

#include "Characters/StatsComponent.h"

#include "Arenas/Duel/GOTADuelCombat.h"
#include "Combat/GOTACombatantObserver.h"
#include "Combat/GOTACombatTelemetry.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTAStats, Log, All);

UStatsComponent::UStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStatsComponent, Health);
	DOREPLIFETIME(UStatsComponent, MaxHealth);
	DOREPLIFETIME(UStatsComponent, Stamina);
	DOREPLIFETIME(UStatsComponent, MaxStamina);
	DOREPLIFETIME(UStatsComponent, DivineMomentum);
	DOREPLIFETIME(UStatsComponent, MaxDivineMomentum);
	DOREPLIFETIME(UStatsComponent, bHealthObserved);
}

void UStatsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UActorComponent* DS = ObservedDamageSystem.Get())
	{
		FGOTABlueprintCombat::UnbindDelegate(DS, TEXT("OnDamageResponse"), this, GET_FUNCTION_NAME_CHECKED(UStatsComponent, HandleBlueprintDamageResponse));
		FGOTABlueprintCombat::UnbindDelegate(DS, TEXT("OnDeath"), this, GET_FUNCTION_NAME_CHECKED(UStatsComponent, HandleBlueprintDeath));
	}
	ObservedDamageSystem.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SyncTimer);
	}
	OnHealthChanged.Clear();
	OnStaminaChanged.Clear();
	OnDivineMomentumChanged.Clear();
	OnDied.Clear();
	Super::EndPlay(EndPlayReason);
}

void UStatsComponent::BeginObservingBlueprintDamage()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || ObservedDamageSystem.IsValid())
	{
		return;
	}
	UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Owner);
	if (!DS)
	{
		return;
	}
	ObservedDamageSystem = DS;
	FGOTABlueprintCombat::BindDelegate(DS, TEXT("OnDamageResponse"), this, GET_FUNCTION_NAME_CHECKED(UStatsComponent, HandleBlueprintDamageResponse));
	FGOTABlueprintCombat::BindDelegate(DS, TEXT("OnDeath"), this, GET_FUNCTION_NAME_CHECKED(UStatsComponent, HandleBlueprintDeath));
	SyncFromBlueprint();
	GetWorld()->GetTimerManager().SetTimer(SyncTimer, this, &UStatsComponent::SyncFromBlueprint, 0.1f, true);
}

void UStatsComponent::SyncFromBlueprint()
{
	AActor* Owner = GetOwner();
	UActorComponent* DS = ObservedDamageSystem.Get();
	if (!Owner || !DS || !Owner->HasAuthority())
	{
		return;
	}
	const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Owner);
	if (!V.bHasDamageSystem)
	{
		return;
	}
	bHealthObserved = true;
	const float NewHealth = V.bIsDead ? 0.f : V.Health;
	if (NewHealth > 0.f)
	{
		bDeathBroadcast = false; // healed / respawned: a new life
	}
	const float OldHealth = Health;
	if (!FMath::IsNearlyEqual(NewHealth, Health) || !FMath::IsNearlyEqual(FMath::Max(V.MaxHealth, 1.f), MaxHealth))
	{
		ObserveHealth(NewHealth, V.MaxHealth);
	}
	ReportTelemetry(OldHealth, Health);
}

void UStatsComponent::ReportTelemetry(float Before, float After)
{
	UGOTACombatTelemetry* Telemetry = UGOTACombatTelemetry::Get(this);
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (!Telemetry || !bTelemetryPrimed)
	{
		bTelemetryPrimed = true; // the first read only replaces the default value
		PendingResponseTime = -1.0;
		return;
	}
	const bool bResponseFresh = PendingResponseTime >= 0.0 && Now - PendingResponseTime <= 0.5;
	if (!FMath::IsNearlyEqual(Before, After))
	{
		APawn* By = nullptr;
		int32 Response = -1;
		if (After < Before)
		{
			// The killing blow can arrive through OnDeath without a response: fall back to the last damager.
			By = bResponseFresh ? PendingResponseBy.Get() : GetRecentDamager(3.f);
			Response = bResponseFresh ? PendingResponse : -1;
			PendingResponseTime = -1.0;
		}
		Telemetry->NoteHealthChange(GetOwner(), Before, After, By, Response);
	}
	else if (PendingResponseTime >= 0.0 && Now - PendingResponseTime > 0.35)
	{
		// A damage response with no health change: blocked, invincible or in i-frames.
		Telemetry->NoteNoDamageResponse(GetOwner(), PendingResponseBy.Get(), PendingResponse);
		PendingResponseTime = -1.0;
	}
}

APawn* UStatsComponent::GetRecentDamager(float Seconds) const
{
	const UWorld* World = GetWorld();
	return (World && World->GetTimeSeconds() - LastDamageTime <= Seconds) ? LastDamager.Get() : nullptr;
}

void UStatsComponent::NoteDamageFrom(AActor* DamageCauser)
{
	RecordDamager(DamageCauser);
	SyncFromBlueprint();
}

APawn* UStatsComponent::RecordDamager(AActor* DamageCauser)
{
	// The causer may be the attacker, its weapon or a projectile: walk instigator / owner to the pawn.
	APawn* By = Cast<APawn>(DamageCauser);
	for (AActor* A = DamageCauser; A && !By; A = A->GetOwner())
	{
		By = A->GetInstigator() ? A->GetInstigator() : Cast<APawn>(A->GetOwner());
	}
	if (By && By != GetOwner())
	{
		LastDamager = By;
		LastDamageTime = GetWorld()->GetTimeSeconds();
	}
	return By;
}

void UStatsComponent::HandleBlueprintDamageResponse(uint8 DamageResponse, AActor* DamageCauser)
{
	APawn* By = RecordDamager(DamageCauser);
	PendingResponseBy = By != GetOwner() ? By : nullptr;
	PendingResponseTime = GetWorld()->GetTimeSeconds();
	PendingResponse = DamageResponse;
	SyncFromBlueprint();
}

void UStatsComponent::HandleBlueprintDeath()
{
	SyncFromBlueprint();
	BroadcastDeath();
}

void UStatsComponent::BroadcastDeath()
{
	if (bDeathBroadcast)
	{
		return;
	}
	bDeathBroadcast = true;
	FCombatActorEvent Event;
	Event.Target = GetOwner();
	Event.Instigator = GetRecentDamager();
	UE_LOG(LogGOTAStats, Log, TEXT("Combatant %s defeated (by %s)"), *GetNameSafe(GetOwner()), *GetNameSafe(Event.Instigator.Get()));
	OnDied.Broadcast(Event);
	StopDefeatedAI();
}

void UStatsComponent::StopDefeatedAI()
{
	// A defeated AI combatant must not act. The enemy template leaves the body in the world and its behavior tree kept
	// running: corpses walked, attacked and dealt damage for up to 20 s after death (RC11/RC12 logs: hundreds of hits by
	// already-dead enemies, including the blows that killed survivors during a Horde intermission).
	APawn* Pawn = Cast<APawn>(GetOwner());
	AAIController* AIC = Pawn && Pawn->HasAuthority() ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}
	if (UBrainComponent* Brain = AIC->GetBrainComponent())
	{
		Brain->StopLogic(TEXT("Defeated"));
	}
	AIC->StopMovement();
	AIC->ClearFocus(EAIFocusPriority::Gameplay);
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UAnimInstance* Anim = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			Anim->StopAllMontages(0.2f); // an attack already in flight cannot land its damage notify
		}
	}
	UE_LOG(LogGOTAStats, Log, TEXT("%s: defeated AI stopped (brain, movement, attacks)"), *GetNameSafe(Pawn));
}

void UStatsComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bAuthoritativeMutations)
	{
		Health = MaxHealth;
		Stamina = MaxStamina;
	}
}

void UStatsComponent::SetAuthoritativeMutationsEnabled(bool bEnabled)
{
	bAuthoritativeMutations = bEnabled;
}

void UStatsComponent::ObserveHealth(float Current, float Maximum)
{
	MaxHealth = FMath::Max(Maximum, 1.f);
	const float Old = Health;
	Health = FMath::Clamp(Current, 0.f, MaxHealth);
	if (!FMath::IsNearlyEqual(Old, Health))
	{
		OnHealthChanged.Broadcast(Health, MaxHealth);
		if (Health <= 0.f && Old > 0.f)
		{
			BroadcastDeath(); // server: exactly once per life, with the killer
		}
	}
}

void UStatsComponent::SetHealth(float NewHealth)
{
	if (!bAuthoritativeMutations || !HasAuthorityToMutate())
	{
		return;
	}

	const float Old = Health;
	Health = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	if (!FMath::IsNearlyEqual(Old, Health))
	{
		OnHealthChanged.Broadcast(Health, MaxHealth);
		if (Health <= 0.f && Old > 0.f)
		{
			FCombatActorEvent Event;
			Event.Target = GetOwner();
			OnDied.Broadcast(Event);
		}
	}
}

void UStatsComponent::ApplyDamage(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}
	SetHealth(Health - Amount);
}

void UStatsComponent::Heal(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}
	SetHealth(Health + Amount);
}

bool UStatsComponent::ConsumeStamina(float Amount)
{
	if (!bAuthoritativeMutations || !HasAuthorityToMutate() || Amount <= 0.f)
	{
		return false;
	}
	if (Stamina < Amount)
	{
		return false;
	}

	Stamina = FMath::Clamp(Stamina - Amount, 0.f, MaxStamina);
	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
	return true;
}

void UStatsComponent::RestoreStamina(float Amount)
{
	if (!bAuthoritativeMutations || !HasAuthorityToMutate() || Amount <= 0.f)
	{
		return;
	}

	const float Old = Stamina;
	Stamina = FMath::Clamp(Stamina + Amount, 0.f, MaxStamina);
	if (!FMath::IsNearlyEqual(Old, Stamina))
	{
		OnStaminaChanged.Broadcast(Stamina, MaxStamina);
	}
}

void UStatsComponent::AddDivineMomentum(float Amount)
{
	// DivineMomentum is C++-owned even while Health stays BP-owned.
	if (!HasAuthorityToMutate() || Amount <= 0.f)
	{
		return;
	}

	const float Old = DivineMomentum;
	DivineMomentum = FMath::Clamp(DivineMomentum + Amount, 0.f, MaxDivineMomentum);
	if (!FMath::IsNearlyEqual(Old, DivineMomentum))
	{
		OnDivineMomentumChanged.Broadcast(DivineMomentum, MaxDivineMomentum);
	}
}

bool UStatsComponent::ConsumeDivineMomentum(float Amount)
{
	if (!HasAuthorityToMutate() || Amount <= 0.f)
	{
		return false;
	}
	if (DivineMomentum < Amount)
	{
		return false;
	}

	DivineMomentum = FMath::Clamp(DivineMomentum - Amount, 0.f, MaxDivineMomentum);
	OnDivineMomentumChanged.Broadcast(DivineMomentum, MaxDivineMomentum);
	return true;
}

void UStatsComponent::OnRep_Health(float OldHealth)
{
	// Clients: presentation events from the replicated, server-mirrored value (never a gameplay decision).
	OnHealthChanged.Broadcast(Health, MaxHealth);
	if (Health <= 0.f && OldHealth > 0.f)
	{
		FCombatActorEvent Event;
		Event.Target = GetOwner();
		OnDied.Broadcast(Event);
	}
}

void UStatsComponent::OnRep_Stamina(float OldStamina)
{
	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
}

void UStatsComponent::OnRep_DivineMomentum(float OldValue)
{
	OnDivineMomentumChanged.Broadcast(DivineMomentum, MaxDivineMomentum);
}

bool UStatsComponent::HasAuthorityToMutate() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor == nullptr || OwnerActor->HasAuthority();
}
