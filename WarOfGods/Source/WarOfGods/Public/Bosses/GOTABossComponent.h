// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Bosses/GOTABossTypes.h"
#include "GOTABossComponent.generated.h"

class ACameraActor;
class UAnimSequenceBase;

UENUM(BlueprintType)
enum class EGOTABossState : uint8
{
	Dormant,     // spawned, round not live yet
	Intro,       // cinematic: no attacks, player protected
	Chase,       // closing to engage distance
	Attacking,   // committed to an attack timeline
	Recovering,  // punish window after an attack
	Staggered,   // hit reaction
	Transition,  // phase II roar (invulnerable)
	Dead
};

/**
 * Boss brain + presentation on a boss pawn (BP_Boss_*, derived from BP_Enemy_Base so the Blueprint
 * damage system stays the single health authority and the player's weapon hits register normally).
 *
 * Server: target selection, movement (AIController MoveTo on the arena navmesh), attack choice by
 * distance / cooldown / phase weight / line of sight, attack timelines with animation-timed hit
 * windows (bone spheres, one hit per window per victim), charge, slams, grapple, stagger, the single
 * phase II transition, death, and the intro cinematic. Damage goes through the victim's
 * BPC_DamageSystem (blockable where the attack allows it).
 * Clients: body setup from the replicated definition, animation/FX/sound via multicasts, HUD state.
 */
UCLASS(ClassGroup = (GOTA), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API UGOTABossComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTABossComponent();

	/** Server: add (or reuse) on Pawn, configure its body and stats from Definition. */
	static UGOTABossComponent* ApplyToPawn(APawn* Pawn, UGOTABossDefinition* Definition);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	const UGOTABossDefinition* GetDefinition() const { return Definition.Get(); }
	int32 GetPhase() const { return Phase; }
	EGOTABossState GetState() const { return State; }
	bool IsIntroActive() const { return State == EGOTABossState::Intro; }
	FText GetLastTelegraph() const { return LastTelegraph; }
	FString DescribeState() const;

	/** Server: begin the intro cinematic now (called when the first round goes live). */
	void StartIntro();
	/** Skip the intro (local host input or test command). */
	void SkipIntro();
	/** Test hooks (server). */
	void DebugForceAttack(FName AttackId);
	void DebugSetAI(bool bEnabled) { bAIEnabled = bEnabled; }

protected:
	UFUNCTION()
	void OnRep_Definition();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySequence(UAnimSequenceBase* Sequence, float PlayRate, bool bHoldLastFrame, int32 LoopCount);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopAnimation(float BlendOut);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Impact(int32 AttackIndex, FVector_NetQuantize Location, bool bLanded);

	/** Cosmetic cue for every peer's HUD/camera (EGOTADuelCue as uint8). */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Cue(uint8 Cue, const FText& Text);

	void ApplyBody();
	void TickServer(float DeltaTime);
	void TickChase(APawn* Target, double Now);
	void TickAttack(APawn* Target, double Now);
	bool ChooseAttack(APawn* Target, double Now);
	void BeginAttack(int32 Index, APawn* Target);
	void EndAttack(bool bHitSomething);
	void BeginPhaseTwo();
	void BeginStagger();
	void Die();
	void EndIntro();
	void ResolveWindow(const FGOTABossAttack& Attack, const FGOTABossHitWindow& Window, int32 WindowIndex, APawn* Target);
	bool DamageTarget(APawn* Target, const FGOTABossAttack& Attack, float Amount);
	void Knock(APawn* Target, float Speed, float Up);
	void FaceTowards(const FVector& Location, float DeltaTime, bool bInstant);
	void SetMoveLocked(bool bLocked);
	void SetBrainMoving(APawn* Target, bool bMove);
	void PlayLocal(UAnimSequenceBase* Sequence, float PlayRate, bool bHoldLastFrame, int32 LoopCount);
	APawn* FindTarget() const;
	bool HasLineOfSight(const AActor* A, const AActor* B) const;
	bool IsRoundLive() const;
	float CurrentAttackGap() const;
	float AttackClock(double Now) const;

	UPROPERTY(ReplicatedUsing = OnRep_Definition)
	TSoftObjectPtr<UGOTABossDefinition> Definition;

	UPROPERTY(Replicated)
	uint8 Phase = 1;

	UPROPERTY(Replicated)
	EGOTABossState State = EGOTABossState::Dormant;

	UPROPERTY(Replicated)
	int32 AttackIndex = INDEX_NONE;

	UPROPERTY(Replicated)
	FText LastTelegraph;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Preloaded;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> IntroCamera;

	// Server state
	bool bAIEnabled = true;
	bool bIntroDone = false;
	bool bBodyApplied = false;
	double StateUntil = 0.0;
	double AttackStart = 0.0;
	double LastAttackEnd = -100.0;
	TArray<double> CooldownUntil;
	int32 SequenceCursor = 0;
	double NextSequenceAt = 0.0;
	int32 ChainRemaining = 0;
	TSet<int32> ResolvedWindows;
	bool bAttackLanded = false;
	// Charge
	bool bCharging = false;
	FVector ChargeDir = FVector::ForwardVector;
	double ChargeEnd = 0.0;
	double ChargeStartedAt = 0.0;
	// Stagger + phase tracking
	float TrackedHealth = -1.f;
	TArray<TPair<double, float>> RecentDamage;
	double LastMoveRequest = 0.0;
	float BaseWalkSpeed = 420.f;
	FName ForcedAttack;
};
