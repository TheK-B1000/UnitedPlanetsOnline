// White Tiger Studios 2024

#include "Abilities/GAS/GOTACombatAttributeSet.h"

#include "GameplayEffectAggregator.h"
#include "GameplayEffectAggregatorLibrary.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTACombatAttributeSet)

UGOTACombatAttributeSet::UGOTACombatAttributeSet()
{
	InitMoveSpeedMultiplier(1.f);
	InitOutgoingDamageMultiplier(1.f);
	InitIncomingDamageMultiplier(1.f);
	InitShieldPool(0.f);
}

void UGOTACombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UGOTACombatAttributeSet, MoveSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGOTACombatAttributeSet, OutgoingDamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGOTACombatAttributeSet, IncomingDamageMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGOTACombatAttributeSet, ShieldPool, COND_None, REPNOTIFY_Always);
}

bool UGOTACombatAttributeSet::IsMultiplier(const FGameplayAttribute& Attribute) const
{
	return Attribute == GetMoveSpeedMultiplierAttribute() || Attribute == GetOutgoingDamageMultiplierAttribute()
		|| Attribute == GetIncomingDamageMultiplierAttribute();
}

void UGOTACombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (IsMultiplier(Attribute))
	{
		NewValue = FMath::Clamp(NewValue, MinMultiplier, MaxMultiplier);
	}
	else if (Attribute == GetShieldPoolAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGOTACombatAttributeSet::OnAttributeAggregatorCreated(const FGameplayAttribute& Attribute, FAggregator* NewAggregator) const
{
	Super::OnAttributeAggregatorCreated(Attribute, NewAggregator);
	if (NewAggregator && IsMultiplier(Attribute))
	{
		// Overlapping penalties do not compound: only the most severe one counts, buffs all count. For incoming damage a
		// "penalty" (below 1) is a reduction: the best defence counts, vulnerabilities add up.
		NewAggregator->EvaluationMetaData = &FAggregatorEvaluateMetaDataLibrary::MostNegativeMod_AllPositiveMods;
	}
}

void UGOTACombatAttributeSet::OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGOTACombatAttributeSet, MoveSpeedMultiplier, OldValue);
}

void UGOTACombatAttributeSet::OnRep_OutgoingDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGOTACombatAttributeSet, OutgoingDamageMultiplier, OldValue);
}

void UGOTACombatAttributeSet::OnRep_IncomingDamageMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGOTACombatAttributeSet, IncomingDamageMultiplier, OldValue);
}

void UGOTACombatAttributeSet::OnRep_ShieldPool(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGOTACombatAttributeSet, ShieldPool, OldValue);
}
