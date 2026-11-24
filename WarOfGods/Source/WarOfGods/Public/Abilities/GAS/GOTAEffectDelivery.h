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
class UDecalComponent;

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
	/** The hit cuts the victim's action short. Zones never do: a repeating pulse must not become a stun lock. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") bool bInterrupts = true;
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
	bool HasExpired() const { return bExpired; }

	FGOTAEffectPayload Payload;
	TWeakObjectPtr<AActor> Source;

	UPROPERTY(Replicated) TObjectPtr<UParticleSystem> BodyFX;
	UPROPERTY(Replicated) TObjectPtr<UParticleSystem> ImpactFX;
	UPROPERTY(Replicated) float FXScale = 1.f;
	/** The payload's label, sent once for client logs (the payload itself stays on the server). */
	UPROPERTY(Replicated) FName ShownLabel;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Impact(FVector_NetQuantize Location);

	/** The source died or left: its deliveries end with it. */
	bool SourceIsGone() const;

	/** Milestone F3, drawing only: can this delivery hurt the player watching this screen? Decides red versus calm. */
	bool ThreatensLocalViewer() const;
	/** Milestone F3: a ground decal (the project's AOE indicator) that this machine alone draws. Null on a dedicated server. */
	UDecalComponent* AddGroundDecal(int32 SortOrder);

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
	UPROPERTY(Replicated) float CollisionRadius = 30.f;
	UPROPERTY(Replicated) float MaxDistance = 2000.f;

	/** Drawing: the impact was decided here, so the rock stops and its ground marks go (clients no longer fly past it). */
	void StopAt(const FVector& Location);

protected:
	virtual void BeginPlay() override;

private:
	/** F3: the hostile projectile's path on the ground, shrinking as it flies, and a mark under it. Local only. */
	void UpdateGroundMarks();

	UPROPERTY(Transient) TObjectPtr<UDecalComponent> Lane;
	UPROPERTY(Transient) TObjectPtr<UDecalComponent> Mark;
	FVector LaneEnd = FVector::ZeroVector;

	float Travelled = 0.f;
	/** Fighters who perfect-dodged this projectile: it flies on through them. */
	TArray<TWeakObjectPtr<const APawn>> DodgedBy;
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
	void StartZone(float InDuration, float InInterval, float InArmDelay = 0.f);

	UPROPERTY(Replicated) float Radius = 300.f;
	/**
	 * Seconds from spawn to the first pulse, sent once. Each screen counts it down from when the zone reached it, less half
	 * its ping; the server's reliable arm impact turns it red if that comes first. (The replicated server clock was measured
	 * 0.3 to 0.4 s off on a loaded client, which drew a burning zone as a warning.)
	 */
	UPROPERTY(Replicated) float ArmDelay = 0.f;

	/** The server's arm impact reached this screen. */
	void OnServerArmed();
	/** The payload's element, sent once so screens can draw the ground (flames for Fire). */
	UPROPERTY(Replicated) FGameplayTag GroundElement;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void Pulse();

	/** F3, local only: the edge goes from a flashing amber warning, filling in, to red when the first pulse lands. */
	void ShowArmed(const TCHAR* Why);
	/** A few small flames inside a burning Fire zone; they go with the zone. */
	void LightGround();

	UPROPERTY(Transient) TObjectPtr<UDecalComponent> Edge;
	UPROPERTY(Transient) TObjectPtr<UDecalComponent> Fill;
	/** Local clock (this machine's world time). */
	float WarnStartedAt = 0.f;
	float ArmsAt = 0.f;
	float ShownArmedAt = -1.f;
	bool bShowsThreat = false;
	bool bArmPending = false;

	FTimerHandle PulseTimer;
	double EndTime = 0.0;
	float Interval = 1.f;
	/** A warned zone: harmless for its arm delay, then the impact effect plays and the first pulse lands. */
	bool bAnnounceArming = false;
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
		float Interval, UParticleSystem* BodyFX = nullptr, float FXScale = 1.f, UParticleSystem* ImpactFX = nullptr, float ArmDelay = 0.f);
	/** Server: the hostile projectile that will reach Victim soonest, within MaxLead seconds (perfect dodge timing). */
	WAROFGODS_API AGOTAProjectile* FindIncomingProjectile(const APawn* Victim, float MaxLead, float& OutLead);
	/** Server: end every projectile and zone from Source (round reset, match transition). */
	WAROFGODS_API void ClearDeliveries(const UWorld* World, const AActor* Source);
}
