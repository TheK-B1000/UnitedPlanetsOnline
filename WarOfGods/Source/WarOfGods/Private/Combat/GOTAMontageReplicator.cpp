// White Tiger Studios 2024

#include "Combat/GOTAMontageReplicator.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Bosses/GOTABossComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAMontageReplicator)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAMontageRep, Log, All);

UGOTAMontageReplicator::UGOTAMontageReplicator()
{
	// A slow tick only to follow the anim instance when the enemy's body (and anim class) is swapped after spawn.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.f;
	SetIsReplicatedByDefault(true);
}

UGOTAMontageReplicator* UGOTAMontageReplicator::EnsureOn(APawn* Pawn)
{
	// Players too: their montages are not multicast here (ShouldReplicate), but their damage notifies feed the dodge guard.
	if (!Pawn || !Pawn->HasAuthority() || !Cast<ACharacter>(Pawn))
	{
		return nullptr;
	}
	if (UGOTAMontageReplicator* Existing = Pawn->FindComponentByClass<UGOTAMontageReplicator>())
	{
		return Existing;
	}
	UGOTAMontageReplicator* Rep = NewObject<UGOTAMontageReplicator>(Pawn, TEXT("GOTA_MontageReplicator"));
	Rep->SetIsReplicated(true);
	Rep->RegisterComponent();
	Rep->Rebind();
	return Rep;
}

UAnimInstance* UGOTAMontageReplicator::GetAnimInstance() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
}

void UGOTAMontageReplicator::Rebind()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	UAnimInstance* Anim = GetAnimInstance();
	if (Anim == Bound.Get())
	{
		return;
	}
	if (UAnimInstance* Old = Bound.Get())
	{
		Old->OnMontageStarted.RemoveDynamic(this, &UGOTAMontageReplicator::HandleMontageStarted);
		Old->OnMontageEnded.RemoveDynamic(this, &UGOTAMontageReplicator::HandleMontageEnded);
		Old->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UGOTAMontageReplicator::HandleNotifyBegin);
	}
	Bound = Anim;
	if (Anim)
	{
		Anim->OnMontageStarted.AddUniqueDynamic(this, &UGOTAMontageReplicator::HandleMontageStarted);
		Anim->OnMontageEnded.AddUniqueDynamic(this, &UGOTAMontageReplicator::HandleMontageEnded);
		// Must precede the Blueprint's Play Montage listener, which is added when each montage starts (delegates run in
		// the order they were added).
		Anim->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &UGOTAMontageReplicator::HandleNotifyBegin);
	}
}

void UGOTAMontageReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Rebind();
}

void UGOTAMontageReplicator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAnimInstance* Anim = Bound.Get())
	{
		Anim->OnMontageStarted.RemoveDynamic(this, &UGOTAMontageReplicator::HandleMontageStarted);
		Anim->OnMontageEnded.RemoveDynamic(this, &UGOTAMontageReplicator::HandleMontageEnded);
		Anim->OnPlayMontageNotifyBegin.RemoveDynamic(this, &UGOTAMontageReplicator::HandleNotifyBegin);
	}
	Super::EndPlay(EndPlayReason);
}

bool UGOTAMontageReplicator::ShouldReplicate(const UAnimMontage* Montage) const
{
	// Root-motion montages already reach simulated proxies through ACharacter::RepRootMotion; bosses multicast their own;
	// players replicate theirs through the player Blueprint's own SR_/MC_ events (a player pawn is seen before possession).
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Montage && !Montage->HasRootMotion() && Pawn && !Pawn->IsPlayerControlled() && !Pawn->FindComponentByClass<UGOTABossComponent>();
}

void UGOTAMontageReplicator::HandleMontageStarted(UAnimMontage* Montage)
{
	UAnimInstance* Anim = Bound.Get();
	if (!Anim || !Montage)
	{
		return;
	}
	// A defeated enemy cannot start an attack: the template can queue one more swing right after death (RC12: a spin
	// started 0.1 s after the killing blow and hit three players). Attack = a montage with a damage notify; death and
	// hit-reaction animations have none and play normally.
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && !OwnerPawn->IsPlayerControlled() && UGOTACombatantObserver::IsCombatantDefeated(GetOwner())
		&& FGOTABlueprintCombat::FindNextImpactTime(Montage, 0.f) >= 0.f)
	{
		Anim->Montage_Stop(0.f, Montage);
		UE_LOG(LogGOTAMontageRep, Log, TEXT("%s: defeated, attack %s cancelled"), *GetNameSafe(GetOwner()), *GetNameSafe(Montage));
		return;
	}
	if (!ShouldReplicate(Montage))
	{
		return;
	}
	++Sent;
	Multicast_PlayMontage(Montage, Anim->Montage_GetPlayRate(Montage), Anim->Montage_GetPosition(Montage));
}

void UGOTAMontageReplicator::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted && ShouldReplicate(Montage))
	{
		Multicast_StopMontage(Montage, 0.2f);
	}
}

void UGOTAMontageReplicator::HandleNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	APawn* Attacker = Cast<APawn>(GetOwner());
	const UAnimMontage* Montage = Cast<UAnimMontage>(Payload.SequenceAsset);
	if (!Attacker || !Attacker->HasAuthority() || !Montage || !Payload.NotifyEvent || !GetWorld())
	{
		return;
	}
	const float NotifyTime = Payload.NotifyEvent->GetTriggerTime();
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (*It != Attacker && It->IsPlayerControlled())
		{
			if (UGOTADuelFighterComponent* Fighter = It->FindComponentByClass<UGOTADuelFighterComponent>())
			{
				Fighter->TryGuardDodgedImpact(Attacker, Montage, NotifyTime);
			}
		}
	}
}

void UGOTAMontageReplicator::Multicast_PlayMontage_Implementation(UAnimMontage* Montage, float PlayRate, float StartPosition)
{
	if (!GetOwner() || GetOwner()->HasAuthority() || !Montage)
	{
		return; // the server already plays it
	}
	if (UAnimInstance* Anim = GetAnimInstance())
	{
		Anim->Montage_Play(Montage, PlayRate > 0.f ? PlayRate : 1.f, EMontagePlayReturnType::MontageLength, StartPosition);
	}
}

void UGOTAMontageReplicator::Multicast_StopMontage_Implementation(UAnimMontage* Montage, float BlendOutTime)
{
	if (!GetOwner() || GetOwner()->HasAuthority())
	{
		return;
	}
	if (UAnimInstance* Anim = GetAnimInstance(); Anim && Anim->Montage_IsPlaying(Montage))
	{
		Anim->Montage_Stop(BlendOutTime, Montage);
	}
}
