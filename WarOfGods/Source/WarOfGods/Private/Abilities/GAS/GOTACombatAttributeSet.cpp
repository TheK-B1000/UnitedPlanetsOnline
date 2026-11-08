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
}

void UGOTACombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UGOTACombatAttributeSet, MoveSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGOTACombatAttributeSet, OutgoingDamageMultiplier, COND_None, REPNOTIFY_Always);
}

void UGOTACombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMoveSpeedMultiplierAttribute() || Attribute == GetOutgoingDamageMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, MinMultiplier, MaxMultiplier);
	}
}

void UGOTACombatAttributeSet::OnAttributeAggregatorCreated(const FGameplayAttribute& Attribute, FAggregator* NewAggregator) const
{
	Super::OnAttributeAggregatorCreated(Attribute, NewAggregator);
	if (NewAggregator && (Attribute == GetMoveSpeedMultiplierAttribute() || Attribute == GetOutgoingDamageMultiplierAttribute()))
	{
		// Overlapping penalties do not compound: only the most severe one counts, buffs all count.
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
