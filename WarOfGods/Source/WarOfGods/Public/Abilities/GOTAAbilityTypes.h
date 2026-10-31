// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GOTAAbilityTypes.generated.h"

class UAnimSequenceBase;
class UParticleSystem;
class USoundBase;

/** What an ability does when it resolves on the server. Each kind is one well-tested code path. */
UENUM(BlueprintType)
enum class EGOTAAbilityKind : uint8
{
	/** Damage everything valid inside Range and an arc of ArcDegrees in front of the caster. */
	MeleeArc,
	/** Dash forward up to Range; first enemy in the path takes Damage and is knocked back. */
	Charge,
	/** Self status for StatusDuration (e.g. damage reduction, evasion). No damage. */
	SelfGuard,
	/** Damage in Radius around the caster after the cast time (shockwave / eruption). */
	AreaBurst,
	/** Teleport behind the target (or forward) with a validated, collision-safe destination. */
	Teleport,
	/** HitCount strikes HitInterval apart on the target in front (Range/ArcDegrees each strike). */
	MultiHit,
	/** Pull the target toward the caster (range + line-of-sight validated). */
	Pull,
	/** Apply a debuff to enemies in Radius for StatusDuration. */
	Aura
};

/** Status effects the ability layer owns. Health is never written here — see the authority contract. */
UENUM(BlueprintType)
enum class EGOTAStatus : uint8
{
	None,
	/** Incoming ability damage scaled by (1 - Magnitude); Blueprint melee is guarded (see component). */
	Unbreakable,
	/** Untargetable by abilities and immune to Blueprint hits (the damage system's IsInvincible). */
	Veiled,
	/** Movement speed scaled by (1 - Magnitude) and outgoing ability damage scaled by (1 - Magnitude * 0.5). */
	Dread
};

/** Cosmetic family used to pick placeholder VFX/SFX until hero-specific effects exist. */
UENUM(BlueprintType)
enum class EGOTAAbilityLook : uint8
{
	Titan,   // Drakon: heavy sparks, deep impact
	Shadow,  // Nyx: quick dark bursts
	Void     // Varoth: dark energy
};

/** One divine ability: every balancing value lives here (authored by Tools/GOTA_BuildHeroAbilities.py). */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAAbilityDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability") FName AbilityId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (MultiLine = true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability") EGOTAAbilityKind Kind = EGOTAAbilityKind::MeleeArc;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability") bool bUltimate = false;

	// --- balance (Docs/GOTA_HERO_ABILITIES.md) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float Damage = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float Cooldown = 8.f;
	/** Divine Momentum spent on activation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float MomentumCost = 0.f;
	/** Wind-up before the effect resolves (the readable tell). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float CastTime = 0.4f;
	/** Vulnerable recovery after the effect, before the caster can act again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float RecoveryTime = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float Range = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float Radius = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float ArcDegrees = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") int32 HitCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float HitInterval = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") EGOTAStatus Status = EGOTAStatus::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float StatusDuration = 0.f;
	/** Status strength (damage reduction / slow fraction) or knockback/pull speed for Charge/Pull. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") float Magnitude = 0.f;
	/** Blockable by the Blueprint guard (counterplay). Ultimates are dodgeable instead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") bool bCanBeBlocked = true;
	/** Taking a hit during the cast cancels the ability (cooldown is still spent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") bool bInterruptible = true;
	/** "Stagger", "Knockback" ... — the Blueprint E_DamageResponse entry used for hits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance") FString HitResponse = TEXT("Stagger");

	// --- presentation ---
	/** Hero-specific animation played on the visible body (Paragon sequence). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TSoftObjectPtr<UAnimSequenceBase> Animation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") float AnimPlayRate = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") float AnimStartTime = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") EGOTAAbilityLook Look = EGOTAAbilityLook::Titan;
	/** Hero VFX (Paragon pack particle systems). Cast plays on the caster at activation; impact where it lands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TSoftObjectPtr<UParticleSystem> CastFX;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TSoftObjectPtr<UParticleSystem> ImpactFX;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") float FXScale = 1.f;
	/** Seconds before spawned FX are removed (keeps effects from lingering over the fight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") float FXLifetime = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TSoftObjectPtr<USoundBase> CastSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TSoftObjectPtr<USoundBase> ImpactSound;
	/** Camera shake strength on impact for the players involved. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") float ImpactKick = 1.f;
	/** Short placeholder text for the HUD slot until icon art exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") FText ShortLabel;

	// --- AI ---
	/** Higher = preferred when several abilities are usable. 0 = AI never uses it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI") float AIPriority = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI") float AIMinDistance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI") float AIMaxDistance = 400.f;
	/** Self abilities: only when own health fraction is at or below this (1 = any time). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI") float AIMaxOwnHealthFraction = 1.f;
};

/** A hero's four divine abilities (slots 1-3 + ultimate). Primary asset type "GOTAAbilitySet". */
UCLASS(BlueprintType)
class WAROFGODS_API UGOTAAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("GOTAAbilitySet"), GetFName()); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<FGOTAAbilityDef> Abilities;

	/** Spawn-time MaxHealth multiplier for this hero (durability identity; applied once before the round). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	float MaxHealthScale = 1.f;

	/** Minimum time between any two AI ability uses (anti-spam). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float AIGlobalGap = 3.5f;
};
