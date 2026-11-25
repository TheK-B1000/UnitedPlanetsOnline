// White Tiger Studios 2024

#include "Characters/GOTARetargetAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Characters/GOTAHeroBodyAnimSet.h"
#include "Combat/GOTACombatantObserver.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Retargeter/IKRetargeter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTARetargetAnimInstance)

DEFINE_LOG_CATEGORY_STATIC(LogGOTABodyLayer, Log, All);

void FGOTARetargetAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	// Must be set before the base class runs the node's game-thread PreUpdate (which copies the source pose).
	if (const UGOTARetargetAnimInstance* Instance = Cast<UGOTARetargetAnimInstance>(InAnimInstance))
	{
		RetargetNode.IKRetargeterAsset = Instance->Retargeter;
	}
	RetargetNode.RetargetFrom = ERetargetSourceMode::ParentSkeletalMeshComponent;
	RetargetNode.bSuppressWarnings = true;
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
}

bool FGOTARetargetAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	SlotNode.Evaluate_AnyThread(Output);
	return true;
}

// ============================================================================ layer node (anim worker)

void FAnimNode_GOTABodyLayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	if (Retarget)
	{
		Retarget->Initialize_AnyThread(Context);
	}
	bUpperBodyBuilt = false;
}

void FAnimNode_GOTABodyLayer::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	if (Retarget)
	{
		Retarget->CacheBones_AnyThread(Context);
	}
	bUpperBodyBuilt = false;
}

void FAnimNode_GOTABodyLayer::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	if (Retarget)
	{
		Retarget->Update_AnyThread(Context);
	}
}

void FAnimNode_GOTABodyLayer::Sample(const UAnimSequence* Seq, float Time, FPoseContext& Out) const
{
	FAnimationPoseData PoseData(Out);
	Seq->GetAnimationPose(PoseData, FAnimExtractContext(static_cast<double>(FMath::Clamp(Time, 0.f, Seq->GetPlayLength())), false));
}

namespace
{
	void BlendInto(FPoseContext& Into, const FPoseContext& Other, float Alpha, const TArray<bool>* Mask)
	{
		Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
		for (FCompactPoseBoneIndex Index : Into.Pose.ForEachBoneIndex())
		{
			if (Mask && (!Mask->IsValidIndex(Index.GetInt()) || !(*Mask)[Index.GetInt()]))
			{
				continue;
			}
			Into.Pose[Index].BlendWith(Other.Pose[Index], Alpha);
		}
	}
}

void FAnimNode_GOTABodyLayer::Evaluate_AnyThread(FPoseContext& Output)
{
	if (!Frame.bActive || !Frame.BaseA)
	{
		// Every hero without its own animation set: exactly the retargeted combat frame, as before.
		if (Retarget)
		{
			Retarget->Evaluate_AnyThread(Output);
		}
		else
		{
			Output.ResetToRefPose();
		}
		return;
	}
	Sample(Frame.BaseA, Frame.TimeA, Output);
	if (Frame.BaseB && Frame.AlphaB > 0.f)
	{
		FPoseContext B(Output);
		Sample(Frame.BaseB, Frame.TimeB, B);
		BlendInto(Output, B, Frame.AlphaB, nullptr);
	}
	if (Retarget && Frame.RetargetWeight > 0.f)
	{
		FPoseContext R(Output);
		Retarget->Evaluate_AnyThread(R);
		BlendInto(Output, R, Frame.RetargetWeight, nullptr);
	}
	if (Frame.Action && Frame.ActionWeight > 0.f)
	{
		if (Frame.bActionUpperBody && (!bUpperBodyBuilt || UpperBody.Num() != Output.Pose.GetNumBones()))
		{
			const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
			const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
			const int32 Spine = Ref.FindBoneIndex(TEXT("spine_01"));
			UpperBody.Init(false, Output.Pose.GetNumBones());
			for (FCompactPoseBoneIndex Index : Output.Pose.ForEachBoneIndex())
			{
				for (int32 Mesh = Bones.MakeMeshPoseIndex(Index).GetInt(); Mesh != INDEX_NONE; Mesh = Ref.GetParentIndex(Mesh))
				{
					if (Mesh == Spine)
					{
						UpperBody[Index.GetInt()] = true;
						break;
					}
				}
			}
			bUpperBodyBuilt = true;
		}
		FPoseContext A(Output);
		Sample(Frame.Action, Frame.ActionTime, A);
		BlendInto(Output, A, Frame.ActionWeight, Frame.bActionUpperBody ? &UpperBody : nullptr);
	}
}

// ============================================================================ game thread: which clips, at what time

void UGOTARetargetAnimInstance::SetAnimSet(UGOTAHeroBodyAnimSet* InSet)
{
	AnimSet = InSet;
	LoadedClips.Reset();
	if (!InSet)
	{
		return;
	}
	for (const TSoftObjectPtr<UAnimSequence>* Ref : { &InSet->Idle, &InSet->Walk, &InSet->Run, &InSet->Jump, &InSet->Death })
	{
		if (UAnimSequence* Seq = Ref->LoadSynchronous())
		{
			LoadedClips.Add(Seq);
		}
	}
	for (const FGOTABodyActionMap& Map : InSet->Actions)
	{
		if (UAnimSequence* Seq = Map.Sequence.LoadSynchronous())
		{
			LoadedClips.Add(Seq);
		}
	}
	UE_LOG(LogGOTABodyLayer, Log, TEXT("BODY LAYER %s: own set %s, %d clips, %d mapped montages"), *GetNameSafe(GetOwningActor()), *GetNameSafe(InSet),
		LoadedClips.Num(), InSet->Actions.Num());
}

void UGOTARetargetAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	Frame = FGOTABodyLayerFrame();
	const ACharacter* Char = Cast<ACharacter>(GetOwningActor());
	const UAnimSequence* Idle = AnimSet ? AnimSet->Idle.Get() : nullptr;
	if (Char && Idle)
	{
		Frame.bActive = true;
		const float BlendRate = 1.f / FMath::Max(AnimSet->BlendTime, 0.01f);
		UAnimInstance* Source = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;

		// Defeated: the death clip once, held on its last frame. Statuses, montages and movement no longer matter.
		const UAnimSequence* Death = AnimSet->Death.Get();
		if (Death && UGOTACombatantObserver::IsCombatantDefeated(Char))
		{
			if (DeathTime < 0.f)
			{
				DeathTime = 0.f;
				UE_LOG(LogGOTABodyLayer, Log, TEXT("BODY LAYER %s: death clip %s"), *GetNameSafe(Char), *GetNameSafe(Death));
			}
			DeathTime += DeltaSeconds;
			Frame.BaseA = Death;
			Frame.TimeA = FMath::Min(DeathTime, Death->GetPlayLength());
		}
		else
		{
			DeathTime = -1.f;

			// Locomotion from the capsule's real movement: idle -> walk -> run, one shared stride phase.
			const UAnimSequence* Walk = AnimSet->Walk.Get();
			const UAnimSequence* Run = AnimSet->Run.Get();
			const float Speed = Char->GetVelocity().Size2D();
			const float WalkSpeed = FMath::Max(AnimSet->WalkSpeed, 1.f);
			const float RunSpeed = FMath::Max(AnimSet->RunSpeed, WalkSpeed + 1.f);
			IdleTime = FMath::Fmod(IdleTime + DeltaSeconds, FMath::Max(Idle->GetPlayLength(), 0.01f));
			const float WalkLen = Walk ? FMath::Max(Walk->GetPlayLength(), 0.01f) : 1.f;
			const float RunLen = Run ? FMath::Max(Run->GetPlayLength(), 0.01f) : 1.f;
			float CyclesPerSecond;
			if (Speed <= WalkSpeed || !Run)
			{
				Frame.BaseA = Idle;
				Frame.TimeA = IdleTime;
				Frame.BaseB = Walk;
				Frame.AlphaB = Walk ? FMath::Clamp(Speed / WalkSpeed, 0.f, 1.f) : 0.f;
				CyclesPerSecond = FMath::Max(Speed / WalkSpeed, 0.5f) / WalkLen;
			}
			else
			{
				const float Alpha = FMath::Clamp((Speed - WalkSpeed) / (RunSpeed - WalkSpeed), 0.f, 1.f);
				Frame.BaseA = Walk;
				Frame.BaseB = Run;
				Frame.AlphaB = Alpha;
				CyclesPerSecond = FMath::Lerp(1.f / WalkLen, 1.f / RunLen, Alpha) * FMath::Max(Speed / RunSpeed, 1.f);
			}
			StridePhase = FMath::Fmod(StridePhase + DeltaSeconds * CyclesPerSecond, 1.f);
			if (Frame.BaseA == Walk)
			{
				Frame.TimeA = StridePhase * WalkLen;
			}
			Frame.TimeB = StridePhase * (Frame.BaseB == Run ? RunLen : WalkLen);

			// In the air: the jump clip's airborne part, blended over whichever stride is stronger.
			const UCharacterMovementComponent* Move = Char->GetCharacterMovement();
			const bool bAir = Move && Move->IsFalling();
			AirTime = bAir ? AirTime + DeltaSeconds : 0.f;
			AirWeight = FMath::FInterpConstantTo(AirWeight, bAir ? 1.f : 0.f, DeltaSeconds, BlendRate);
			if (const UAnimSequence* Jump = AnimSet->Jump.Get(); Jump && AirWeight > 0.f)
			{
				if (Frame.AlphaB >= 0.5f && Frame.BaseB)
				{
					Frame.BaseA = Frame.BaseB;
					Frame.TimeA = Frame.TimeB;
				}
				Frame.BaseB = Jump;
				Frame.TimeB = FMath::Clamp(AnimSet->JumpAirStart + AirTime, AnimSet->JumpAirStart, AnimSet->JumpAirEnd);
				Frame.AlphaB = AirWeight;
			}

			// Combat montages on the hidden frame: the mapped clip at the same progress, or the retargeted frame.
			float FallbackTarget = 0.f;
			const FAnimMontageInstance* Inst = Source ? Source->GetActiveMontageInstance() : nullptr;
			const UAnimMontage* Montage = Inst ? Inst->Montage.Get() : nullptr;
			if (Montage)
			{
				const FSoftObjectPath MontagePath(Montage);
				const FGOTABodyActionMap* Map = AnimSet->Actions.FindByPredicate([&](const FGOTABodyActionMap& M) { return M.SourceMontage.ToSoftObjectPath() == MontagePath; });
				const UAnimSequence* Clip = Map ? Map->Sequence.Get() : nullptr;
				const float Weight = Inst->GetWeight();
				if (Clip)
				{
					const float Pos = Inst->GetPosition();
					const float ClipLen = Clip->GetPlayLength();
					float T = ClipLen * Pos / FMath::Max(Montage->GetPlayLength(), 0.01f);
					if (Map->TimeKeys.Num() >= 2)
					{
						// Piecewise-linear: each key pins a moment of the montage (a hit notify) to a moment of the clip.
						const TArray<FVector2D>& K = Map->TimeKeys;
						int32 i = 0;
						while (i < K.Num() - 2 && Pos > K[i + 1].X)
						{
							++i;
						}
						const float Span = FMath::Max(static_cast<float>(K[i + 1].X - K[i].X), 0.001f);
						T = FMath::Lerp(static_cast<float>(K[i].Y), static_cast<float>(K[i + 1].Y), FMath::Clamp((Pos - static_cast<float>(K[i].X)) / Span, 0.f, 1.f));
					}
					Frame.Action = Clip;
					Frame.ActionTime = T;
					Frame.ActionWeight = Weight;
					Frame.bActionUpperBody = Map->bUpperBodyOnly;
					if (LastAction.Get() != Clip)
					{
						UE_LOG(LogGOTABodyLayer, Log, TEXT("BODY LAYER %s: %s -> own clip %s%s"), *GetNameSafe(Char), *Montage->GetName(), *Clip->GetName(),
							Map->bUpperBodyOnly ? TEXT(" (upper body)") : TEXT(""));
					}
					LastAction = Clip;
				}
				else
				{
					FallbackTarget = Weight;
					if (LastSourceState != Montage->GetFName())
					{
						UE_LOG(LogGOTABodyLayer, Log, TEXT("BODY LAYER %s: %s has no own clip, showing the retargeted combat frame"), *GetNameSafe(Char), *Montage->GetName());
					}
				}
				LastSourceState = Montage->GetFName();
			}
			else
			{
				LastAction.Reset();
				LastSourceState = NAME_None;
			}
			FallbackWeight = FMath::FInterpConstantTo(FallbackWeight, FallbackTarget, DeltaSeconds, BlendRate);
			Frame.RetargetWeight = FallbackWeight;
		}
	}
	GetProxyOnGameThread<FGOTARetargetAnimInstanceProxy>().LayerNode.Frame = Frame;
}
