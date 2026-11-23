// White Tiger Studios 2024

#include "Abilities/GAS/GOTAEffectDelivery.h"
#include "Abilities/GAS/GOTAAbilitySystemComponent.h"
#include "Abilities/GAS/GOTAStatusEffects.h"
#include "Abilities/GOTAHeroAbilityComponent.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAEffectDelivery)

DEFINE_LOG_CATEGORY_STATIC(LogGOTADelivery, Log, All);

namespace
{
	int32 TeamOf(const APawn* Pawn)
	{
		const FIntProperty* TeamProp = FindFProperty<FIntProperty>(Pawn->GetClass(), TEXT("TeamNumber"));
		return TeamProp ? TeamProp->GetPropertyValue_InContainer(Pawn) : 0;
	}

	void CapsuleOf(const APawn* Pawn, float& OutRadius, float& OutHalfHeight)
	{
		const ACharacter* Char = Cast<ACharacter>(Pawn);
		OutRadius = Char ? Char->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
		OutHalfHeight = Char ? Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	}

	/** World geometry only: other fighters never shield a target from a zone. */
	bool ClearLine(const UWorld* World, const FVector& From, const FVector& To, const AActor* Ignore)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GOTADeliveryLine), false, Ignore);
		return !World->LineTraceTestByObjectType(From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	}

	float OutgoingScale(const AActor* Source)
	{
		const UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(Source);
		return ASC ? ASC->GetOutgoingDamageMultiplier() : 1.f;
	}
}

// ============================================================================ shared rules

namespace GOTAGAS
{
	bool CanAffect(const AActor* Source, const APawn* Victim)
	{
		if (!Victim || Victim == Source || !UGOTACombatantObserver::FindDamageSystem(Victim) || UGOTACombatantObserver::IsCombatantDefeated(Victim))
		{
			return false;
		}
		if (const APawn* SourcePawn = Cast<APawn>(Source))
		{
			const bool bSourcePlayer = SourcePawn->IsPlayerControlled();
			const bool bVictimPlayer = Victim->IsPlayerControlled();
			if (bSourcePlayer && bVictimPlayer && TeamOf(SourcePawn) == TeamOf(Victim))
			{
				return false; // co-op allies; Duel PvP uses a different TeamNumber per slot
			}
			if (!bSourcePlayer && !bVictimPlayer)
			{
				return false; // enemy powers never hit other enemies
			}
		}
		const UGOTAHeroAbilityComponent* Abilities = Victim->FindComponentByClass<UGOTAHeroAbilityComponent>();
		return !(Abilities && Abilities->IsUntargetable());
	}

	bool DeliverEffect(APawn* Victim, AActor* Source, const FGOTAEffectPayload& Payload)
	{
		if (!Victim || !Victim->HasAuthority() || !CanAffect(Source, Victim))
		{
			return false;
		}
		if (const UGOTADuelFighterComponent* Fighter = Victim->FindComponentByClass<UGOTADuelFighterComponent>(); Fighter && Fighter->IsDodgeProtected())
		{
			UE_LOG(LogGOTADelivery, Log, TEXT("%s on %s: perfect-dodged"), *Payload.Label.ToString(), *GetNameSafe(Victim));
			return false;
		}
		bool bLanded;
		if (Payload.Damage > 0.f)
		{
			bLanded = DealElementalHit(Victim, Source, Payload.Damage, Payload.Element, Payload.bCanBeBlocked, Payload.HitResponse);
		}
		else
		{
			// Status only: full resistance to the element means nothing lands, as for a damaging hit.
			bLanded = !Payload.Element.IsValid() || GetElementDamageScale(Victim, Payload.Element) > 0.f;
			if (bLanded && Payload.Element.IsValid())
			{
				ApplyElementOnHit(Victim, Payload.Element, Source);
			}
		}
		if (bLanded && Payload.Status != EGOTAStatus::None && Payload.StatusDuration > 0.f && !UGOTACombatantObserver::IsCombatantDefeated(Victim))
		{
			UGOTAHeroAbilityComponent::ApplyStatusTo(Victim, Payload.Status, Payload.StatusDuration, Payload.StatusMagnitude, Source);
		}
		UE_LOG(LogGOTADelivery, Log, TEXT("%s reached %s (damage %.1f, landed=%d) from %s"), *Payload.Label.ToString(), *GetNameSafe(Victim), Payload.Damage,
			bLanded ? 1 : 0, *GetNameSafe(Source));
		return bLanded;
	}

	AGOTAProjectile* FireProjectile(AActor* Source, const FVector& From, const FVector& Direction, const FGOTAEffectPayload& Payload, float Speed,
		float CollisionRadius, float MaxDistance, UParticleSystem* BodyFX, UParticleSystem* ImpactFX, float FXScale)
	{
		UWorld* World = Source ? Source->GetWorld() : nullptr;
		const FVector Dir = Direction.GetSafeNormal();
		if (!World || !Source->HasAuthority() || Dir.IsNearlyZero() || Speed <= 0.f || MaxDistance <= 0.f)
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.Owner = Source;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.bDeferConstruction = true;
		AGOTAProjectile* Projectile = World->SpawnActor<AGOTAProjectile>(AGOTAProjectile::StaticClass(), FTransform(Dir.Rotation(), From), Params);
		if (!Projectile)
		{
			return nullptr;
		}
		Projectile->Source = Source;
		Projectile->Payload = Payload;
		Projectile->Payload.Damage *= OutgoingScale(Source);
		Projectile->StartLocation = From;
		Projectile->Velocity = Dir * Speed;
		Projectile->CollisionRadius = CollisionRadius;
		Projectile->MaxDistance = MaxDistance;
		Projectile->BodyFX = BodyFX;
		Projectile->ImpactFX = ImpactFX;
		Projectile->FXScale = FXScale;
		Projectile->FinishSpawning(FTransform(Dir.Rotation(), From));
		UE_LOG(LogGOTADelivery, Log, TEXT("PROJECTILE %s launched by %s: damage %.1f speed %.0f range %.0f"), *Payload.Label.ToString(), *GetNameSafe(Source),
			Projectile->Payload.Damage, Speed, MaxDistance);
		return Projectile;
	}

	AGOTAEffectZone* SpawnZone(AActor* Source, const FVector& Location, const FGOTAEffectPayload& Payload, float Radius, float Duration, float Interval,
		UParticleSystem* BodyFX, float FXScale)
	{
		UWorld* World = Source ? Source->GetWorld() : nullptr;
		if (!World || !Source->HasAuthority() || Radius <= 0.f || Duration <= 0.f || Interval <= 0.f)
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.Owner = Source;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.bDeferConstruction = true;
		AGOTAEffectZone* Zone = World->SpawnActor<AGOTAEffectZone>(AGOTAEffectZone::StaticClass(), FTransform(Location), Params);
		if (!Zone)
		{
			return nullptr;
		}
		Zone->Source = Source;
		Zone->Payload = Payload;
		Zone->Payload.Damage *= OutgoingScale(Source);
		Zone->Radius = Radius;
		Zone->BodyFX = BodyFX;
		Zone->FXScale = FXScale;
		Zone->FinishSpawning(FTransform(Location));
		Zone->StartZone(Duration, Interval);
		UE_LOG(LogGOTADelivery, Log, TEXT("ZONE %s started by %s: radius %.0f, %.1fs, every %.2fs, damage %.1f"), *Payload.Label.ToString(), *GetNameSafe(Source),
			Radius, Duration, Interval, Zone->Payload.Damage);
		return Zone;
	}

	void ClearDeliveries(const UWorld* World, const AActor* Source)
	{
		if (!World || !Source || !Source->HasAuthority())
		{
			return;
		}
		for (TActorIterator<AGOTAEffectDelivery> It(World); It; ++It)
		{
			if (It->Source.Get() == Source)
			{
				It->Expire(TEXT("cleared"));
			}
		}
	}
}

// ============================================================================ base

AGOTAEffectDelivery::AGOTAEffectDelivery()
{
	bReplicates = true;
	SetReplicatingMovement(false);
	bAlwaysRelevant = true;   // arenas are small and everyone must see the danger
	SetNetUpdateFrequency(5.f); // state is sent once; only spawn and teardown matter
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGOTAEffectDelivery::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGOTAEffectDelivery, BodyFX, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGOTAEffectDelivery, ImpactFX, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGOTAEffectDelivery, FXScale, COND_InitialOnly);
}

void AGOTAEffectDelivery::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		UE_LOG(LogGOTADelivery, Log, TEXT("DELIVERY client sees %s"), IsA<AGOTAProjectile>() ? TEXT("projectile") : TEXT("zone"));
	}
	if (BodyFX && GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::SpawnEmitterAttached(BodyFX, RootComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(FXScale),
			EAttachLocation::KeepRelativeOffset, true);
	}
}

void AGOTAEffectDelivery::EndPlay(const EEndPlayReason::Type Reason)
{
	if (!HasAuthority() && Reason == EEndPlayReason::Destroyed)
	{
		UE_LOG(LogGOTADelivery, Log, TEXT("DELIVERY client removed %s"), IsA<AGOTAProjectile>() ? TEXT("projectile") : TEXT("zone"));
	}
	Super::EndPlay(Reason);
}

void AGOTAEffectDelivery::Multicast_Impact_Implementation(FVector_NetQuantize Location)
{
	if (!HasAuthority())
	{
		UE_LOG(LogGOTADelivery, Log, TEXT("DELIVERY client impact"));
	}
	if (ImpactFX && GetNetMode() != NM_DedicatedServer)
	{
		const FVector Where = Location;
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactFX.Get(), FTransform(FRotator::ZeroRotator, Where, FVector(FXScale)));
	}
}

bool AGOTAEffectDelivery::SourceIsGone() const
{
	const AActor* S = Source.Get();
	return !S || UGOTACombatantObserver::IsCombatantDefeated(S);
}

void AGOTAEffectDelivery::Expire(const TCHAR* Why)
{
	if (bExpired || !HasAuthority())
	{
		return;
	}
	bExpired = true;
	SetActorTickEnabled(false);
	GetWorldTimerManager().ClearAllTimersForObject(this);
	UE_LOG(LogGOTADelivery, Log, TEXT("%s %s ended (%s)"), IsA<AGOTAProjectile>() ? TEXT("PROJECTILE") : TEXT("ZONE"), *Payload.Label.ToString(), Why);
	SetLifeSpan(0.25f); // lets the reliable impact reach clients before the channel closes
}

// ============================================================================ projectile

AGOTAProjectile::AGOTAProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AGOTAProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGOTAProjectile, StartLocation, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGOTAProjectile, Velocity, COND_InitialOnly);
}

void AGOTAProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Speed = Velocity.Size();
	if (Speed <= 0.f)
	{
		return;
	}
	const FVector Dir = FVector(Velocity) / Speed;
	const float Step = FMath::Min(Speed * DeltaSeconds, MaxDistance - Travelled);
	const FVector From = FVector(StartLocation) + Dir * Travelled;
	FVector To = From + Dir * Step;

	if (!HasAuthority())
	{
		// Cosmetic replay of the server's line; it stops when the server's actor goes away.
		Travelled += Step;
		SetActorLocation(To);
		return;
	}
	if (SourceIsGone())
	{
		Expire(TEXT("source gone"));
		return;
	}

	// World first: nothing behind a wall can be hit.
	UWorld* World = GetWorld();
	FHitResult WallHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GOTAProjectile), false, this);
	if (const AActor* S = Source.Get())
	{
		Params.AddIgnoredActor(S);
	}
	const bool bWall = World->SweepSingleByObjectType(WallHit, From, To, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
		FCollisionShape::MakeSphere(CollisionRadius), Params);
	const float PathLength = bWall ? WallHit.Distance : Step;
	const FVector PathEnd = From + Dir * PathLength;

	// Earliest combatant along the path.
	APawn* Best = nullptr;
	float BestAlong = TNumericLimits<float>::Max();
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* P = *It;
		if (!GOTAGAS::CanAffect(Source.Get(), P))
		{
			continue;
		}
		float CapRadius, CapHalf;
		CapsuleOf(P, CapRadius, CapHalf);
		const FVector Centre = P->GetActorLocation();
		const FVector Closest = FMath::ClosestPointOnSegment(Centre, From, PathEnd);
		if (FVector::Dist2D(Closest, Centre) <= CollisionRadius + CapRadius && FMath::Abs(Closest.Z - Centre.Z) <= CollisionRadius + CapHalf)
		{
			const float Along = FVector::DotProduct(Closest - From, Dir);
			if (Along < BestAlong)
			{
				BestAlong = Along;
				Best = P;
			}
		}
	}
	if (Best)
	{
		const FVector HitAt = From + Dir * BestAlong;
		SetActorLocation(HitAt);
		const bool bLanded = GOTAGAS::DeliverEffect(Best, Source.Get(), Payload);
		UE_LOG(LogGOTADelivery, Log, TEXT("PROJECTILE %s hit %s after %.0f (landed=%d)"), *Payload.Label.ToString(), *GetNameSafe(Best), Travelled + BestAlong, bLanded ? 1 : 0);
		Multicast_Impact(HitAt);
		Expire(TEXT("hit"));
		return;
	}
	if (bWall)
	{
		SetActorLocation(PathEnd);
		UE_LOG(LogGOTADelivery, Log, TEXT("PROJECTILE %s stopped by %s after %.0f"), *Payload.Label.ToString(), *GetNameSafe(WallHit.GetActor()), Travelled + PathLength);
		Multicast_Impact(PathEnd);
		Expire(TEXT("world"));
		return;
	}
	Travelled += Step;
	SetActorLocation(To);
	if (Travelled >= MaxDistance - KINDA_SMALL_NUMBER)
	{
		Expire(TEXT("max range"));
	}
}

// ============================================================================ zone

AGOTAEffectZone::AGOTAEffectZone()
{
	PrimaryActorTick.bCanEverTick = false; // a server timer at the zone's interval; nothing per frame
}

void AGOTAEffectZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGOTAEffectZone, Radius, COND_InitialOnly);
}

void AGOTAEffectZone::StartZone(float InDuration, float InInterval)
{
	if (!HasAuthority())
	{
		return;
	}
	Interval = InInterval;
	EndTime = GetWorld()->GetTimeSeconds() + InDuration;
	GetWorldTimerManager().SetTimer(PulseTimer, this, &AGOTAEffectZone::Pulse, Interval, true);
}

void AGOTAEffectZone::Pulse()
{
	if (SourceIsGone())
	{
		Expire(TEXT("source gone"));
		return;
	}
	UWorld* World = GetWorld();
	const FVector Centre = GetActorLocation();
	int32 Reached = 0;
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* P = *It;
		if (!GOTAGAS::CanAffect(Source.Get(), P))
		{
			continue;
		}
		float CapRadius, CapHalf;
		CapsuleOf(P, CapRadius, CapHalf);
		const FVector Where = P->GetActorLocation();
		if (FVector::Dist2D(Where, Centre) > Radius + CapRadius || FMath::Abs(Where.Z - Centre.Z) > 250.f || !ClearLine(World, Centre, Where, P))
		{
			continue;
		}
		Reached += GOTAGAS::DeliverEffect(P, Source.Get(), Payload) ? 1 : 0;
	}
	UE_LOG(LogGOTADelivery, Log, TEXT("ZONE %s pulse: %d target(s)"), *Payload.Label.ToString(), Reached);
	if (World->GetTimeSeconds() + KINDA_SMALL_NUMBER >= EndTime)
	{
		Expire(TEXT("duration"));
	}
}
