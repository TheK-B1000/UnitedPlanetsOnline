// White Tiger Studios 2024

#include "Abilities/GAS/GOTACombatAttributeSet.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTACombatAttributeSet)

UGOTACombatAttributeSet::UGOTACombatAttributeSet()
{
	InitMoveSpeedMultiplier(1.f);
}

void UGOTACombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UGOTACombatAttributeSet, MoveSpeedMultiplier, COND_None, REPNOTIFY_Always);
}

void UGOTACombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMoveSpeedMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, MinMoveSpeedMultiplier, 2.f);
	}
}

void UGOTACombatAttributeSet::OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGOTACombatAttributeSet, MoveSpeedMultiplier, OldValue);
}
