// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GOTABossTypes.generated.h"

class UAnimSequenceBase;
class UAnimInstance;
class UParticleSystem;
class USkeletalMesh;
class USoundBase;

/** How a boss attack resolves on the server. */
UENUM(BlueprintType)
enum class EGOTABossAttackKind : uint8
{
	/** Hit windows (montage time) sweep spheres at bones; each window hits a victim at most once. */
	Melee,
	/** Wind-up, then a committed rush along a locked direction; first contact damages + knocks back. */
	Charge,
	/** Radial damage at each hit window's start (the impact frame). Repeats ChainCount times in phase II. */
	Slam,
	/** Close-range grapple: validated range + facing at the window; damage + toss (no attachment). */
	Grab
};

/** One damaging moment inside an attack animation, in montage seconds (play rate already applied by the montage). */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTABossHitWindow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float Start = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float End = 0.45f;
	/** Bone the hit sphere follows (Melee/Grab). Slam uses the actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") FName Bone = TEXT("hand_r");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float Radius = 110.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float Damage = 12.f;
};

/** One boss attack. Every balance value lives in data (Tools/GOTA_BuildBosses.py). */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTABossAttack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") FName AttackId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") EGOTABossAttackKind Kind = EGOTABossAttackKind::Melee;
	/** Played in sequence on the boss's own skeleton (e.g. GroundSmash Start + End). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") TArray<TSoftObjectPtr<UAnimSequenceBase>> Animations;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float PlayRate = 1.f;
	/** Hit windows on the combined animation timeline (seconds from attack start at PlayRate 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") TArray<FGOTABossHitWindow> HitWindows;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float MinRange = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float MaxRange = 380.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float Cooldown = 4.f;
	/** Selection weight per phase (0 = not used in that phase). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float WeightPhase1 = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float WeightPhase2 = 1.f;
	/** Seconds from the start during which the boss keeps turning toward the target; then it commits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float TrackSeconds = 0.25f;
	/** Vulnerable recovery after the animation (missed attacks are punishable). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float Recovery = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float RecoveryScalePhase2 = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") bool bCanBeBlocked = true;
	/** A block timed into the defender's parry window deflects it: the boss recoils (ParryStaggerSeconds). Needs bCanBeBlocked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") bool bCanBeParried = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") FString HitResponse = TEXT("Stagger");
	/** Charge: speed and max duration of the rush; Slam: radius; Grab/Charge: knockback speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float ChargeSpeed = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float ChargeSeconds = 1.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float SlamRadius = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float Knockback = 700.f;
	/** Phase II: Slam repeats this many times in a chain (1 = no chain). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") int32 ChainCountPhase2 = 1;
	/** HUD telegraph shown to the player at wind-up (empty = none). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") FText Telegraph;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") TSoftObjectPtr<UParticleSystem> ImpactFX;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float FXScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") TSoftObjectPtr<USoundBase> CastSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") TSoftObjectPtr<USoundBase> ImpactSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss") float ImpactKick = 1.2f;
};

/** A boss encounter's creature: body, movement, health, phases and attacks. Primary asset "GOTABoss". */
UCLASS(BlueprintType)
class WAROFGODS_API UGOTABossDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("GOTABoss"), GetFName()); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") FName BossId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity") FText DisplayName;

	// --- body (the boss's own skeleton; never the Warden pose-copy) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") TSoftObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") TSoftClassPtr<UAnimInstance> AnimClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") float CapsuleRadius = 110.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") float CapsuleHalfHeight = 115.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") float MeshYaw = -90.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") float MeshScale = 1.f;
	/** Montage slot in the boss's anim blueprint (Paragon blueprints use "UpperBody", not "DefaultSlot"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") FName MontageSlot = TEXT("UpperBody");

	// --- movement / combat stats ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float MaxHealth = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float WalkSpeed = 420.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float WalkSpeedPhase2 = 520.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float TurnRateDegrees = 240.f;
	/** Distance the boss closes to before choosing an attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float EngageDistance = 300.f;
	/** Minimum time between attacks (anti-spam); scaled in phase II. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float AttackGap = 1.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float AttackGapPhase2 = 0.9f;
	/** Damage within StaggerWindow that interrupts a non-committed boss with a hit reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float StaggerThreshold = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float StaggerWindow = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float PhaseTwoHealthFraction = 0.5f;

	// --- reactions to the player's defence / payoff (combat vertical slice) ---
	/** Perfect parry: the attack is cut off and the boss recoils, open to punishment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactions") float ParryStaggerSeconds = 1.6f;
	/** Divine Counter: the boss is stunned (Stun start + loop), the big punish window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactions") float CounterStunSeconds = 2.6f;
	/** Divine Counter damage against this boss (it has far more health than a duel champion). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactions") float DivineCounterDamage = 90.f;
	/** Light hit reaction (upper body) on landed hits while chasing / recovering, at most this often. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactions") float FlinchCooldown = 0.9f;

	// --- set pieces ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UAnimSequenceBase> IntroAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UAnimSequenceBase> PhaseTwoAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UAnimSequenceBase> DeathAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UAnimSequenceBase> HitReactAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UAnimSequenceBase> ChargeLoopAnim;
	/** Recoil after a perfect parry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UAnimSequenceBase> ParriedAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UAnimSequenceBase> StunStartAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UAnimSequenceBase> StunLoopAnim;
	/** Directional flinches: front, back, left, right. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TArray<TSoftObjectPtr<UAnimSequenceBase>> FlinchAnims;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<USoundBase> RoarSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation") TSoftObjectPtr<UParticleSystem> PhaseTwoFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attacks") TArray<FGOTABossAttack> Attacks;
};
