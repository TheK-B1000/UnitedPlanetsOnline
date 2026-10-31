// White Tiger Studios 2024

#include "Characters/GOTARetargetAnimInstance.h"
#include "Retargeter/IKRetargeter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTARetargetAnimInstance)

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
