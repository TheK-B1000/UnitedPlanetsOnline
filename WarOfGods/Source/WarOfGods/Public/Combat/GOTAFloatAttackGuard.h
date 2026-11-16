// White Tiger Studios 2024
//
// Server-side cleanup for the Story shrine boss's floating AOE Trail attack (BP_Enemy_Boss / BT_Enemy_Boss).
//
// The attack is a behavior-tree sequence: BTT_Boss_StartFloat (the Blueprint's StartFloat event raises the boss on
// its floating platform and sets BPC_DamageSystem.IsInvincible), the AOE Trail attack, then BTT_Boss_StopFloat (lands
// and clears IsInvincible). If the sequence is aborted between the two (the target dies or leaves, the tree switches
// branch, the brain stops), StopFloat never runs and the boss stays airborne and invincible for good.
//
// This guard watches the behavior tree. Once StartFloat has run, it remembers the sequence it belongs to; if the active
// node leaves that sequence, or the pawn dies or ends play, before StopFloat runs, it calls the Blueprint's own StopFloat.
// The attack and its intended temporary invulnerability are unchanged.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GOTAFloatAttackGuard.generated.h"

class UBTCompositeNode;
class UBehaviorTreeComponent;

UCLASS(ClassGroup = (GOTA))
class WAROFGODS_API UGOTAFloatAttackGuard : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTAFloatAttackGuard();

	/** Server: add the guard to a pawn whose Blueprint has the StartFloat / StopFloat events. Null otherwise. */
	static UGOTAFloatAttackGuard* EnsureOn(APawn* Pawn);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool IsFloatPending() const { return FloatSequence.IsValid(); }

protected:
	UBehaviorTreeComponent* GetBrain() const;
	/** Calls the Blueprint StopFloat event once and forgets the pending float. */
	void Land(const TCHAR* Why);

	/** The sequence that ran StartFloat, while its StopFloat has not run yet. */
	TWeakObjectPtr<const UBTCompositeNode> FloatSequence;
	double FloatStartTime = 0.0;
};
