// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GOTACombatAttributeSet.generated.h"

/**
 * Combat modifiers that have no other owner. Deliberately holds no Health, Momentum, Stamina or cooldowns:
 * those stay on BPC_DamageSystem, UStatsComponent and UGOTAHeroAbilityComponent (Docs/GOTA_GAS_ENHANCEMENT_PLAN.md).
 * Values are multipliers with a base of 1. Effects change them; game code only reads them.
 *
 * Both attributes aggregate "strongest penalty wins": of all active modifiers that lower the value only the most
 * severe applies, and every modifier that raises it still applies. Dread (x0.65 walk) and Time Lash (x0.6 walk)
 * together give x0.6, not x0.39.
 */
UCLASS()
class WAROFGODS_API UGOTACombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGOTACombatAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void OnAttributeAggregatorCreated(const FGameplayAttribute& Attribute, FAggregator* NewAggregator) const override;

	/** Walk speed multiplier. Read by UGOTAHeroAbilityComponent, the only writer of MaxWalkSpeed. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeedMultiplier, Category = "GOTA|Attributes")
	FGameplayAttributeData MoveSpeedMultiplier;
	ATTRIBUTE_ACCESSORS_BASIC(UGOTACombatAttributeSet, MoveSpeedMultiplier);

	/**
	 * Damage this combatant deals through the C++ damage paths, read once per hit just before the amount goes to the
	 * victim's BPC_DamageSystem: hero abilities (UGOTAHeroAbilityComponent::DealDamage) and boss attacks
	 * (UGOTABossComponent::DamageTarget). Blueprint melee does not read it (it did not read Dread either).
	 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OutgoingDamageMultiplier, Category = "GOTA|Attributes")
	FGameplayAttributeData OutgoingDamageMultiplier;
	ATTRIBUTE_ACCESSORS_BASIC(UGOTACombatAttributeSet, OutgoingDamageMultiplier);

	/** Lowest value any combination of effects can reach (matches the pre-GAS clamps). */
	static constexpr float MinMultiplier = 0.2f;
	static constexpr float MaxMultiplier = 3.f;

protected:
	UFUNCTION()
	void OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_OutgoingDamageMultiplier(const FGameplayAttributeData& OldValue);
};
