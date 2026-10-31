// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"
#include "GOTARetargetAnimInstance.generated.h"

class UIKRetargeter;

/**
 * Native proxy: DefaultSlot( Retarget Pose From Mesh ). The pose is copied from the parent skeletal
 * mesh (the hidden combat Mannequin), so every combat montage, notify and hit window keeps running on
 * the original skeleton and the visible body follows it. Hero-specific animation (divine abilities)
 * plays as a montage in DefaultSlot directly on the body's own skeleton and overrides the copied pose
 * while it runs — gameplay timing comes from the server ability timeline, not from these montages.
 */
USTRUCT()
struct FGOTARetargetAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FGOTARetargetAnimInstanceProxy() = default;
	explicit FGOTARetargetAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) { LinkNodes(); }

	void LinkNodes()
	{
		SlotNode.SlotName = FName(TEXT("DefaultSlot"));
		SlotNode.Source.SetLinkNode(&RetargetNode);
	}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual FAnimNode_Base* GetCustomRootNode() override { return &SlotNode; }
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override { OutNodes.Add(&SlotNode); OutNodes.Add(&RetargetNode); }

	FAnimNode_RetargetPoseFromMesh RetargetNode;
	FAnimNode_Slot SlotNode;
};

/** Anim instance for a hero's prototype body. Set Retargeter after creation; no Blueprint needed. */
UCLASS(Transient, NotBlueprintable)
class WAROFGODS_API UGOTARetargetAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Mannequin -> body retargeter (Tools/GOTA_BuildParagonRigs.py). Held here so GC keeps it alive. */
	UPROPERTY(Transient)
	TObjectPtr<UIKRetargeter> Retargeter;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FGOTARetargetAnimInstanceProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete static_cast<FGOTARetargetAnimInstanceProxy*>(InProxy); }
};

/**
 * Native proxy: Copy Pose From Mesh (the attached parent, same skeleton). Used for a body's extra meshes
 * (e.g. hair cards) instead of leader pose, so the extra mesh evaluates its own animation and therefore
 * runs its asset's Post Process Anim Blueprint (e.g. Echo_Hair_AnimBP: rigid-body ponytail).
 */
USTRUCT()
struct FGOTACopyPoseAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FGOTACopyPoseAnimInstanceProxy() = default;
	explicit FGOTACopyPoseAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance)
	{
		CopyNode.bUseAttachedParent = true;
		CopyNode.bCopyCurves = true;
	}

	virtual bool Evaluate(FPoseContext& Output) override
	{
		CopyNode.Evaluate_AnyThread(Output);
		return true;
	}
	virtual FAnimNode_Base* GetCustomRootNode() override { return &CopyNode; }
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override { OutNodes.Add(&CopyNode); }

	FAnimNode_CopyPoseFromMesh CopyNode;
};

/** Anim instance for a hero body's extra meshes: copies the body's pose, keeps the mesh's own post-process. */
UCLASS(Transient, NotBlueprintable)
class WAROFGODS_API UGOTACopyPoseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FGOTACopyPoseAnimInstanceProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete static_cast<FGOTACopyPoseAnimInstanceProxy*>(InProxy); }
};
