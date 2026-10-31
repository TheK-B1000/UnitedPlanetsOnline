// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/CombatTimingTypes.h"
#include "CombatTimingComponent.generated.h"

class UCombatComponent;
class UCombatTimingConfig;
class UStatsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatDefenseResolved, const FCombatDefenseResolveResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatGuardStateChanged, ECombatGuardState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDivineCounterStarted, AActor*, Target, float, MomentumSpent);

/**
 * Authoritative defense timing for Perfect Parry (04), Perfect Dodge (05), Divine Counter (06).
 * Blueprint still owns hit traces / damage apply — call ResolveIncomingAttack before applying damage.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API UCombatTimingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatTimingComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Timing")
	TObjectPtr<UCombatTimingConfig> TimingConfig;

	/** Fallback when no DataAsset assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Timing")
	TObjectPtr<UCombatTimingConfig> RuntimeFallbackConfig;

	UFUNCTION(BlueprintPure, Category = "Combat|Timing")
	ECombatGuardState GetGuardState() const { return GuardState; }

	UFUNCTION(BlueprintPure, Category = "Combat|Timing")
	bool IsInvulnerable() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Timing")
	bool IsInTimedParryWindow() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Timing")
	FVector GetLastDodgeDirection() const { return LastDodgeDirection; }

	UFUNCTION(BlueprintPure, Category = "Combat|Timing")
	const UCombatTimingConfig* GetActiveConfig() const;

	/** Press timed parry (also raises a short block if sustain is held). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Timing")
	void RequestParry();

	UFUNCTION(BlueprintCallable, Category = "Combat|Timing")
	void RequestBlock(bool bPressed);

	/** Directional dodge. WorldDirection is flattened on XY; zero uses actor forward. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Timing")
	void RequestDodge(FVector WorldDirection);

	/** Spend Divine Momentum for a counter window that auto-perfects the next eligible hit. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Timing")
	bool RequestDivineCounter(AActor* OptionalTarget = nullptr);

	/**
	 * Server-preferred resolve when an attack would connect.
	 * Returns whether BP should cancel damage for this AttackId.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Timing")
	FCombatDefenseResolveResult ResolveIncomingAttack(AActor* Attacker, FName AttackId, bool bUnparryable = false);

	UPROPERTY(BlueprintAssignable, Category = "Combat|Timing")
	FOnCombatDefenseResolved OnDefenseResolved;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Timing")
	FOnCombatGuardStateChanged OnGuardStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Timing")
	FOnDivineCounterStarted OnDivineCounterStarted;

protected:
	UFUNCTION(Server, Reliable)
	void Server_RequestParry(float ClientTimeSeconds);

	UFUNCTION(Server, Reliable)
	void Server_RequestBlock(bool bPressed);

	UFUNCTION(Server, Reliable)
	void Server_RequestDodge(FVector_NetQuantizeNormal WorldDirection, float ClientTimeSeconds);

	UFUNCTION(Server, Reliable)
	void Server_RequestDivineCounter(AActor* OptionalTarget);

	void BeginParryInternal(float ServerTimeSeconds);
	void BeginDodgeInternal(FVector WorldDirection, float ServerTimeSeconds);
	bool BeginDivineCounterInternal(AActor* OptionalTarget);

	void SetGuardState(ECombatGuardState NewState);
	void ClearTransientDefense();
	float NowSeconds() const;
	void GrantMomentum(float Amount);
	void BroadcastResolve(const FCombatDefenseResolveResult& Result);
	void NotifyCombatObservers(AActor* Attacker, FName AttackId, ECombatDefenseOutcome Outcome);

	UCombatComponent* FindCombat() const;
	UStatsComponent* FindStats() const;

	UPROPERTY(ReplicatedUsing = OnRep_GuardState, BlueprintReadOnly, Category = "Combat|Timing")
	ECombatGuardState GuardState = ECombatGuardState::Idle;

	UFUNCTION()
	void OnRep_GuardState();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat|Timing")
	FVector_NetQuantizeNormal LastDodgeDirection = FVector::ForwardVector;

	bool bBlockHeld = false;
	float ParryStartServerTime = -1.f;
	float DodgeStartServerTime = -1.f;
	float DivineCounterStartServerTime = -1.f;
	FName LastResolvedAttackId = NAME_None;

	FTimerHandle ParryExpireHandle;
	FTimerHandle DodgeExpireHandle;
	FTimerHandle DivineExpireHandle;
};
