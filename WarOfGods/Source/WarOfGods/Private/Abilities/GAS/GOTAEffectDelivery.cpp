// White Tiger Studios 2024

#include "Abilities/GAS/GOTAEffectDelivery.h"
#include "Abilities/GAS/GOTAAbilitySystemComponent.h"
#include "Abilities/GAS/GOTAStatusEffects.h"
#include "Abilities/GOTAHeroAbilityComponent.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Combat/GOTACombatLive.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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

	// ---------------------------------------------------------------- F3 ground marks (drawing only)
	// One language on every screen: amber = about to hurt you, red = hurting you now, calm blue = yours or an ally's.
	// The project's own AOE indicator decal (already used by the Blueprint mage and boss).
	const TCHAR* GroundDecalPath = TEXT("/Game/AOE/Decals/Decal_AOE.Decal_AOE");
	const TCHAR* GroundFlamePath = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Small.P_Fire_Small"); // Burning's own flames
	// Emission stays low: brighter values bloom to pale yellow on the sandy arena floors and lose the colour.
	// Speed spins the indicator: fast while warning, slow for a friendly area, still for lanes and fills.
	struct FGroundLook { FLinearColor Color; float Emission; float Spin; };
	// The burning state must read stronger than the warning, so the warning is the dimmer of the two.
	const FGroundLook WarnLook{FLinearColor(0.55f, 0.18f, 0.f), 0.5f, 1.6f};
	const FGroundLook WarnFillLook{FLinearColor(0.55f, 0.18f, 0.f), 0.3f, 0.f};
	const FGroundLook DangerLook{FLinearColor(1.f, 0.f, 0.f), 2.4f, 0.4f};
	const FGroundLook FriendlyLook{FLinearColor(0.1f, 0.45f, 1.f), 0.35f, 0.15f};
	const FGroundLook LaneLook{FLinearColor(1.f, 0.f, 0.f), 1.2f, 0.f};
	const FGroundLook LaneMarkLook{FLinearColor(1.f, 0.f, 0.f), 2.4f, 0.f};
	constexpr float DecalDepth = 260.f;   // reaches the floor from a capsule centre or a projectile's flight height
	constexpr float LaneHalfWidth = 85.f;  // the boulder's radius plus a fighter's: outside it, a sidestep is enough

	/** Half this machine's round trip to the server, in seconds (0 on the server). */
	float HalfPing(const UWorld* World)
	{
		const APlayerController* PC = World && World->GetNetMode() == NM_Client ? World->GetFirstPlayerController() : nullptr;
		const APlayerState* PS = PC ? PC->PlayerState : nullptr;
		return PS ? FMath::Clamp(PS->ExactPing * 0.0005f, 0.f, 0.5f) : 0.f;
	}

	void SetDecalLook(UDecalComponent* Decal, const FGroundLook& Look)
	{
		if (UMaterialInstanceDynamic* MID = Decal ? Cast<UMaterialInstanceDynamic>(Decal->GetDecalMaterial()) : nullptr)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Look.Color);
			MID->SetScalarParameterValue(TEXT("Emission"), Look.Emission);
			MID->SetScalarParameterValue(TEXT("Speed"), Look.Spin);
		}
	}

	/** Lays a decal flat on the floor: centred at Centre, HalfAlong along Dir, HalfAcross across it. */
	void PlaceOnGround(UDecalComponent* Decal, const FVector& Centre, const FVector& Dir, float HalfAlong, float HalfAcross)
	{
		const FVector Along = Dir.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : Dir.GetSafeNormal2D();
		Decal->SetWorldLocationAndRotation(Centre, FRotationMatrix::MakeFromXZ(-FVector::UpVector, Along).Rotator());
		Decal->DecalSize = FVector(DecalDepth, HalfAcross, HalfAlong);
		Decal->MarkRenderStateDirty();
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
		bool bLanded;
		if (Payload.Damage > 0.f)
		{
			bLanded = DealElementalHit(Victim, Source, Payload.Damage, Payload.Element, Payload.bCanBeBlocked, Payload.HitResponse, Payload.bInterrupts);
		}
		else
		{
			// Status only: full resistance to the element means nothing lands, as for a damaging hit.
			// So do invincibility frames.
			const UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Victim);
			bLanded = !(DS && FGOTABlueprintCombat::GetBool(DS, TEXT("IsInvincible")))
				&& (!Payload.Element.IsValid() || GetElementDamageScale(Victim, Payload.Element) > 0.f);
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
		Projectile->ShownLabel = Payload.Label;
		Projectile->BodyFX = BodyFX;
		Projectile->ImpactFX = ImpactFX;
		Projectile->FXScale = FXScale;
		Projectile->FinishSpawning(FTransform(Dir.Rotation(), From));
		UE_LOG(LogGOTADelivery, Log, TEXT("PROJECTILE %s launched by %s: damage %.1f speed %.0f range %.0f"), *Payload.Label.ToString(), *GetNameSafe(Source),
			Projectile->Payload.Damage, Speed, MaxDistance);
		return Projectile;
	}

	AGOTAEffectZone* SpawnZone(AActor* Source, const FVector& Location, const FGOTAEffectPayload& Payload, float Radius, float Duration, float Interval,
		UParticleSystem* BodyFX, float FXScale, UParticleSystem* ImpactFX, float ArmDelay)
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
		Zone->Payload.bInterrupts = false;
		Zone->Radius = Radius;
		Zone->ArmDelay = FMath::Max(ArmDelay, 0.f);
		Zone->GroundElement = Payload.Element;
		Zone->ShownLabel = Payload.Label;
		Zone->BodyFX = BodyFX;
		Zone->ImpactFX = ImpactFX;
		Zone->FXScale = FXScale;
		Zone->FinishSpawning(FTransform(Location));
		Zone->StartZone(Duration, Interval, ArmDelay);
		UE_LOG(LogGOTADelivery, Log, TEXT("ZONE %s started by %s: radius %.0f, %.1fs, every %.2fs, damage %.1f, arms in %.1fs"), *Payload.Label.ToString(), *GetNameSafe(Source),
			Radius, Duration, Interval, Zone->Payload.Damage, ArmDelay);
		return Zone;
	}

	AGOTAProjectile* FindIncomingProjectile(const APawn* Victim, float MaxLead, float& OutLead)
	{
		const UWorld* World = Victim ? Victim->GetWorld() : nullptr;
		if (!World)
		{
			return nullptr;
		}
		float CapRadius, CapHalf;
		CapsuleOf(Victim, CapRadius, CapHalf);
		const FVector Centre = Victim->GetActorLocation();
		AGOTAProjectile* Best = nullptr;
		OutLead = MaxLead;
		for (TActorIterator<AGOTAProjectile> It(World); It; ++It)
		{
			AGOTAProjectile* P = *It;
			const FVector V = P->Velocity;
			const float Speed = V.Size();
			if (P->HasExpired() || Speed <= 0.f || !CanAffect(P->Source.Get(), Victim))
			{
				continue;
			}
			const FVector Dir = V / Speed;
			const FVector From = P->GetActorLocation();
			const float Along = FVector::DotProduct(Centre - From, Dir);
			const FVector Closest = From + Dir * FMath::Max(Along, 0.f);
			if (Along < -CapRadius || FVector::Dist2D(Closest, Centre) > P->CollisionRadius + CapRadius
				|| FMath::Abs(Closest.Z - Centre.Z) > P->CollisionRadius + CapHalf)
			{
				continue;
			}
			const float Lead = FMath::Max(Along - P->CollisionRadius - CapRadius, 0.f) / Speed;
			if (Lead <= OutLead)
			{
				OutLead = Lead;
				Best = P;
			}
		}
		return Best;
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
	DOREPLIFETIME_CONDITION(AGOTAEffectDelivery, ShownLabel, COND_InitialOnly);
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
	if (AGOTAProjectile* Projectile = Cast<AGOTAProjectile>(this))
	{
		Projectile->StopAt(Location);
	}
	else if (AGOTAEffectZone* Zone = Cast<AGOTAEffectZone>(this))
	{
		Zone->OnServerArmed(); // a zone's only impact is its arming
	}
	if (ImpactFX && GetNetMode() != NM_DedicatedServer)
	{
		const FVector Where = Location;
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactFX.Get(), FTransform(FRotator::ZeroRotator, Where, FVector(FXScale)));
	}
}

bool AGOTAEffectDelivery::ThreatensLocalViewer() const
{
	// Mirrors GOTAGAS::CanAffect's sides without its team property, which only the server knows: enemies and bosses
	// threaten every player; another player threatens you only in a Duel; your own delivery never does.
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Me = PC ? PC->GetPawn() : nullptr;
	const APawn* From = Cast<APawn>(GetOwner());
	if (!From)
	{
		return true; // source unknown or gone: show the danger
	}
	if (From == Me)
	{
		return false;
	}
	return !From->IsPlayerControlled() || World->GetGameState<ADuelGameState>() != nullptr;
}

UDecalComponent* AGOTAEffectDelivery::AddGroundDecal(int32 SortOrder)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	static TWeakObjectPtr<UMaterialInterface> Base;
	if (!Base.IsValid())
	{
		Base = LoadObject<UMaterialInterface>(nullptr, GroundDecalPath);
	}
	if (!Base.IsValid())
	{
		UE_LOG(LogGOTADelivery, Warning, TEXT("DELIVERY ground mark missing asset %s"), GroundDecalPath);
		return nullptr;
	}
	UDecalComponent* Decal = NewObject<UDecalComponent>(this);
	Decal->SetUsingAbsoluteLocation(true);
	Decal->SetUsingAbsoluteRotation(true);
	Decal->SetUsingAbsoluteScale(true);
	Decal->SetupAttachment(RootComponent);
	Decal->SetDecalMaterial(UMaterialInstanceDynamic::Create(Base.Get(), Decal));
	Decal->SortOrder = SortOrder;
	Decal->SetFadeScreenSize(0.f); // the whole arena must read it, near or far
	Decal->RegisterComponent();
	return Decal;
}

bool AGOTAEffectDelivery::SourceIsGone() const
{
	const AActor* S = Source.Get();
	return !S || UGOTACombatantObserver::IsCombatantDefeated(S) || !GOTACombatLive::IsCombatLive(GetWorld());
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
	DOREPLIFETIME_CONDITION(AGOTAProjectile, CollisionRadius, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGOTAProjectile, MaxDistance, COND_InitialOnly);
}

void AGOTAProjectile::BeginPlay()
{
	Super::BeginPlay();
	const FVector Dir = FVector(Velocity).GetSafeNormal();
	if (GetNetMode() == NM_DedicatedServer || Dir.IsNearlyZero() || !ThreatensLocalViewer())
	{
		return; // only a projectile that can hit you gets a lane; your own or an ally's adds no clutter
	}
	// Where it will stop: the same world sweep the server makes, once, on this machine.
	const FVector From = StartLocation;
	LaneEnd = From + Dir * MaxDistance;
	FHitResult WallHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GOTAProjectileLane), false, this);
	Params.AddIgnoredActor(GetOwner());
	if (GetWorld()->SweepSingleByObjectType(WallHit, From, LaneEnd, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
		FCollisionShape::MakeSphere(CollisionRadius), Params))
	{
		LaneEnd = From + Dir * WallHit.Distance;
	}
	Lane = AddGroundDecal(1);
	Mark = AddGroundDecal(2);
	SetDecalLook(Lane, LaneLook);
	SetDecalLook(Mark, LaneMarkLook);
	UE_LOG(LogGOTADelivery, Log, TEXT("PROJECTILE MARK %s threat=1 lane=%.0f"), *ShownLabel.ToString(), FVector::Dist(From, LaneEnd));
	UpdateGroundMarks();
}

void AGOTAProjectile::UpdateGroundMarks()
{
	if (!Lane && !Mark)
	{
		return;
	}
	const FVector Here = GetActorLocation();
	const FVector Dir = FVector(Velocity).GetSafeNormal();
	const float Left = FVector::DotProduct(LaneEnd - Here, Dir);
	if (Lane)
	{
		Lane->SetVisibility(Left > 60.f);
		if (Left > 60.f)
		{
			PlaceOnGround(Lane, Here + Dir * (Left * 0.5f), Dir, Left * 0.5f, LaneHalfWidth);
		}
	}
	if (Mark)
	{
		PlaceOnGround(Mark, Here, Dir, CollisionRadius + 25.f, CollisionRadius + 25.f);
	}
}

void AGOTAProjectile::StopAt(const FVector& Location)
{
	for (UDecalComponent* Decal : { Lane.Get(), Mark.Get() })
	{
		if (Decal)
		{
			Decal->DestroyComponent();
		}
	}
	Lane = nullptr;
	Mark = nullptr;
	if (!HasAuthority())
	{
		SetActorLocation(Location);
		Velocity = FVector::ZeroVector; // cosmetic copy only; the server's projectile has already ended
	}
}

void AGOTAProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateGroundMarks();
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
		Expire(TEXT("source gone or round over"));
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
		if (!GOTAGAS::CanAffect(Source.Get(), P) || DodgedBy.Contains(P))
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
	if (UGOTADuelFighterComponent* Fighter = Best ? Best->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr; Fighter && Fighter->ConsumeDodgedDelivery(this))
	{
		// That fighter's perfect dodge covers this projectile only; it flies on and can still hit someone behind them.
		DodgedBy.Add(Best);
		UE_LOG(LogGOTADelivery, Log, TEXT("PROJECTILE %s perfect-dodged by %s"), *Payload.Label.ToString(), *GetNameSafe(Best));
		Best = nullptr;
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
	// The rules run on a server timer at the zone's interval. Ticking is only ever switched on to draw a warning fill
	// on a screen, for the arm delay.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AGOTAEffectZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGOTAEffectZone, Radius, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGOTAEffectZone, ArmDelay, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AGOTAEffectZone, GroundElement, COND_InitialOnly);
}

void AGOTAEffectZone::LightGround()
{
	if (GetNetMode() == NM_DedicatedServer || !GroundElement.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("GOTA.Element.Fire"), false)))
	{
		return;
	}
	static TWeakObjectPtr<UParticleSystem> Flame;
	if (!Flame.IsValid())
	{
		Flame = LoadObject<UParticleSystem>(nullptr, GroundFlamePath);
	}
	if (!Flame.IsValid())
	{
		UE_LOG(LogGOTADelivery, Warning, TEXT("DELIVERY ground flames missing asset %s"), GroundFlamePath);
		return;
	}
	// A ring inside the edge plus the centre: enough to read "this ground burns" without hiding fighters.
	const int32 Ring = FMath::Clamp(FMath::RoundToInt(Radius / 90.f), 3, 6);
	const FVector Centre = GetActorLocation() - FVector(0.f, 0.f, 85.f);
	for (int32 i = 0; i <= Ring; ++i)
	{
		const FVector Offset = i == 0 ? FVector::ZeroVector : FRotator(0.f, 360.f * i / Ring + 20.f, 0.f).Vector() * Radius * 0.6f;
		UGameplayStatics::SpawnEmitterAttached(Flame.Get(), RootComponent, NAME_None, Centre + Offset, FRotator::ZeroRotator, FVector(0.7f),
			EAttachLocation::KeepWorldPosition, true);
	}
	UE_LOG(LogGOTADelivery, Log, TEXT("ZONE MARK %s flames %d"), *ShownLabel.ToString(), Ring + 1);
}

void AGOTAEffectZone::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	bShowsThreat = ThreatensLocalViewer();
	// The zone reached this screen about half a round trip after the server spawned it.
	const float WarnLeft = ArmDelay - HalfPing(GetWorld());
	bArmPending = WarnLeft > 0.f;
	// The edge is the exact radius the server tests (a fighter is caught once their capsule overlaps it).
	Edge = AddGroundDecal(1);
	SetDecalLook(Edge, !bShowsThreat ? FriendlyLook : bArmPending ? WarnLook : DangerLook);
	if (Edge)
	{
		PlaceOnGround(Edge, GetActorLocation(), FVector::ForwardVector, Radius, Radius);
	}
	UE_LOG(LogGOTADelivery, Log, TEXT("ZONE MARK %s threat=%d warning=%.2fs radius=%.0f"), *ShownLabel.ToString(), bShowsThreat ? 1 : 0,
		FMath::Max(WarnLeft, 0.f), Radius);
	if (bArmPending)
	{
		if (bShowsThreat)
		{
			// The fill grows to the edge as the arm delay runs out: when it is full, the ground burns.
			Fill = AddGroundDecal(2);
			SetDecalLook(Fill, WarnFillLook);
		}
		WarnStartedAt = GetWorld()->GetTimeSeconds();
		ArmsAt = WarnStartedAt + WarnLeft;
		SetActorTickEnabled(true);
		Tick(0.f);
	}
	else
	{
		LightGround();
	}
}

void AGOTAEffectZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bArmPending)
	{
		SetActorTickEnabled(false);
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now >= ArmsAt)
	{
		ShowArmed(TEXT("counted down"));
		return;
	}
	if (Fill)
	{
		const float Span = FMath::Max(ArmsAt - WarnStartedAt, KINDA_SMALL_NUMBER);
		const float Frac = FMath::Clamp((Now - WarnStartedAt) / Span, 0.05f, 1.f);
		PlaceOnGround(Fill, GetActorLocation(), FVector::ForwardVector, Radius * Frac, Radius * Frac);
	}
}

void AGOTAEffectZone::OnServerArmed()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (bArmPending)
	{
		ShowArmed(TEXT("server arm arrived first"));
	}
	else if (ShownArmedAt >= 0.f)
	{
		// How far this screen's countdown was from the server's real arm moment (positive: it turned red early).
		UE_LOG(LogGOTADelivery, Log, TEXT("ZONE MARK %s server arm %.2fs after the mark"), *ShownLabel.ToString(), GetWorld()->GetTimeSeconds() - ShownArmedAt);
	}
}

void AGOTAEffectZone::ShowArmed(const TCHAR* Why)
{
	ShownArmedAt = GetWorld()->GetTimeSeconds();
	bArmPending = false;
	SetActorTickEnabled(false);
	if (Fill)
	{
		Fill->DestroyComponent();
		Fill = nullptr;
	}
	SetDecalLook(Edge, bShowsThreat ? DangerLook : FriendlyLook);
	LightGround();
	UE_LOG(LogGOTADelivery, Log, TEXT("ZONE MARK %s burning (%s)"), *ShownLabel.ToString(), Why);
}

void AGOTAEffectZone::StartZone(float InDuration, float InInterval, float InArmDelay)
{
	if (!HasAuthority())
	{
		return;
	}
	Interval = InInterval;
	bAnnounceArming = InArmDelay > 0.f;
	EndTime = GetWorld()->GetTimeSeconds() + InArmDelay + InDuration;
	GetWorldTimerManager().SetTimer(PulseTimer, this, &AGOTAEffectZone::Pulse, Interval, true, InArmDelay > 0.f ? InArmDelay : Interval);
}

void AGOTAEffectZone::Pulse()
{
	if (SourceIsGone())
	{
		Expire(TEXT("source gone or round over"));
		return;
	}
	UWorld* World = GetWorld();
	const FVector Centre = GetActorLocation();
	if (bAnnounceArming)
	{
		bAnnounceArming = false;
		UE_LOG(LogGOTADelivery, Log, TEXT("ZONE %s armed"), *Payload.Label.ToString());
		Multicast_Impact(Centre);
	}
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
