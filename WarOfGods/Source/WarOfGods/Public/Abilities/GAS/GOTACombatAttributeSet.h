// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GOTACombatAttributeSet.generated.h"

/**
 * Combat modifiers that have no other owner. Deliberately holds no Health, Momentum, Stamina or cooldowns:
 * those stay on BPC_DamageSystem, UStatsComponent and UGOTAHeroAbilityComponent (Docs/GOTA_GAS_ARCHITECTURE.md).
 * Values are multipliers with a base of 1. Effects change them; game code only reads them.
 */
UCLASS()
class WAROFGODS_API UGOTACombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGOTACombatAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** Walk speed multiplier. Read by UGOTAHeroAbilityComponent, the only writer of MaxWalkSpeed. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeedMultiplier, Category = "GOTA|Attributes")
	FGameplayAttributeData MoveSpeedMultiplier;
	ATTRIBUTE_ACCESSORS_BASIC(UGOTACombatAttributeSet, MoveSpeedMultiplier);

	/** Lowest walk speed multiplier any combination of effects can reach (matches the pre-GAS clamp). */
	static constexpr float MinMoveSpeedMultiplier = 0.2f;

protected:
	UFUNCTION()
	void OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldValue);
};
