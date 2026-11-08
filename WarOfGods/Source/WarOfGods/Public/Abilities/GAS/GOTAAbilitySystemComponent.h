// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GOTAAbilitySystemComponent.generated.h"

class APawn;

/**
 * The combatant's Ability System Component. Owner and avatar are both the pawn (Docs/GOTA_GAS_ARCHITECTURE.md,
 * "Owner and avatar"): health already lives on the pawn's BPC_DamageSystem, and a respawn is a new pawn with a
 * clean combat state. The PlayerState keeps the hero choice (HeroId) and is where a future loadout would live.
 *
 * Attached on the server by UCombatBootstrapSubsystem to every pawn with a Blueprint damage system (players,
 * AI fighters, Horde enemies, bosses). Clients receive the replicated instance and never create their own.
 * Replication is Mixed: full effects to the owning client, tags and cues to everyone, attributes to everyone.
 *
 * Phase 1 owns status effects (only Slowed so far) and the combat AttributeSet. It does not own health,
 * momentum, cooldowns, ability activation, parry, dodge, block or melee.
 */
UCLASS(ClassGroup = (GOTA))
class WAROFGODS_API UGOTAAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UGOTAAbilitySystemComponent();

	/** Server: the pawn's ASC, created with the combat AttributeSet if missing. Null on clients or bad input. */
	static UGOTAAbilitySystemComponent* EnsureOnPawn(APawn* Pawn);
	/** Any peer: the actor's GOTA ASC, if it has one. */
	static UGOTAAbilitySystemComponent* Find(const AActor* Actor);

	virtual void BeginPlay() override;

	/** Combat AttributeSet value, or 1 when the set has not replicated yet. Clamped to the attribute's range. */
	float GetMoveSpeedMultiplier() const;

	/** Server: remove every effect that grants a GOTA.Status tag (death, round reset, test reset). */
	int32 ClearStatusEffects();

	/** "Slowed 2.4s x0.60" for each active status effect visible on this peer; empty when none. */
	FString DescribeStatusEffects() const;
};
