// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GOTAPlayerCharacter.generated.h"

/**
 * Native parent of BP_Player (previously ACharacter directly). It adds no gameplay of its own; it is the
 * server-side trust boundary for the Blueprint's client-to-server combat RPCs, which Blueprint cannot validate:
 *
 *  - SR_ParryAttack(AttackTarget): the client names the target of the parry riposte. Accepted only for a living
 *    combatant other than itself, within ParryRiposteRange, while this pawn is (or was within ParryGraceSeconds)
 *    blocking / inside its parry window on the SERVER, at most once per ParryRiposteCooldown.
 *  - SR_OnBlocked(CanbeParried, DamageCauser): accepted only for a living causer within BlockedCauserRange while
 *    the server sees this pawn blocking (or did within ParryGraceSeconds).
 *  - Attack / spell / block / parry requests are ignored while the pawn is dead.
 *
 * Rejected calls are dropped before the Blueprint runs and counted (gota.Net.Players shows the count).
 * The checks read only server state, so they hold whether the call came from a client or from the server itself.
 */
UCLASS()
class WAROFGODS_API AGOTAPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGOTAPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ProcessEvent(UFunction* Function, void* Parms) override;
	virtual void Tick(float DeltaSeconds) override;

	int32 GetRejectedRPCs() const { return RejectedRPCs; }

	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Net Validation")
	float ParryRiposteRange = 450.f;
	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Net Validation")
	float BlockedCauserRange = 900.f;
	/** Covers the client->server delay between the block on the server and the riposte request. */
	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Net Validation")
	float ParryGraceSeconds = 0.6f;
	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Net Validation")
	float ParryRiposteCooldown = 0.35f;
	/** How long a converted BP global slow-mo lasts as actor-local CustomTimeDilation. */
	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Presentation")
	float LocalParrySlowMoSeconds = 0.25f;

protected:
	/** False = drop the call. Only consulted on the server for SR_* functions. */
	bool ValidateServerRPC(const UFunction* Function, void* Parms);
	bool IsDefendingNow() const;
	static AActor* ReadActorParam(const UFunction* Function, void* Parms);
	/** BP_Player may call SetGlobalTimeDilation on parry; neutralize it in multiplayer. */
	void NeutralizeUnsafeGlobalSlowMo();

	double LastDefendingTime = -100.0;
	double LastRiposteTime = -100.0;
	int32 RejectedRPCs = 0;
	FTimerHandle LocalSlowMoHandle;
	bool bLoggedGlobalSlowMoClamp = false;
};
