// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "GOTAEnemyTargeting.generated.h"

class AAIController;
class UBlackboardComponent;

/**
 * Server-side target keeping for enemies built on AIC_Enemy_Base (the template's perception + blackboard state machine).
 *
 * The template only attacks what its sight sense has seen; a wave enemy that spawns out of sight range, or whose target
 * died, goes Passive and stands still for good (RC11: 7-9 stalled enemies per match, one match stuck for six minutes).
 * This component does not replace the behavior tree: twice a second it reads the controller's blackboard state and
 * target and, through the controller's own Blueprint functions, keeps it engaged:
 *
 *  - no living target       -> SetStateAsAttacking(nearest living player that is reachable and inside the leash)
 *  - target died / left      -> the same, with the next player
 *  - no progress for a while -> that target is marked unreachable for a while; try another one
 *  - nothing reachable       -> SetStateAsSeeking(home): walk back towards the spawn point and wait there
 *
 * Leash: a target is only chosen within EngageRadius of the enemy and LeashRadius of the enemy's home (spawn point),
 * so enemies do not chase across a whole map. Every decision is logged once (LogGOTATargeting), never per frame.
 */
UCLASS(NotBlueprintable)
class WAROFGODS_API UGOTAEnemyTargeting : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTAEnemyTargeting();

	/** Server: attach to an AI pawn whose controller has the template's SetStateAsAttacking / blackboard keys. */
	static UGOTAEnemyTargeting* EnsureOn(APawn* Pawn, float InEngageRadius = 6000.f, float InLeashRadius = 6000.f);

	/** Players farther than this from the enemy are not acquired. */
	float EngageRadius = 6000.f;
	/** Players farther than this from the enemy's home are not acquired (and a target that goes there is dropped). */
	float LeashRadius = 6000.f;
	/** Seconds without closing distance on a target (while not attacking, outside melee) before it counts as unreachable. */
	float PursuitTimeout = 8.f;
	/** How long an unreachable target is skipped. */
	float UnreachableSkipSeconds = 10.f;

	int32 GetAcquisitions() const { return Acquisitions; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Bound to the controller's ReceiveMoveCompleted: counts Blocked / OffPath / Invalid moves (Aborted is replanning). */
	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TWeakObjectPtr<AAIController> BoundController;
	TArray<double> RecentMoveFailures;
	/** While recovering from repeated path failures the enemy walks to a reachable point; no re-engage until then. */
	double RecoverUntil = -1.0;
	int32 Recoveries = 0;
	bool IsAttackMontagePlaying(const APawn* Self) const;
	/** Walk to the closest reachable point towards a player (end of a partial path) or home. */
	void Recover(AAIController* AIC, APawn* From, const TCHAR* Why, double Now);

	FVector Home = FVector::ZeroVector;
	FName StateKey;
	FName TargetKey;
	bool bKeysResolved = false;
	/** Last tick saw the enemy Stunned / Frozen (re-engage on release). */
	bool bWasIncapacitated = false;
	TWeakObjectPtr<APawn> LastTarget;
	float BestDistance = 0.f;
	double LastProgressTime = 0.0;
	double LastHomeOrder = -100.0;
	double NextSearchTime = 0.0;
	TMap<TWeakObjectPtr<APawn>, double> SkipUntil;
	int32 Acquisitions = 0;
	int32 PursuitFailures = 0;
	int32 HomeReturns = 0;

	bool ResolveKeys(AAIController* AIC);
	FString StateName(const UBlackboardComponent* BB) const;
	APawn* ChooseTarget(const APawn* Self, double Now) const;
	bool IsEligible(const APawn* Player, const APawn* Self, double Now) const;
	bool IsReachable(const APawn* Self, const APawn* Player) const;
	void Engage(AAIController* AIC, APawn* Target, const TCHAR* Why, double Now);
	void ReturnHome(AAIController* AIC, const TCHAR* Why, double Now);
};
