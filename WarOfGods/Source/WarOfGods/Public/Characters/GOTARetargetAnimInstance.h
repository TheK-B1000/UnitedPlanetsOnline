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
class UAnimSequence;
class UGOTAHeroBodyAnimSet;

/** One frame of a body's own clips, decided on the game thread and read by the layer node on the anim worker. */
struct FGOTABodyLayerFrame
{
	bool bActive = false;              // false: the body is exactly the retargeted combat frame (every hero without a set)
	const UAnimSequence* BaseA = nullptr;
	float TimeA = 0.f;
	const UAnimSequence* BaseB = nullptr;
	float TimeB = 0.f;
	float AlphaB = 0.f;                // blend from BaseA to BaseB (walk/run, or into the jump)
	float RetargetWeight = 0.f;        // how much of the retargeted combat frame shows over the base (unmapped actions)
	const UAnimSequence* Action = nullptr;
	float ActionTime = 0.f;
	float ActionWeight = 0.f;
	bool bActionUpperBody = false;
};

/**
 * The body's base pose: the retargeted combat frame, or (for a body with its own animation set) the body's own clips,
 * blended with the retargeted frame where no clip is mapped. Evaluates poses only; it never moves anything.
 */
USTRUCT()
struct FAnimNode_GOTABodyLayer : public FAnimNode_Base
{
	GENERATED_BODY()

	FAnimNode_RetargetPoseFromMesh* Retarget = nullptr;
	FGOTABodyLayerFrame Frame;

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;

private:
	void Sample(const UAnimSequence* Seq, float Time, FPoseContext& Out) const;
	/** Compact-pose indices of the spine, arms, neck and head (descendants of spine_01). */
	TArray<bool> UpperBody;
	bool bUpperBodyBuilt = false;
};

/**
 * Native proxy: DefaultSlot( BodyLayer( Retarget Pose From Mesh ) ). The pose is copied from the parent skeletal
 * mesh (the hidden combat Mannequin), so every combat montage, notify and hit window keeps running on
 * the original skeleton and the visible body follows it. Hero-specific animation (divine abilities)
 * plays as a montage in DefaultSlot directly on the body's own skeleton and overrides the copied pose
 * while it runs — gameplay timing comes from the server ability timeline, not from these montages.
 * BodyLayer only changes anything for a body with its own animation set (UGOTAHeroBodyAnimSet).
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
		LayerNode.Retarget = &RetargetNode;
		SlotNode.Source.SetLinkNode(&LayerNode);
	}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual FAnimNode_Base* GetCustomRootNode() override { return &SlotNode; }
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override { OutNodes.Add(&SlotNode); OutNodes.Add(&LayerNode); OutNodes.Add(&RetargetNode); }

	FAnimNode_RetargetPoseFromMesh RetargetNode;
	FAnimNode_GOTABodyLayer LayerNode;
	FAnimNode_Slot SlotNode;
};

/** Anim instance for a hero's prototype body. Set Retargeter (and AnimSet, if any) after creation; no Blueprint needed. */
UCLASS(Transient, NotBlueprintable)
class WAROFGODS_API UGOTARetargetAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Mannequin -> body retargeter (Tools/GOTA_BuildParagonRigs.py). Held here so GC keeps it alive. */
	UPROPERTY(Transient)
	TObjectPtr<UIKRetargeter> Retargeter;

	/** The body's own clips (optional). Loaded with the body; held here so GC keeps them alive. */
	UPROPERTY(Transient)
	TObjectPtr<UGOTAHeroBodyAnimSet> AnimSet;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedClips;

	/** Loads the set's clips (game thread, once). */
	void SetAnimSet(UGOTAHeroBodyAnimSet* InSet);

	/** Game thread: this frame's choice of clips. */
	FGOTABodyLayerFrame GetLayerFrame() const { return Frame; }

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FGOTARetargetAnimInstanceProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete static_cast<FGOTARetargetAnimInstanceProxy*>(InProxy); }
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	FGOTABodyLayerFrame Frame;
	float IdleTime = 0.f;
	float StridePhase = 0.f;   // walk and run share one normalized phase so the blend between them stays in step
	float AirTime = 0.f;
	float AirWeight = 0.f;
	float DeathTime = -1.f;
	float FallbackWeight = 0.f;
	float ActionWeightSmoothed = 0.f;
	TWeakObjectPtr<const UAnimSequence> LastAction;
	float LastActionTime = 0.f;
	FName LastSourceState;
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
