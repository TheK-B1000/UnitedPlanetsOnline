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
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAMontageReplicator)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAMontageRep, Log, All);

UGOTAMontageReplicator::UGOTAMontageReplicator()
{
	// A slow tick: follow the anim instance when a body (and anim class) is swapped after spawn, and keep a player's
	// stance mirror current.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
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
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Rebind();
		const APawn* Pawn = Cast<APawn>(GetOwner());
		if (Pawn && Pawn->IsPlayerControlled())
		{
			MirroredStance = ReadStance(Pawn); // replicates only when it changes
		}
	}
	else if (MirroredStance >= 0 && StanceAttempts < 3 && ReadStance(GetOwner()) != MirroredStance)
	{
		SyncStanceLocally(); // e.g. the Blueprint reset its stance on this machine after the mirror arrived
	}
}

void UGOTAMontageReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGOTAMontageReplicator, MirroredStance);
}

int32 UGOTAMontageReplicator::ReadStance(const AActor* Actor)
{
	const FProperty* P = Actor ? Actor->GetClass()->FindPropertyByName(TEXT("Stance")) : nullptr;
	if (const FByteProperty* B = CastField<FByteProperty>(P))
	{
		return B->GetPropertyValue_InContainer(Actor);
	}
	if (const FEnumProperty* E = CastField<FEnumProperty>(P))
	{
		return static_cast<int32>(E->GetUnderlyingProperty()->GetSignedIntPropertyValue(E->ContainerPtrToValuePtr<void>(Actor)));
	}
	return -1;
}

void UGOTAMontageReplicator::OnRep_MirroredStance()
{
	StanceAttempts = 0;
	SyncStanceLocally();
}

void UGOTAMontageReplicator::SyncStanceLocally()
{
	AActor* Owner = GetOwner();
	const int32 Local = ReadStance(Owner);
	if (!Owner || Owner->HasAuthority() || MirroredStance < 0 || Local < 0 || Local == MirroredStance)
	{
		return;
	}
	++StanceAttempts;
	// The stance enum's entry name ("Melee") names the Blueprint's own multicast handler (MC_MeleeStance).
	const FProperty* P = Owner->GetClass()->FindPropertyByName(TEXT("Stance"));
	const UEnum* Enum = nullptr;
	if (const FByteProperty* B = CastField<FByteProperty>(P))
	{
		Enum = B->Enum;
	}
	else if (const FEnumProperty* E = CastField<FEnumProperty>(P))
	{
		Enum = E->GetEnum();
	}
	const FString Entry = Enum ? Enum->GetDisplayNameTextByValue(MirroredStance).ToString().Replace(TEXT(" "), TEXT("")) : FString();
	UFunction* Fn = Entry.IsEmpty() ? nullptr : Owner->FindFunction(FName(*FString::Printf(TEXT("MC_%sStance"), *Entry)));
	if (!Fn || Fn->ParmsSize != 0)
	{
		UE_LOG(LogGOTAMontageRep, Warning, TEXT("%s: stance %d (%s) has no MC_%sStance event; local stance stays %d"), *GetNameSafe(Owner), MirroredStance,
			*Entry, *Entry, Local);
		return;
	}
	// Calling a multicast on a client runs it here only: exactly what the lost multicast would have done.
	Owner->ProcessEvent(Fn, nullptr);
	UE_LOG(LogGOTAMontageRep, Log, TEXT("STANCE %s: local %d -> server %d via %s (now %d)"), *GetNameSafe(Owner), Local, MirroredStance, *Fn->GetName(),
		ReadStance(Owner));
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
	// Bosses multicast their own. AI root-motion montages already reach simulated proxies through
	// ACharacter::RepRootMotion. Players: every montage (see the class comment); a client already playing it skips it,
	// which also covers root motion on simulated proxies. IsPlayerControlled is read per montage: a pawn is bootstrapped
	// before it is possessed.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Montage || !Pawn || Pawn->FindComponentByClass<UGOTABossComponent>())
	{
		return false;
	}
	return Pawn->IsPlayerControlled() || !Montage->HasRootMotion();
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
	if (OwnerPawn && OwnerPawn->IsPlayerControlled())
	{
		UE_LOG(LogGOTAMontageRep, Log, TEXT("MREP send #%d %s on %s"), Sent, *Montage->GetName(), *GetNameSafe(GetOwner()));
	}
	Multicast_PlayMontage(Montage, Anim->Montage_GetPlayRate(Montage), Anim->Montage_GetPosition(Montage), Sent);
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

void UGOTAMontageReplicator::Multicast_PlayMontage_Implementation(UAnimMontage* Montage, float PlayRate, float StartPosition, int32 EventId)
{
	if (!GetOwner() || GetOwner()->HasAuthority() || !Montage)
	{
		return; // the server already plays it
	}
	UAnimInstance* Anim = GetAnimInstance();
	if (!Anim)
	{
		return;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const bool bPlayer = Pawn && Pawn->IsPlayerControlled();
	// Already playing: the player Blueprint's own multicast (or the engine's root-motion replication) got here first.
	if (bPlayer && Anim->Montage_IsPlaying(Montage))
	{
		UE_LOG(LogGOTAMontageRep, Log, TEXT("MREP #%d %s on %s (%s): already playing, skipped"), EventId, *Montage->GetName(), *GetNameSafe(Pawn),
			Pawn->IsLocallyControlled() ? TEXT("owner") : TEXT("observer"));
		return;
	}
	Anim->Montage_Play(Montage, PlayRate > 0.f ? PlayRate : 1.f, EMontagePlayReturnType::MontageLength, StartPosition);
	if (bPlayer)
	{
		UE_LOG(LogGOTAMontageRep, Log, TEXT("MREP #%d %s on %s (%s): played"), EventId, *Montage->GetName(), *GetNameSafe(Pawn),
			Pawn->IsLocallyControlled() ? TEXT("owner") : TEXT("observer"));
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
