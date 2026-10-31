// White Tiger Studios 2024

#include "Combat/LockonComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

ULockonComponent::ULockonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULockonComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULockonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentTarget)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !IsValid(CurrentTarget))
	{
		StopLockon();
		return;
	}

	const float DistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), CurrentTarget->GetActorLocation());
	if (DistSq > FMath::Square(MaxLockonDistance * 1.15f))
	{
		StopLockon();
		return;
	}

	if (bRequireLineOfSight)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LockonLOS), false, OwnerActor);
		Params.AddIgnoredActor(CurrentTarget);
		const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		const FVector End = CurrentTarget->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			if (Hit.GetActor() != CurrentTarget)
			{
				StopLockon();
				return;
			}
		}
	}

	if (bRotateOwnerTowardTarget)
	{
		const FVector OwnerLoc = OwnerActor->GetActorLocation();
		const FVector TargetLoc = CurrentTarget->GetActorLocation();
		const FRotator Desired = UKismetMathLibrary::FindLookAtRotation(OwnerLoc, TargetLoc);
		const FRotator Current = OwnerActor->GetActorRotation();
		const FRotator NewRot = FMath::RInterpTo(Current, FRotator(Current.Pitch, Desired.Yaw, Current.Roll), DeltaTime, SoftLookInterpSpeed);
		OwnerActor->SetActorRotation(NewRot);
	}

	if (APlayerController* PC = GetOwnerPlayerController())
	{
		const FVector CamLoc = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : OwnerActor->GetActorLocation();
		const FRotator DesiredCam = UKismetMathLibrary::FindLookAtRotation(CamLoc, CurrentTarget->GetActorLocation());
		FRotator Control = PC->GetControlRotation();
		Control = FMath::RInterpTo(Control, DesiredCam, DeltaTime, SoftLookInterpSpeed);
		PC->SetControlRotation(Control);
	}
}

void ULockonComponent::ToggleLockon()
{
	if (CurrentTarget)
	{
		StopLockon();
	}
	else
	{
		StartLockon();
	}
}

void ULockonComponent::StartLockon()
{
	if (AActor* Best = FindBestTarget())
	{
		SetCurrentTarget(Best);
		SetComponentTickEnabled(true);
	}
}

void ULockonComponent::StopLockon()
{
	SetCurrentTarget(nullptr);
	SetComponentTickEnabled(false);
}

void ULockonComponent::SwitchTarget(float Direction)
{
	if (FMath::IsNearlyZero(Direction) || !CurrentTarget)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	APlayerController* PC = GetOwnerPlayerController();
	if (!OwnerActor || !PC)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewRight = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Y);

	TArray<AActor*> Candidates;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(TargetTraceChannel));
	TArray<AActor*> Ignore;
	Ignore.Add(OwnerActor);

	UKismetSystemLibrary::SphereOverlapActors(
		this,
		OwnerActor->GetActorLocation(),
		MaxLockonDistance,
		ObjectTypes,
		APawn::StaticClass(),
		Ignore,
		Candidates);

	AActor* Best = nullptr;
	float BestScore = -FLT_MAX;
	const FVector ToCurrent = (CurrentTarget->GetActorLocation() - ViewLocation).GetSafeNormal();

	for (AActor* Candidate : Candidates)
	{
		if (Candidate == CurrentTarget || !IsValidLockonCandidate(Candidate, ViewLocation, ViewRotation.Vector()))
		{
			continue;
		}

		const FVector ToCandidate = (Candidate->GetActorLocation() - ViewLocation).GetSafeNormal();
		const float Side = FVector::DotProduct(ViewRight, ToCandidate);
		if ((Direction > 0.f && Side < 0.05f) || (Direction < 0.f && Side > -0.05f))
		{
			continue;
		}

		const float Alignment = FVector::DotProduct(ToCurrent, ToCandidate);
		if (Alignment > BestScore)
		{
			BestScore = Alignment;
			Best = Candidate;
		}
	}

	if (Best)
	{
		SetCurrentTarget(Best);
	}
}

AActor* ULockonComponent::FindBestTarget() const
{
	AActor* OwnerActor = GetOwner();
	APlayerController* PC = GetOwnerPlayerController();
	if (!OwnerActor || !PC)
	{
		return nullptr;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDir = ViewRotation.Vector();

	TArray<AActor*> Candidates;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(TargetTraceChannel));
	TArray<AActor*> Ignore;
	Ignore.Add(OwnerActor);

	UKismetSystemLibrary::SphereOverlapActors(
		this,
		OwnerActor->GetActorLocation(),
		MaxLockonDistance,
		ObjectTypes,
		APawn::StaticClass(),
		Ignore,
		Candidates);

	AActor* Best = nullptr;
	float BestScore = -FLT_MAX;

	for (AActor* Candidate : Candidates)
	{
		if (!IsValidLockonCandidate(Candidate, ViewLocation, ViewDir))
		{
			continue;
		}

		const FVector ToTarget = (Candidate->GetActorLocation() - ViewLocation).GetSafeNormal();
		const float Dot = FVector::DotProduct(ViewDir, ToTarget);
		const float DistAlpha = 1.f - (FVector::Dist(OwnerActor->GetActorLocation(), Candidate->GetActorLocation()) / MaxLockonDistance);
		const float Score = Dot * 2.f + DistAlpha;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}

	return Best;
}

bool ULockonComponent::IsValidLockonCandidate(AActor* Candidate, const FVector& ViewLocation, const FVector& ViewDirection) const
{
	if (!IsValid(Candidate) || Candidate == GetOwner())
	{
		return false;
	}

	const APawn* Pawn = Cast<APawn>(Candidate);
	if (!Pawn || Pawn->IsPlayerControlled())
	{
		// Default: lock onto AI / non-player pawns only.
		return false;
	}

	const FVector ToTarget = (Candidate->GetActorLocation() - ViewLocation).GetSafeNormal();
	const float Dot = FVector::DotProduct(ViewDirection, ToTarget);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(MaxLockonAngleDegrees));
	if (Dot < MinDot)
	{
		return false;
	}

	if (bRequireLineOfSight && GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LockonCandidateLOS), false, GetOwner());
		Params.AddIgnoredActor(Candidate);
		const FVector Start = ViewLocation;
		const FVector End = Candidate->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			if (Hit.GetActor() != Candidate)
			{
				return false;
			}
		}
	}

	return true;
}

void ULockonComponent::SetCurrentTarget(AActor* NewTarget)
{
	if (CurrentTarget == NewTarget)
	{
		return;
	}

	AActor* Previous = CurrentTarget;
	CurrentTarget = NewTarget;
	OnLockonTargetChanged.Broadcast(CurrentTarget, Previous);
}

APlayerController* ULockonComponent::GetOwnerPlayerController() const
{
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}
	return nullptr;
}
