// White Tiger Studios 2024
//
// GAS milestone E: how an effect reaches its targets away from the caster. A projectile or a zone decides WHO is hit, on
// the server; the hit itself goes through GOTAGAS::DealElementalHit and ApplyStatusTo, so health stays with the victim's
// BPC_DamageSystem and immunity, resistance, shields and attribution follow the same rules as every other hit.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "GOTAEffectDelivery.generated.h"

class UParticleSystem;

/** What a delivery does to each target it reaches. Data only, shared by heroes, enemies and tests. */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAEffectPayload
{
	GENERATED_BODY()

	/** Name used in logs (the ability or attack). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") FName Label;
	/** 0 = status only (a zone that keeps its element's status on whoever stands in it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") float Damage = 0.f;
	/** Scales the damage by resistance and applies the element's status and interactions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (Categories = "GOTA.Element")) FGameplayTag Element;
	/** An extra status on each target the effect lands on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") EGOTAStatus Status = EGOTAStatus::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") float StatusDuration = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") float StatusMagnitude = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") bool bCanBeBlocked = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") FString HitResponse = TEXT("Stagger");
};

/** Shared base: source, payload and cleanup. Server decides everything; clients only draw. */
UCLASS(Abstract, NotBlueprintable)
class WAROFGODS_API AGOTAEffectDelivery : public AActor
{
	GENERATED_BODY()

public:
	AGOTAEffectDelivery();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: stop now, deal nothing more. Clients keep the actor just long enough to receive the last impact. */
	void Expire(const TCHAR* Why);

	FGOTAEffectPayload Payload;
	TWeakObjectPtr<AActor> Source;

	UPROPERTY(Replicated) TObjectPtr<UParticleSystem> BodyFX;
	UPROPERTY(Replicated) TObjectPtr<UParticleSystem> ImpactFX;
	UPROPERTY(Replicated) float FXScale = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Impact(FVector_NetQuantize Location);

	/** The source died or left: its deliveries end with it. */
	bool SourceIsGone() const;

	bool bExpired = false;
};

/** Straight-line projectile. The server sweeps it every frame; clients replay the same line from its initial state. */
UCLASS(NotBlueprintable)
class WAROFGODS_API AGOTAProjectile : public AGOTAEffectDelivery
{
	GENERATED_BODY()

public:
	AGOTAProjectile();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated) FVector_NetQuantize StartLocation;
	UPROPERTY(Replicated) FVector_NetQuantize10 Velocity;
	float CollisionRadius = 30.f;
	float MaxDistance = 2000.f;

private:
	float Travelled = 0.f;
};

/** Persistent area: applies the payload to everyone inside on a fixed server interval, then ends. */
UCLASS(NotBlueprintable)
class WAROFGODS_API AGOTAEffectZone : public AGOTAEffectDelivery
{
	GENERATED_BODY()

public:
	AGOTAEffectZone();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server, once after spawning. */
	void StartZone(float InDuration, float InInterval);

	UPROPERTY(Replicated) float Radius = 300.f;

private:
	void Pulse();

	FTimerHandle PulseTimer;
	double EndTime = 0.0;
	float Interval = 1.f;
};

namespace GOTAGAS
{
	/** May Source's effects touch Victim? A living combatant, not the source, not an ally, not untargetable. */
	WAROFGODS_API bool CanAffect(const AActor* Source, const APawn* Victim);
	/** Server: apply Payload to one target. True if it landed. A perfect dodge, a block or full resistance carries no status. */
	WAROFGODS_API bool DeliverEffect(APawn* Victim, AActor* Source, const FGOTAEffectPayload& Payload);
	/** Server: fire a projectile. The source's outgoing damage modifiers are taken at launch. */
	WAROFGODS_API AGOTAProjectile* FireProjectile(AActor* Source, const FVector& From, const FVector& Direction, const FGOTAEffectPayload& Payload,
		float Speed, float CollisionRadius, float MaxDistance, UParticleSystem* BodyFX = nullptr, UParticleSystem* ImpactFX = nullptr, float FXScale = 1.f);
	/** Server: start a zone at Location. The source's outgoing damage modifiers are taken when it starts. */
	WAROFGODS_API AGOTAEffectZone* SpawnZone(AActor* Source, const FVector& Location, const FGOTAEffectPayload& Payload, float Radius, float Duration,
		float Interval, UParticleSystem* BodyFX = nullptr, float FXScale = 1.f);
	/** Server: end every projectile and zone from Source (round reset, match transition). */
	WAROFGODS_API void ClearDeliveries(const UWorld* World, const AActor* Source);
}
