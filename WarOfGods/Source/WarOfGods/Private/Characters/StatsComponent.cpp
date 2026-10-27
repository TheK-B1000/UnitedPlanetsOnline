// White Tiger Studios 2024

#include "Characters/StatsComponent.h"

#include "Net/UnrealNetwork.h"

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
			FCombatActorEvent Event;
			Event.Target = GetOwner();
			OnDied.Broadcast(Event);
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
