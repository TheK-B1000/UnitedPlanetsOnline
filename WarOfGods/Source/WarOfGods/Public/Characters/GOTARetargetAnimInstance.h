// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "GOTARetargetAnimInstance.generated.h"

class UIKRetargeter;

/**
 * Native proxy whose whole graph is one Retarget Pose From Mesh node. The pose is copied from
 * the parent skeletal mesh (the hidden combat Mannequin), so every combat montage, notify and
 * hit window keeps running on the original skeleton and the visible body simply follows it.
 */
USTRUCT()
struct FGOTARetargetAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FGOTARetargetAnimInstanceProxy() = default;
	explicit FGOTARetargetAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual FAnimNode_Base* GetCustomRootNode() override { return &RetargetNode; }
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override { OutNodes.Add(&RetargetNode); }

	FAnimNode_RetargetPoseFromMesh RetargetNode;
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
