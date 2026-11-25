// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/AnimInstance.h"
#include "GOTAMontageReplicator.generated.h"

class UAnimInstance;
class UAnimMontage;

/**
 * Makes combatants' montages visible to remote players: AI enemies' attacks and, since 2026-11-25, player fighters'
 * combat montages (attacks, blocks, parries, hit reactions).
 *
 * The enemy template (BP_Enemy_Base / BPC_Attacks) plays every attack montage on the server only: no multicast. The
 * engine replicates root-motion montages to simulated proxies by itself, but these attacks are not root motion, so a
 * remote client saw enemies stand still while damage arrived (RC12: 0 of 6 attacking enemies showed a montage on every
 * client sample; only the listen host saw wind-ups). This server component listens to the enemy's anim instance and
 * multicasts each non-root-motion montage start (same montage, rate and position) and each interruption; clients play
 * it locally. Cosmetic only: damage and every gameplay decision stay on the server (the client copy's notifies have no
 * damage listeners). Bosses are skipped: UGOTABossComponent already multicasts its own animations.
 *
 * Players: the player Blueprint multicasts some of its own montages (MC_MeleeAttack, MC_StartBlock ...), but those
 * handlers act only in the matching weapon stance, and Stance is a non-replicated Blueprint variable set by one
 * unreliable multicast. A fighter equipped before a client could see it (every round-one Duel fighter, Horde spawns)
 * stayed unarmed on that client, so its attacks and blocks never played there; hit reactions are not multicast at all.
 * So for players this component also multicasts each montage start (a client already playing it skips it, so nothing
 * plays twice) and mirrors the server's Stance; a client whose copy differs runs the Blueprint's own MC_<stance> event
 * locally, as if that multicast had arrived. Presentation only: no damage, timing or input path changes.
 */
UCLASS(NotBlueprintable)
class WAROFGODS_API UGOTAMontageReplicator : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTAMontageReplicator();

	/** Server: attach to an AI pawn (no-op for players, bosses and pawns without a skeletal anim instance). */
	static UGOTAMontageReplicator* EnsureOn(APawn* Pawn);

	/** EventId: per-pawn sequence number, for matching server and client logs (MREP lines). */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayMontage(UAnimMontage* Montage, float PlayRate, float StartPosition, int32 EventId);

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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** The owner's Blueprint Stance (0 = unarmed), or -1 when it has none. */
	static int32 ReadStance(const AActor* Actor);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TWeakObjectPtr<UAnimInstance> Bound;
	int32 Sent = 0;
	UAnimInstance* GetAnimInstance() const;
	void Rebind();
	bool ShouldReplicate(const UAnimMontage* Montage) const;

	/** Server: the owner's Blueprint Stance, for clients whose own copy missed the stance multicast (-1 = none). */
	UPROPERTY(ReplicatedUsing = OnRep_MirroredStance)
	int32 MirroredStance = -1;
	UFUNCTION()
	void OnRep_MirroredStance();
	/** Client: bring the local Blueprint Stance to the server's through the Blueprint's own MC_<stance> event. */
	void SyncStanceLocally();
	int32 StanceAttempts = 0;
	/** Client: when the local stance first differed from the mirror (-1 = it matches). */
	double StanceMismatchSince = -1.0;
	/** Client: a difference must last this long before the tick corrects it (longer than any round trip). */
	static constexpr double StanceSettleSeconds = 1.0;
};
