// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Events/CombatEvents.h"
#include "StatsComponent.generated.h"

/**
 * Core combat resources. Server should own mutations; replication hooks land in Phase 3.
 * Does not replace BPC_DamageSystem — coexist until migration.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API UStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatsComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetStamina() const { return Stamina; }

	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetDivineMomentum() const { return DivineMomentum; }

	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	float GetMaxDivineMomentum() const { return MaxDivineMomentum; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void SetHealth(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	bool ConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void RestoreStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void AddDivineMomentum(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	bool ConsumeDivineMomentum(float Amount);

	/**
	 * When false (Phase 1 default via bootstrap), mutation APIs no-op so BPC_DamageSystem
	 * remains the health authority. DivineMomentum / Stamina helpers can still be enabled later.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void SetAuthoritativeMutationsEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	bool AreAuthoritativeMutationsEnabled() const { return bAuthoritativeMutations; }

	/** Mirror BP-owned health into replicated display without claiming authority. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void ObserveHealth(float Current, float Maximum);

	/**
	 * Server: mirror the owner's Blueprint damage system (BPC_DamageSystem, the health authority) into this
	 * replicated component - health and death for every client, plus the last damager for kill credit.
	 * Event-driven (OnDamageResponse / OnDeath) with a 10 Hz check for heals and respawns. Idempotent.
	 */
	void BeginObservingBlueprintDamage();
	/** Server: re-read the Blueprint health now. */
	void SyncFromBlueprint();
	/** True once the server has mirrored real health into this component (clients: replicated). */
	bool HasObservedHealth() const { return bHealthObserved; }
	/** Server: the pawn (or its controller's pawn) that last damaged the owner, if within Seconds. */
	APawn* GetRecentDamager(float Seconds = 10.f) const;
	/** Server: record who is about to damage the owner (C++ damage path; Blueprint hits arrive via OnDamageResponse). */
	void NoteDamageFrom(AActor* DamageCauser);

	/** Bound to BPC_DamageSystem.OnDamageResponse(DamageResponse, DamageCauser) on the server. */
	UFUNCTION()
	void HandleBlueprintDamageResponse(uint8 DamageResponse, AActor* DamageCauser);
	/** Bound to BPC_DamageSystem.OnDeath() on the server. */
	UFUNCTION()
	void HandleBlueprintDeath();

	UPROPERTY(BlueprintAssignable, Category = "Combat|Stats")
	FOnStatChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Stats")
	FOnStatChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Stats")
	FOnStatChanged OnDivineMomentumChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Stats")
	FOnCombatActorEvent OnDied;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Combat|Stats")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Combat|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Combat|Stats")
	float Stamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Combat|Stats")
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_DivineMomentum, Category = "Combat|Stats")
	float DivineMomentum = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Combat|Stats")
	float MaxDivineMomentum = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Stats")
	bool bAuthoritativeMutations = false;

	UPROPERTY(Replicated)
	bool bHealthObserved = false;

	TWeakObjectPtr<UActorComponent> ObservedDamageSystem;
	TWeakObjectPtr<APawn> LastDamager;
	double LastDamageTime = -1000.0;
	FTimerHandle SyncTimer;
	/** One death event per life (cleared when health comes back). */
	bool bDeathBroadcast = false;
	void BroadcastDeath();

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void OnRep_Stamina(float OldStamina);

	UFUNCTION()
	void OnRep_DivineMomentum(float OldValue);

	bool HasAuthorityToMutate() const;
};
