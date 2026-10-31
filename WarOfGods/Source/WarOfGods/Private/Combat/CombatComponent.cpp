// White Tiger Studios 2024

#include "Combat/CombatComponent.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::NotifyAttackStarted(AActor* Target, FGameplayTagContainer Tags)
{
	FCombatActorEvent Event;
	Event.Instigator = GetOwner();
	Event.Target = Target;
	Event.Tags = MoveTemp(Tags);
	OnAttackStarted.Broadcast(Event);
}

void UCombatComponent::NotifyAttackCommitted(AActor* Target, FGameplayTagContainer Tags)
{
	FCombatActorEvent Event;
	Event.Instigator = GetOwner();
	Event.Target = Target;
	Event.Tags = MoveTemp(Tags);
	OnAttackCommitted.Broadcast(Event);
}

void UCombatComponent::NotifyHitConfirmed(AActor* Victim, float Damage, FVector HitLocation, FName AttackId)
{
	FCombatHitEvent Event;
	Event.Attacker = GetOwner();
	Event.Victim = Victim;
	Event.Damage = Damage;
	Event.HitLocation = HitLocation;
	Event.AttackId = AttackId;
	OnHitConfirmed.Broadcast(Event);
}

void UCombatComponent::NotifyDamageApplied(AActor* Victim, float Damage, FVector HitLocation, FName AttackId)
{
	FCombatHitEvent Event;
	Event.Attacker = GetOwner();
	Event.Victim = Victim;
	Event.Damage = Damage;
	Event.HitLocation = HitLocation;
	Event.AttackId = AttackId;
	OnDamageApplied.Broadcast(Event);
}

void UCombatComponent::NotifyBlockSucceeded(AActor* Attacker, FName AttackId)
{
	FCombatDefenseEvent Event;
	Event.Defender = GetOwner();
	Event.Attacker = Attacker;
	Event.bPerfect = false;
	Event.AttackId = AttackId;
	OnBlockSucceeded.Broadcast(Event);
}

void UCombatComponent::NotifyParrySucceeded(AActor* Attacker, bool bPerfect, FName AttackId)
{
	FCombatDefenseEvent Event;
	Event.Defender = GetOwner();
	Event.Attacker = Attacker;
	Event.bPerfect = bPerfect;
	Event.AttackId = AttackId;
	OnParrySucceeded.Broadcast(Event);
	if (bPerfect)
	{
		OnPerfectParry.Broadcast(Event);
	}
}

void UCombatComponent::NotifyPerfectDodge(AActor* Attacker, FName AttackId)
{
	FCombatDefenseEvent Event;
	Event.Defender = GetOwner();
	Event.Attacker = Attacker;
	Event.bPerfect = true;
	Event.AttackId = AttackId;
	OnPerfectDodge.Broadcast(Event);
}

void UCombatComponent::NotifyStaggered(AActor* Instigator, FGameplayTagContainer Tags)
{
	FCombatActorEvent Event;
	Event.Instigator = Instigator;
	Event.Target = GetOwner();
	Event.Tags = MoveTemp(Tags);
	OnCharacterStaggered.Broadcast(Event);
}

void UCombatComponent::NotifyDied(AActor* Killer, FGameplayTagContainer Tags)
{
	FCombatActorEvent Event;
	Event.Instigator = Killer;
	Event.Target = GetOwner();
	Event.Tags = MoveTemp(Tags);
	OnCharacterDied.Broadcast(Event);
}
