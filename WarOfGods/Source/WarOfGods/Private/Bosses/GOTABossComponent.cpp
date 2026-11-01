// White Tiger Studios 2024

#include "Bosses/GOTABossComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "BrainComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/GOTAPlayerCameraManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTABossComponent)

DEFINE_LOG_CATEGORY_STATIC(LogGOTABoss, Log, All);

namespace GOTABoss
{
	static const FName Slot(TEXT("DefaultSlot"));
	static constexpr float IntroSeconds = 4.6f;
}

UGOTABossComponent::UGOTABossComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

UGOTABossComponent* UGOTABossComponent::ApplyToPawn(APawn* Pawn, UGOTABossDefinition* InDefinition)
{
	if (!Pawn || !InDefinition || !Pawn->HasAuthority())
	{
		return nullptr;
	}
	UGOTABossComponent* Comp = Pawn->FindComponentByClass<UGOTABossComponent>();
	if (!Comp)
	{
		Comp = NewObject<UGOTABossComponent>(Pawn, TEXT("GOTABoss"));
		Comp->RegisterComponent();
		Pawn->AddInstanceComponent(Comp);
	}
	Comp->Definition = InDefinition;
	Comp->ApplyBody();
	Comp->CooldownUntil.Init(0.0, InDefinition->Attacks.Num());
	UE_LOG(LogGOTABoss, Log, TEXT("Boss %s configured on %s (%d attacks)"), *InDefinition->BossId.ToString(), *GetNameSafe(Pawn), InDefinition->Attacks.Num());
	return Comp;
}

void UGOTABossComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGOTABossComponent, Definition);
	DOREPLIFETIME(UGOTABossComponent, Phase);
	DOREPLIFETIME(UGOTABossComponent, State);
	DOREPLIFETIME(UGOTABossComponent, AttackIndex);
	DOREPLIFETIME(UGOTABossComponent, LastTelegraph);
}

void UGOTABossComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!Definition.IsNull())
	{
		ApplyBody();
	}
}

void UGOTABossComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (IntroCamera)
	{
		IntroCamera->Destroy();
		IntroCamera = nullptr;
	}
	Super::EndPlay(Reason);
}

void UGOTABossComponent::OnRep_Definition()
{
	ApplyBody();
}

void UGOTABossComponent::ApplyBody()
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	UGOTABossDefinition* Def = Definition.LoadSynchronous();
	if (!Char || !Def || bBodyApplied)
	{
		return;
	}
	USkeletalMesh* MeshAsset = Def->Mesh.LoadSynchronous();
	UClass* AnimClass = Def->AnimClass.LoadSynchronous();
	if (!MeshAsset)
	{
		UE_LOG(LogGOTABoss, Error, TEXT("Boss %s: mesh failed to load"), *Def->BossId.ToString());
		return;
	}
	bBodyApplied = true;
	// The boss's own skeleton and animation blueprint (no pose copying).
	UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
	Capsule->SetCapsuleSize(Def->CapsuleRadius, Def->CapsuleHalfHeight);
	USkeletalMeshComponent* Mesh = Char->GetMesh();
	Mesh->SetSkeletalMeshAsset(MeshAsset);
	if (AnimClass)
	{
		Mesh->SetAnimInstanceClass(AnimClass);
	}
	Mesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -Def->CapsuleHalfHeight), FRotator(0.f, Def->MeshYaw, 0.f));
	Mesh->SetRelativeScale3D(FVector(Def->MeshScale));
	// Bones must update on the server too: hit windows trace from the claws.
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh->SetHiddenInGame(false);
	// The Mannequin-era floating health bar and any attached weapon are not part of this creature.
	TArray<UWidgetComponent*> Widgets;
	Char->GetComponents<UWidgetComponent>(Widgets);
	for (UWidgetComponent* W : Widgets)
	{
		W->SetVisibility(false);
	}
	TArray<AActor*> Attached;
	Char->GetAttachedActors(Attached);
	for (AActor* A : Attached)
	{
		if (A && !A->IsA<APawn>())
		{
			A->SetActorHiddenInGame(true);
			A->SetActorEnableCollision(false);
		}
	}
	if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Def->WalkSpeed;
		Move->RotationRate = FRotator(0.f, Def->TurnRateDegrees, 0.f);
		Move->bOrientRotationToMovement = true;
		Move->bUseControllerDesiredRotation = false;
	}
	Char->bUseControllerRotationYaw = false;
	BaseWalkSpeed = Def->WalkSpeed;
	Preloaded.Reset();
	for (const FGOTABossAttack& A : Def->Attacks)
	{
		for (const TSoftObjectPtr<UAnimSequenceBase>& S : A.Animations)
		{
			Preloaded.Add(S.LoadSynchronous());
		}
		Preloaded.Add(A.ImpactFX.LoadSynchronous());
		Preloaded.Add(A.CastSound.LoadSynchronous());
		Preloaded.Add(A.ImpactSound.LoadSynchronous());
	}
	for (const TSoftObjectPtr<UAnimSequenceBase>* S : { &Def->IntroAnim, &Def->PhaseTwoAnim, &Def->DeathAnim, &Def->HitReactAnim, &Def->ChargeLoopAnim })
	{
		Preloaded.Add(S->LoadSynchronous());
	}
	Preloaded.Add(Def->RoarSound.LoadSynchronous());
	Preloaded.Add(Def->PhaseTwoFX.LoadSynchronous());
	UE_LOG(LogGOTABoss, Log, TEXT("Boss body %s on %s (anim %s, capsule %.0f/%.0f, %s)"), *GetNameSafe(MeshAsset), *GetNameSafe(Char), *GetNameSafe(AnimClass),
		Def->CapsuleRadius, Def->CapsuleHalfHeight, Char->HasAuthority() ? TEXT("authority") : TEXT("remote"));
}

FString UGOTABossComponent::DescribeState() const
{
	const UGOTABossDefinition* Def = Definition.Get();
	const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(GetOwner());
	return FString::Printf(TEXT("boss=%s state=%s phase=%d attack=%s hp=%.0f/%.0f at=%s"), Def ? *Def->BossId.ToString() : TEXT("?"),
		*StaticEnum<EGOTABossState>()->GetNameStringByValue(static_cast<int64>(State)), Phase,
		(Def && Def->Attacks.IsValidIndex(AttackIndex)) ? *Def->Attacks[AttackIndex].AttackId.ToString() : TEXT("-"), V.Health, V.MaxHealth,
		GetOwner() ? *GetOwner()->GetActorLocation().ToCompactString() : TEXT("-"));
}

// ============================================================================ server brain

bool UGOTABossComponent::IsRoundLive() const
{
	const ADuelGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr;
	return GS && GS->DuelPhase == EDuelPhase::InRound && GS->bCombatEnabled;
}

APawn* UGOTABossComponent::FindTarget() const
{
	const UGOTADuelFighterComponent* Fighter = GetOwner() ? GetOwner()->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr;
	APawn* Opp = Fighter ? Fighter->GetOpponent() : nullptr;
	return (IsValid(Opp) && !UGOTACombatantObserver::IsCombatantDefeated(Opp)) ? Opp : nullptr;
}

float UGOTABossComponent::CurrentAttackGap() const
{
	const UGOTABossDefinition* Def = Definition.Get();
	return Def ? (Phase >= 2 ? Def->AttackGapPhase2 : Def->AttackGap) : 1.5f;
}

float UGOTABossComponent::AttackClock(double Now) const
{
	const UGOTABossDefinition* Def = Definition.Get();
	const float Rate = (Def && Def->Attacks.IsValidIndex(AttackIndex)) ? Def->Attacks[AttackIndex].PlayRate : 1.f;
	return static_cast<float>(Now - AttackStart) * Rate;
}

bool UGOTABossComponent::HasLineOfSight(const AActor* A, const AActor* B) const
{
	if (!A || !B)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GOTABossLOS), false, A);
	Params.AddIgnoredActor(B);
	return !GetWorld()->LineTraceTestByChannel(A->GetActorLocation() + FVector(0, 0, 60), B->GetActorLocation() + FVector(0, 0, 60), ECC_WorldStatic, Params);
}

void UGOTABossComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner() && GetOwner()->HasAuthority() && Definition.Get())
	{
		TickServer(DeltaTime);
	}
}

void UGOTABossComponent::TickServer(float DeltaTime)
{
	const UGOTABossDefinition* Def = Definition.Get();
	APawn* Owner = Cast<APawn>(GetOwner());
	const double Now = GetWorld()->GetTimeSeconds();
	if (!Owner || State == EGOTABossState::Dead)
	{
		return;
	}
	if (UGOTACombatantObserver::IsCombatantDefeated(Owner))
	{
		Die();
		return;
	}
	if (!IsRoundLive())
	{
		if (State != EGOTABossState::Dormant && State != EGOTABossState::Intro)
		{
			SetBrainMoving(nullptr, false);
		}
		return;
	}
	if (!bIntroDone && State != EGOTABossState::Intro)
	{
		StartIntro();
		return;
	}
	if (State == EGOTABossState::Intro)
	{
		if (Now >= StateUntil)
		{
			EndIntro();
		}
		return;
	}

	// Damage tracking: phase II (once) and stagger (only when not committed to an attack).
	const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Owner);
	if (TrackedHealth >= 0.f && V.Health < TrackedHealth - 0.1f)
	{
		RecentDamage.Add(TPair<double, float>(Now, TrackedHealth - V.Health));
	}
	TrackedHealth = V.Health;
	RecentDamage.RemoveAll([&](const TPair<double, float>& D) { return Now - D.Key > Def->StaggerWindow; });
	if (Phase == 1 && V.MaxHealth > 0.f && V.Health / V.MaxHealth <= Def->PhaseTwoHealthFraction && State != EGOTABossState::Transition)
	{
		BeginPhaseTwo();
		return;
	}
	if (State == EGOTABossState::Transition)
	{
		if (Now >= StateUntil)
		{
			if (UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Owner))
			{
				FGOTABlueprintCombat::SetBool(DS, TEXT("IsInvincible"), false);
			}
			State = EGOTABossState::Recovering;
			StateUntil = Now + 0.3;
			LastAttackEnd = Now;
		}
		return;
	}
	float Recent = 0.f;
	for (const TPair<double, float>& D : RecentDamage)
	{
		Recent += D.Value;
	}
	if (Recent >= Def->StaggerThreshold && (State == EGOTABossState::Chase || State == EGOTABossState::Recovering))
	{
		RecentDamage.Reset();
		BeginStagger();
		return;
	}

	APawn* Target = FindTarget();
	switch (State)
	{
	case EGOTABossState::Dormant:
	case EGOTABossState::Chase:
		State = EGOTABossState::Chase;
		if (Target && bAIEnabled)
		{
			TickChase(Target, Now);
		}
		else
		{
			SetBrainMoving(nullptr, false);
		}
		break;
	case EGOTABossState::Attacking:
		TickAttack(Target, Now);
		break;
	case EGOTABossState::Recovering:
	case EGOTABossState::Staggered:
		if (Now >= StateUntil)
		{
			SetMoveLocked(false);
			State = EGOTABossState::Chase;
		}
		break;
	default:
		break;
	}
}

void UGOTABossComponent::TickChase(APawn* Target, double Now)
{
	const UGOTABossDefinition* Def = Definition.Get();
	APawn* Owner = Cast<APawn>(GetOwner());
	const float Dist = FVector::Dist2D(Target->GetActorLocation(), Owner->GetActorLocation());
	if ((Now - LastAttackEnd >= CurrentAttackGap() || !ForcedAttack.IsNone()) && ChooseAttack(Target, Now))
	{
		return;
	}
	// Close the distance on the navmesh; hold position when already in range (no infinite chasing into the player).
	const bool bWantMove = Dist > Def->EngageDistance;
	SetBrainMoving(bWantMove ? Target : nullptr, bWantMove);
	if (!bWantMove)
	{
		FaceTowards(Target->GetActorLocation(), GetWorld()->GetDeltaSeconds(), false);
	}
}

bool UGOTABossComponent::ChooseAttack(APawn* Target, double Now)
{
	const UGOTABossDefinition* Def = Definition.Get();
	APawn* Owner = Cast<APawn>(GetOwner());
	const float Dist = FVector::Dist2D(Target->GetActorLocation(), Owner->GetActorLocation());
	const bool bSight = HasLineOfSight(Owner, Target); // never attack through walls
	int32 Best = INDEX_NONE;
	float BestScore = 0.f;
	for (int32 i = 0; i < Def->Attacks.Num(); ++i)
	{
		const FGOTABossAttack& A = Def->Attacks[i];
		if (!ForcedAttack.IsNone())
		{
			if (A.AttackId == ForcedAttack)
			{
				Best = i;
				break;
			}
			continue;
		}
		const float Weight = Phase >= 2 ? A.WeightPhase2 : A.WeightPhase1;
		if (Weight <= 0.f || Now < CooldownUntil[i] || Dist < A.MinRange || Dist > A.MaxRange || !bSight)
		{
			continue;
		}
		const float Score = Weight * FMath::FRandRange(0.6f, 1.4f);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = i;
		}
	}
	ForcedAttack = NAME_None;
	if (Best == INDEX_NONE)
	{
		return false;
	}
	BeginAttack(Best, Target);
	return true;
}

void UGOTABossComponent::BeginAttack(int32 Index, APawn* Target)
{
	const UGOTABossDefinition* Def = Definition.Get();
	const FGOTABossAttack& A = Def->Attacks[Index];
	const double Now = GetWorld()->GetTimeSeconds();
	AttackIndex = Index;
	State = EGOTABossState::Attacking;
	AttackStart = Now;
	CooldownUntil[Index] = Now + A.Cooldown;
	ResolvedWindows.Reset();
	bAttackLanded = false;
	bCharging = false;
	SequenceCursor = 0;
	NextSequenceAt = Now;
	ChainRemaining = (Phase >= 2 && A.Kind == EGOTABossAttackKind::Slam) ? FMath::Max(1, A.ChainCountPhase2) - 1 : 0;
	SetBrainMoving(nullptr, false);
	SetMoveLocked(true); // no sliding while attacking
	if (Target)
	{
		FaceTowards(Target->GetActorLocation(), 0.f, true);
	}
	LastTelegraph = A.Telegraph;
	if (!A.Telegraph.IsEmpty())
	{
		Multicast_Cue(static_cast<uint8>(EGOTADuelCue::BossTelegraph), A.Telegraph);
	}
	UE_LOG(LogGOTABoss, Log, TEXT("Boss attack %s (phase %d, dist %.0f, chain %d)"), *A.AttackId.ToString(), Phase,
		Target ? FVector::Dist2D(Target->GetActorLocation(), GetOwner()->GetActorLocation()) : -1.f, ChainRemaining);
}

void UGOTABossComponent::TickAttack(APawn* Target, double Now)
{
	const UGOTABossDefinition* Def = Definition.Get();
	if (!Def->Attacks.IsValidIndex(AttackIndex))
	{
		EndAttack(false);
		return;
	}
	const FGOTABossAttack& A = Def->Attacks[AttackIndex];
	ACharacter* Char = Cast<ACharacter>(GetOwner());

	// Play the attack's animations back to back on the boss's own skeleton.
	if (SequenceCursor < A.Animations.Num() && Now >= NextSequenceAt && !bCharging)
	{
		UAnimSequenceBase* Seq = A.Animations[SequenceCursor].Get();
		if (Seq)
		{
			Multicast_PlaySequence(Seq, A.PlayRate, SequenceCursor == A.Animations.Num() - 1 && A.Kind == EGOTABossAttackKind::Charge, 1);
			NextSequenceAt = Now + Seq->GetPlayLength() / FMath::Max(A.PlayRate, 0.1f);
			if (A.CastSound.Get() && SequenceCursor == 0)
			{
				// cast sound rides on the impact multicast path (cosmetic) — played with the first sequence
				Multicast_Impact(AttackIndex, Char->GetActorLocation(), false);
			}
		}
		++SequenceCursor;
	}
	const float Clock = AttackClock(Now);

	// Commit: track the target only during the early wind-up.
	if (Target && Clock < A.TrackSeconds)
	{
		FaceTowards(Target->GetActorLocation(), GetWorld()->GetDeltaSeconds(), false);
	}

	if (A.Kind == EGOTABossAttackKind::Charge)
	{
		const float WindUp = A.HitWindows.Num() > 0 ? A.HitWindows[0].Start : 0.6f;
		if (!bCharging && ChargeEnd <= 0.0 && Clock >= WindUp)
		{
			bCharging = true;
			ChargeDir = Char->GetActorForwardVector().GetSafeNormal2D();
			ChargeStartedAt = Now;
			ChargeEnd = Now + A.ChargeSeconds;
			SetMoveLocked(false);
			Char->GetCharacterMovement()->MaxWalkSpeed = A.ChargeSpeed;
			Char->GetCharacterMovement()->bOrientRotationToMovement = false;
			if (UAnimSequenceBase* Loop = Def->ChargeLoopAnim.Get())
			{
				Multicast_PlaySequence(Loop, 1.3f, false, 8);
			}
			UE_LOG(LogGOTABoss, Log, TEXT("Boss charge: rush %s"), *ChargeDir.ToCompactString());
		}
		if (bCharging)
		{
			Char->AddMovementInput(ChargeDir, 1.f, true);
			const float Speed = Char->GetVelocity().Size2D();
			const bool bBlocked = Now - ChargeStartedAt > 0.3 && Speed < A.ChargeSpeed * 0.25f; // ran into the arena wall
			if (Target && !ResolvedWindows.Contains(0))
			{
				const float Reach = Char->GetCapsuleComponent()->GetScaledCapsuleRadius() + 70.f;
				if (FVector::Dist2D(Target->GetActorLocation(), Char->GetActorLocation()) <= Reach + 40.f && HasLineOfSight(Char, Target))
				{
					ResolvedWindows.Add(0);
					const float Dmg = A.HitWindows.Num() > 0 ? A.HitWindows[0].Damage : 20.f;
					bAttackLanded = DamageTarget(Target, A, Dmg);
					Knock(Target, A.Knockback, 350.f);
					Multicast_Impact(AttackIndex, Target->GetActorLocation(), bAttackLanded);
					ChargeEnd = Now; // stop on impact
				}
			}
			if (Now >= ChargeEnd || bBlocked)
			{
				bCharging = false;
				Char->GetCharacterMovement()->StopMovementImmediately();
				Char->GetCharacterMovement()->MaxWalkSpeed = Phase >= 2 ? Def->WalkSpeedPhase2 : Def->WalkSpeed;
				Char->GetCharacterMovement()->bOrientRotationToMovement = true;
				Multicast_StopAnimation(0.25f);
				UE_LOG(LogGOTABoss, Log, TEXT("Boss charge ended (%s)"), ResolvedWindows.Contains(0) ? TEXT("hit") : (bBlocked ? TEXT("wall") : TEXT("missed")));
				ChargeEnd = -1.0;
				EndAttack(bAttackLanded);
			}
		}
		return;
	}

	// Animation-timed hit windows.
	for (int32 w = 0; w < A.HitWindows.Num(); ++w)
	{
		const FGOTABossHitWindow& Win = A.HitWindows[w];
		if (Clock >= Win.Start && Clock <= Win.End + 0.05f && !ResolvedWindows.Contains(w))
		{
			ResolveWindow(A, Win, w, Target);
		}
		else if (Clock > Win.End + 0.05f && !ResolvedWindows.Contains(w))
		{
			// Window closed without contact: a clean miss (logged with the claw-to-target distance for tuning).
			ResolvedWindows.Add(w);
			if (Target && Char && A.Kind == EGOTABossAttackKind::Melee)
			{
				const FVector Claw = Char->GetMesh()->DoesSocketExist(Win.Bone) ? Char->GetMesh()->GetSocketLocation(Win.Bone) : Char->GetActorLocation();
				const FVector Local = Char->GetActorTransform().InverseTransformPosition(Claw);
				const FVector TargetLocal = Char->GetActorTransform().InverseTransformPosition(Target->GetActorLocation());
				UE_LOG(LogGOTABoss, Log, TEXT("Boss %s window %d missed: %s %.0f from target (radius %.0f), body %.0f, target local fwd=%.0f right=%.0f, claw local fwd=%.0f right=%.0f up=%.0f"), *A.AttackId.ToString(), w, *Win.Bone.ToString(),
					FVector::Dist2D(Claw, Target->GetActorLocation()), Win.Radius, FVector::Dist2D(Char->GetActorLocation(), Target->GetActorLocation()), TargetLocal.X, TargetLocal.Y, Local.X, Local.Y, Local.Z);
			}
		}
	}

	const bool bAnimsDone = SequenceCursor >= A.Animations.Num() && Now >= NextSequenceAt - 0.05;
	if (bAnimsDone)
	{
		if (ChainRemaining > 0)
		{
			--ChainRemaining;
			AttackStart = Now;
			SequenceCursor = 0;
			NextSequenceAt = Now;
			ResolvedWindows.Reset();
			if (Target)
			{
				FaceTowards(Target->GetActorLocation(), 0.f, true);
			}
			return;
		}
		EndAttack(bAttackLanded);
	}
}

void UGOTABossComponent::ResolveWindow(const FGOTABossAttack& A, const FGOTABossHitWindow& Win, int32 WindowIndex, APawn* Target)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char || !Target)
	{
		if (A.Kind == EGOTABossAttackKind::Slam)
		{
			ResolvedWindows.Add(WindowIndex);
			Multicast_Impact(AttackIndex, Char->GetActorLocation() - FVector(0, 0, Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), false);
		}
		return;
	}
	const float TargetRadius = Cast<ACharacter>(Target) ? Cast<ACharacter>(Target)->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
	switch (A.Kind)
	{
	case EGOTABossAttackKind::Melee:
	{
		// Sphere on the claw bone, checked every tick inside the window: one hit per window (no duplicate damage).
		const FVector Claw = Char->GetMesh()->DoesSocketExist(Win.Bone) ? Char->GetMesh()->GetSocketLocation(Win.Bone) : Char->GetActorLocation() + Char->GetActorForwardVector() * 200.f;
		FVector ToTarget = Target->GetActorLocation() - Claw;
		const float Vertical = FMath::Abs(ToTarget.Z);
		ToTarget.Z = 0.f;
		// Swipe arc: Rampage's Paragon swipes are animated for point-blank range (the claws stay near his own body),
		// so the window also strikes anything in front of him within capsule + claw reach - the same rule as Grab.
		FVector FromBody = Target->GetActorLocation() - Char->GetActorLocation();
		FromBody.Z = 0.f;
		const float Facing = FVector::DotProduct(FromBody.GetSafeNormal(), Char->GetActorForwardVector());
		const float ArcReach = Char->GetCapsuleComponent()->GetScaledCapsuleRadius() + Win.Radius + TargetRadius;
		// The claw sphere only counts in his front half: on the backswing the hand passes behind him, and the
		// visible strike never reaches a target at his back.
		const bool bClawContact = ToTarget.Size() <= Win.Radius + TargetRadius && Facing >= 0.f;
		const bool bArcContact = FromBody.Size() <= ArcReach && Facing >= 0.35f;
		if ((bClawContact || bArcContact) && Vertical < 200.f && HasLineOfSight(Char, Target))
		{
			UE_LOG(LogGOTABoss, Log, TEXT("Boss %s window %d hit (%s): body %.0f, reach %.0f, facing %.2f"), *A.AttackId.ToString(), WindowIndex,
				bClawContact ? TEXT("claw") : TEXT("swipe arc"), FromBody.Size(), ArcReach, Facing);
			ResolvedWindows.Add(WindowIndex);
			const bool bLanded = DamageTarget(Target, A, Win.Damage);
			bAttackLanded |= bLanded;
			Multicast_Impact(AttackIndex, Claw, bLanded);
		}
		break;
	}
	case EGOTABossAttackKind::Slam:
	{
		// Impact frame only: radial damage, never before the slam lands.
		ResolvedWindows.Add(WindowIndex);
		const FVector Ground = Char->GetActorLocation() - FVector(0, 0, Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		const FVector Centre = Char->GetActorLocation() + Char->GetActorForwardVector() * 120.f;
		FVector To = Target->GetActorLocation() - Centre;
		const float Vertical = FMath::Abs(To.Z);
		To.Z = 0.f;
		bool bLanded = false;
		if (To.Size() <= A.SlamRadius + TargetRadius && Vertical < 250.f && HasLineOfSight(Char, Target))
		{
			bLanded = DamageTarget(Target, A, Win.Damage);
			if (bLanded)
			{
				Knock(Target, A.Knockback, 300.f);
			}
			bAttackLanded |= bLanded;
		}
		Multicast_Impact(AttackIndex, Ground + Char->GetActorForwardVector() * 120.f, bLanded);
		UE_LOG(LogGOTABoss, Log, TEXT("Boss slam impact: target at %.0f (radius %.0f) landed=%d"), To.Size(), A.SlamRadius, bLanded ? 1 : 0);
		break;
	}
	case EGOTABossAttackKind::Grab:
	{
		// Grapple prototype: valid only if the target is close AND in front at the grab frame.
		ResolvedWindows.Add(WindowIndex);
		FVector To = Target->GetActorLocation() - Char->GetActorLocation();
		To.Z = 0.f;
		const float Facing = FVector::DotProduct(To.GetSafeNormal(), Char->GetActorForwardVector());
		const float Reach = Char->GetCapsuleComponent()->GetScaledCapsuleRadius() + Win.Radius + TargetRadius;
		bool bLanded = false;
		if (To.Size() <= Reach && Facing >= 0.6f && HasLineOfSight(Char, Target))
		{
			bLanded = DamageTarget(Target, A, Win.Damage);
			if (bLanded)
			{
				Knock(Target, A.Knockback, 650.f); // toss
			}
			bAttackLanded |= bLanded;
		}
		Multicast_Impact(AttackIndex, Target->GetActorLocation(), bLanded);
		UE_LOG(LogGOTABoss, Log, TEXT("Boss grapple: dist %.0f facing %.2f landed=%d"), To.Size(), Facing, bLanded ? 1 : 0);
		break;
	}
	default:
		break;
	}
}

bool UGOTABossComponent::DamageTarget(APawn* Target, const FGOTABossAttack& A, float Amount)
{
	// The victim's own BPC_DamageSystem decides block / parry / i-frames: the single health authority.
	const bool bLanded = FGOTABlueprintCombat::ApplyDamage(Target, GetOwner(), Amount, A.bCanBeBlocked, false, A.HitResponse);
	UE_LOG(LogGOTABoss, Log, TEXT("Boss %s hit %s for %.1f (landed=%d)"), *A.AttackId.ToString(), *GetNameSafe(Target), Amount, bLanded ? 1 : 0);
	return bLanded;
}

void UGOTABossComponent::Knock(APawn* Target, float Speed, float Up)
{
	ACharacter* Victim = Cast<ACharacter>(Target);
	if (!Victim || Speed <= 0.f)
	{
		return;
	}
	FVector Dir = Target->GetActorLocation() - GetOwner()->GetActorLocation();
	Dir.Z = 0.f;
	Victim->LaunchCharacter(Dir.GetSafeNormal() * Speed + FVector(0, 0, Up), true, true);
}

void UGOTABossComponent::EndAttack(bool bHitSomething)
{
	const UGOTABossDefinition* Def = Definition.Get();
	const double Now = GetWorld()->GetTimeSeconds();
	float Recovery = 0.8f;
	if (Def && Def->Attacks.IsValidIndex(AttackIndex))
	{
		const FGOTABossAttack& A = Def->Attacks[AttackIndex];
		Recovery = A.Recovery * (Phase >= 2 ? A.RecoveryScalePhase2 : 1.f);
		if (!bHitSomething)
		{
			Recovery += 0.35f; // a whiff leaves the creature open a little longer
		}
	}
	State = EGOTABossState::Recovering;
	StateUntil = Now + Recovery;
	LastAttackEnd = Now + Recovery;
	AttackIndex = INDEX_NONE;
	LastTelegraph = FText::GetEmpty();
}

void UGOTABossComponent::BeginStagger()
{
	const UGOTABossDefinition* Def = Definition.Get();
	UAnimSequenceBase* React = Def->HitReactAnim.Get();
	State = EGOTABossState::Staggered;
	SetBrainMoving(nullptr, false);
	SetMoveLocked(true);
	StateUntil = GetWorld()->GetTimeSeconds() + (React ? React->GetPlayLength() : 0.8);
	if (React)
	{
		Multicast_PlaySequence(React, 1.f, false, 1);
	}
	UE_LOG(LogGOTABoss, Log, TEXT("Boss staggered"));
}

void UGOTABossComponent::BeginPhaseTwo()
{
	const UGOTABossDefinition* Def = Definition.Get();
	APawn* Owner = Cast<APawn>(GetOwner());
	const double Now = GetWorld()->GetTimeSeconds();
	Phase = 2; // set first: the transition can never trigger twice
	bCharging = false;
	ChargeEnd = -1.0;
	AttackIndex = INDEX_NONE;
	State = EGOTABossState::Transition;
	SetBrainMoving(nullptr, false);
	SetMoveLocked(true);
	if (ACharacter* Char = Cast<ACharacter>(Owner))
	{
		Char->GetCharacterMovement()->MaxWalkSpeed = Def->WalkSpeedPhase2;
		Char->GetCharacterMovement()->bOrientRotationToMovement = true;
	}
	BaseWalkSpeed = Def->WalkSpeedPhase2;
	if (UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Owner))
	{
		FGOTABlueprintCombat::SetBool(DS, TEXT("IsInvincible"), true); // the roar is a breather, not a damage race
	}
	UAnimSequenceBase* Roar = Def->PhaseTwoAnim.Get();
	StateUntil = Now + (Roar ? Roar->GetPlayLength() : 1.3);
	if (Roar)
	{
		Multicast_PlaySequence(Roar, 1.f, false, 1);
	}
	Multicast_Cue(static_cast<uint8>(EGOTADuelCue::BossPhaseTwo), NSLOCTEXT("GOTA", "BossPhase2", "UNCHAINED FURY"));
	UE_LOG(LogGOTABoss, Log, TEXT("Boss PHASE II at %.0f%% health"), UGOTACombatantObserver::ReadVitals(Owner).GetFraction() * 100.f);
}

void UGOTABossComponent::Die()
{
	const UGOTABossDefinition* Def = Definition.Get();
	if (State == EGOTABossState::Dead)
	{
		return;
	}
	State = EGOTABossState::Dead;
	AttackIndex = INDEX_NONE;
	bCharging = false;
	SetBrainMoving(nullptr, false);
	SetMoveLocked(true);
	if (AAIController* AIC = Cast<AAIController>(Cast<APawn>(GetOwner())->GetController()))
	{
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("boss dead"));
		}
	}
	if (UAnimSequenceBase* Death = Def ? Def->DeathAnim.Get() : nullptr)
	{
		Multicast_PlaySequence(Death, 1.f, true, 1);
	}
	Multicast_Cue(static_cast<uint8>(EGOTADuelCue::BossDefeated), Def ? Def->DisplayName : FText::GetEmpty());
	UE_LOG(LogGOTABoss, Log, TEXT("Boss DEFEATED"));
}

// ============================================================================ intro

void UGOTABossComponent::StartIntro()
{
	const UGOTABossDefinition* Def = Definition.Get();
	ACharacter* Boss = Cast<ACharacter>(GetOwner());
	if (!Boss || !Def)
	{
		return;
	}
	State = EGOTABossState::Intro;
	StateUntil = GetWorld()->GetTimeSeconds() + GOTABoss::IntroSeconds;
	SetBrainMoving(nullptr, false);
	SetMoveLocked(true);
	APawn* Player = FindTarget();
	// Frame the creature from the player's side, low and wide; then hand the camera back.
	const FVector BossLoc = Boss->GetActorLocation();
	FVector Toward = Player ? (Player->GetActorLocation() - BossLoc).GetSafeNormal2D() : Boss->GetActorForwardVector();
	const FVector CamLoc = BossLoc + Toward * 720.f + FVector::CrossProduct(Toward, FVector::UpVector) * 260.f + FVector(0, 0, 40.f);
	const FRotator CamRot = (BossLoc + FVector(0, 0, 90.f) - CamLoc).Rotation();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	IntroCamera = GetWorld()->SpawnActor<ACameraActor>(CamLoc, CamRot, P);
	if (IntroCamera)
	{
		IntroCamera->GetCameraComponent()->SetFieldOfView(62.f);
	}
	if (Player)
	{
		if (UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Player))
		{
			FGOTABlueprintCombat::SetBool(DS, TEXT("IsInvincible"), true); // noninteractive: the player cannot be hurt
		}
		FGOTABlueprintCombat::SetBool(Player, TEXT("CanMove"), false);
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			if (IntroCamera)
			{
				PC->SetViewTargetWithBlend(IntroCamera, 0.6f, VTBlend_Cubic);
			}
			PC->SetIgnoreMoveInput(true);
			// Local host can skip with Enter / gamepad Start.
			if (PC->IsLocalController() && PC->InputComponent)
			{
				for (const FKey& K : { EKeys::Enter, EKeys::Gamepad_Special_Right })
				{
					FInputKeyBinding B(FInputChord(K), IE_Pressed);
					B.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this]() { SkipIntro(); });
					PC->InputComponent->KeyBindings.Add(B);
				}
			}
		}
	}
	if (UAnimSequenceBase* Roar = Def->IntroAnim.Get())
	{
		Multicast_PlaySequence(Roar, 1.f, false, 1);
	}
	// Title after the roar starts.
	FTimerHandle H;
	GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (State == EGOTABossState::Intro && Definition.Get())
		{
			Multicast_Cue(static_cast<uint8>(EGOTADuelCue::BossTitle), Definition.Get()->DisplayName);
		}
	}), 1.2f, false);
	UE_LOG(LogGOTABoss, Log, TEXT("Boss intro started (%.1fs, skippable)"), GOTABoss::IntroSeconds);
}

void UGOTABossComponent::SkipIntro()
{
	if (State == EGOTABossState::Intro && GetOwner() && GetOwner()->HasAuthority())
	{
		UE_LOG(LogGOTABoss, Log, TEXT("Boss intro skipped"));
		EndIntro();
	}
}

void UGOTABossComponent::EndIntro()
{
	bIntroDone = true;
	APawn* Player = FindTarget();
	if (Player)
	{
		if (UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Player))
		{
			FGOTABlueprintCombat::SetBool(DS, TEXT("IsInvincible"), false);
		}
		FGOTABlueprintCombat::SetBool(Player, TEXT("CanMove"), true);
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			PC->SetViewTargetWithBlend(Player, 0.7f, VTBlend_Cubic);
			PC->SetIgnoreMoveInput(false);
			if (PC->InputComponent)
			{
				PC->InputComponent->KeyBindings.RemoveAll([this](const FInputKeyBinding& B) { return B.KeyDelegate.IsBoundToObject(this); });
			}
		}
	}
	if (IntroCamera)
	{
		FTimerHandle H;
		TWeakObjectPtr<ACameraActor> Cam = IntroCamera;
		GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [Cam]() { if (Cam.IsValid()) { Cam->Destroy(); } }), 1.f, false);
		IntroCamera = nullptr;
	}
	Multicast_StopAnimation(0.3f);
	SetMoveLocked(false);
	State = EGOTABossState::Chase;
	LastAttackEnd = GetWorld()->GetTimeSeconds() + 0.8; // short beat before the first attack
	UE_LOG(LogGOTABoss, Log, TEXT("Boss intro ended — fight"));
}

// ============================================================================ movement helpers

void UGOTABossComponent::FaceTowards(const FVector& Location, float DeltaTime, bool bInstant)
{
	AActor* Owner = GetOwner();
	const UGOTABossDefinition* Def = Definition.Get();
	FRotator Want = (Location - Owner->GetActorLocation()).Rotation();
	Want.Pitch = Want.Roll = 0.f;
	const FRotator Now = Owner->GetActorRotation();
	const FRotator New = bInstant ? Want : FMath::RInterpConstantTo(Now, Want, DeltaTime, Def ? Def->TurnRateDegrees : 240.f);
	Owner->SetActorRotation(New);
}

void UGOTABossComponent::SetMoveLocked(bool bLocked)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		return;
	}
	UCharacterMovementComponent* Move = Char->GetCharacterMovement();
	if (bLocked)
	{
		Move->StopMovementImmediately();
		Move->MaxWalkSpeed = 0.f;
	}
	else
	{
		Move->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UGOTABossComponent::SetBrainMoving(APawn* Target, bool bMove)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	AAIController* AIC = Owner ? Cast<AAIController>(Owner->GetController()) : nullptr;
	if (!AIC)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (bMove && Target)
	{
		if (Now - LastMoveRequest > 0.25 || AIC->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			LastMoveRequest = Now;
			const UGOTABossDefinition* Def = Definition.Get();
			// Centre-to-centre acceptance (bStopOnOverlap would add both capsule radii and park the boss out of claw reach).
			AIC->MoveToActor(Target, Def ? Def->EngageDistance * 0.85f : 200.f, false, true, true);
		}
	}
	else if (AIC->GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		AIC->StopMovement();
	}
}

// ============================================================================ presentation (all peers)

void UGOTABossComponent::PlayLocal(UAnimSequenceBase* Sequence, float PlayRate, bool bHoldLastFrame, int32 LoopCount)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anim = Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim || !Sequence)
	{
		return;
	}
	const UGOTABossDefinition* Def = Definition.Get();
	const FName SlotName = Def ? Def->MontageSlot : GOTABoss::Slot;
	UAnimMontage* M = Anim->PlaySlotAnimationAsDynamicMontage(Sequence, SlotName, 0.12f, 0.2f, PlayRate, LoopCount);
	if (M && bHoldLastFrame)
	{
		M->bEnableAutoBlendOut = false; // death: stay down
	}
	if (!M)
	{
		UE_LOG(LogGOTABoss, Warning, TEXT("Boss could not play %s (no DefaultSlot in the anim blueprint?)"), *GetNameSafe(Sequence));
	}
}

void UGOTABossComponent::Multicast_PlaySequence_Implementation(UAnimSequenceBase* Sequence, float PlayRate, bool bHoldLastFrame, int32 LoopCount)
{
	PlayLocal(Sequence, PlayRate, bHoldLastFrame, LoopCount);
}

void UGOTABossComponent::Multicast_StopAnimation_Implementation(float BlendOut)
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (UAnimInstance* Anim = Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
	{
		const UGOTABossDefinition* Def = Definition.Get();
		Anim->StopSlotAnimation(BlendOut, Def ? Def->MontageSlot : GOTABoss::Slot);
	}
}

void UGOTABossComponent::Multicast_Impact_Implementation(int32 InAttackIndex, FVector_NetQuantize Location, bool bLanded)
{
	const UGOTABossDefinition* Def = Definition.Get();
	UWorld* World = GetWorld();
	if (!Def || !Def->Attacks.IsValidIndex(InAttackIndex) || !World || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const FGOTABossAttack& A = Def->Attacks[InAttackIndex];
	const bool bIsCastCue = !bLanded && A.Kind != EGOTABossAttackKind::Slam && FVector::Dist(Location, GetOwner()->GetActorLocation()) < 5.f;
	if (bIsCastCue)
	{
		if (USoundBase* S = A.CastSound.Get())
		{
			UGameplayStatics::PlaySoundAtLocation(this, S, Location);
		}
		return;
	}
	if (UParticleSystem* PS = A.ImpactFX.Get())
	{
		if (UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(World, PS, Location, GetOwner()->GetActorRotation(), FVector(A.FXScale), true))
		{
			FTimerHandle H;
			TWeakObjectPtr<UParticleSystemComponent> W = PSC;
			World->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(PSC, [W]() { if (W.IsValid()) { W->DestroyComponent(); } }), 1.6f, false);
		}
	}
	if (USoundBase* S = A.ImpactSound.Get())
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, Location);
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->IsLocalController() && PC->GetPawn() && FVector::Dist(PC->GetPawn()->GetActorLocation(), Location) < 1400.f)
		{
			if (AGOTAPlayerCameraManager* Cam = Cast<AGOTAPlayerCameraManager>(PC->PlayerCameraManager))
			{
				Cam->AddImpactKick(A.ImpactKick * (bLanded ? 1.f : 0.6f));
			}
		}
	}
}

void UGOTABossComponent::Multicast_Cue_Implementation(uint8 Cue, const FText& Text)
{
	LastTelegraph = Text;
	if (ADuelGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr)
	{
		GS->OnDuelCombatCue.Broadcast(static_cast<EGOTADuelCue>(Cue), GetOwner(), nullptr);
	}
	const UGOTABossDefinition* Def = Definition.Get();
	if (Def && (Cue == static_cast<uint8>(EGOTADuelCue::BossTitle) || Cue == static_cast<uint8>(EGOTADuelCue::BossPhaseTwo)) && GetNetMode() != NM_DedicatedServer)
	{
		if (USoundBase* S = Def->RoarSound.Get())
		{
			UGameplayStatics::PlaySoundAtLocation(this, S, GetOwner()->GetActorLocation(), 1.2f);
		}
		if (Cue == static_cast<uint8>(EGOTADuelCue::BossPhaseTwo))
		{
			if (UParticleSystem* PS = Def->PhaseTwoFX.Get())
			{
				if (UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAttached(PS, GetOwner()->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.4f), EAttachLocation::SnapToTarget, true))
				{
					FTimerHandle H;
					TWeakObjectPtr<UParticleSystemComponent> W = PSC;
					GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(PSC, [W]() { if (W.IsValid()) { W->DestroyComponent(); } }), 2.f, false);
				}
			}
		}
	}
}

void UGOTABossComponent::DebugForceAttack(FName AttackId)
{
	ForcedAttack = AttackId;
	LastAttackEnd = -100.0;
	if (State == EGOTABossState::Recovering)
	{
		StateUntil = 0.0;
	}
}
