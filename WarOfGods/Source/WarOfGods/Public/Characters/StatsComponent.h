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

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void OnRep_Stamina(float OldStamina);

	UFUNCTION()
	void OnRep_DivineMomentum(float OldValue);

	bool HasAuthorityToMutate() const;
};
