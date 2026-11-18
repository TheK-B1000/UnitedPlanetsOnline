// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/AnimInstance.h"
#include "GOTAMontageReplicator.generated.h"

class UAnimInstance;
class UAnimMontage;

/**
 * Makes an AI enemy's attack animations visible to remote players.
 *
 * The enemy template (BP_Enemy_Base / BPC_Attacks) plays every attack montage on the server only: no multicast. The
 * engine replicates root-motion montages to simulated proxies by itself, but these attacks are not root motion, so a
 * remote client saw enemies stand still while damage arrived (RC12: 0 of 6 attacking enemies showed a montage on every
 * client sample; only the listen host saw wind-ups). This server component listens to the enemy's anim instance and
 * multicasts each non-root-motion montage start (same montage, rate and position) and each interruption; clients play
 * it locally. Cosmetic only: damage and every gameplay decision stay on the server (the client copy's notifies have no
 * damage listeners). Bosses are skipped: UGOTABossComponent already multicasts its own animations.
 */
UCLASS(NotBlueprintable)
class WAROFGODS_API UGOTAMontageReplicator : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTAMontageReplicator();

	/** Server: attach to an AI pawn (no-op for players, bosses and pawns without a skeletal anim instance). */
	static UGOTAMontageReplicator* EnsureOn(APawn* Pawn);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayMontage(UAnimMontage* Montage, float PlayRate, float StartPosition);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopMontage(UAnimMontage* Montage, float BlendOutTime);

	UFUNCTION()
	void HandleMontageStarted(UAnimMontage* Montage);

	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/**
	 * Server: a damage notify (PlayMontageNotify) is firing. Bound when the enemy spawns, so it runs before the enemy
	 * Blueprint's own Play Montage listener (bound per montage) applies the damage: a player whose perfect dodge was
	 * timed against exactly this impact refuses it (UGOTADuelFighterComponent::TryGuardDodgedImpact).
	 */
	UFUNCTION()
	void HandleNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

	int32 GetSentCount() const { return Sent; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TWeakObjectPtr<UAnimInstance> Bound;
	int32 Sent = 0;
	UAnimInstance* GetAnimInstance() const;
	void Rebind();
	bool ShouldReplicate(const UAnimMontage* Montage) const;
};
