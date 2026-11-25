// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GOTAHeroBodyAnimSet.generated.h"

class UAnimMontage;
class UAnimSequence;

/**
 * While the combat frame plays SourceMontage, the hero's body shows Sequence instead, at the same progress, so the
 * visible strike stays on the server's hit timing. Drawing only: the montage, its notifies and hit windows are untouched.
 */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTABodyActionMap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") TSoftObjectPtr<UAnimMontage> SourceMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") TSoftObjectPtr<UAnimSequence> Sequence;
	/** Only the spine, arms and head take the clip; legs keep the hero's locomotion (the montage is upper-body too). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") bool bUpperBodyOnly = false;
	/**
	 * Optional timing keys, (montage seconds, clip seconds), ascending: piecewise-linear between them, so each of the
	 * clip's strikes can be pinned to the montage's hit notify. Empty = the whole montage maps onto the whole clip.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") TArray<FVector2D> TimeKeys;
};

/**
 * A hero body's own animation set, for bodies with their own clips on their own skeleton (Ayla's Meshy library).
 * Gameplay is unchanged: the hidden combat frame (BP_Player) still runs every montage, notify, hit window, dodge and
 * block. The body shows its own locomotion from the pawn's real movement and its own clip for each mapped montage;
 * any montage or state it does not know shows the retargeted combat frame as before, so nothing is ever unanimated.
 * A hero without a set is untouched.
 */
UCLASS(BlueprintType)
class WAROFGODS_API UGOTAHeroBodyAnimSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion") TSoftObjectPtr<UAnimSequence> Idle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion") TSoftObjectPtr<UAnimSequence> Walk;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion") TSoftObjectPtr<UAnimSequence> Run;
	/** Ground speed (cm/s) at which Walk and Run play at their authored rate; between them they blend and stretch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion") float WalkSpeed = 160.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion") float RunSpeed = 450.f;

	/** In the air: the clip is held between AirStart and AirEnd (seconds) while the capsule flies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion") TSoftObjectPtr<UAnimSequence> Jump;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion") float JumpAirStart = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion") float JumpAirEnd = 1.2f;

	/** Played once when the combatant is defeated, then held on its last frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") TSoftObjectPtr<UAnimSequence> Death;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat") TArray<FGOTABodyActionMap> Actions;

	/** Seconds to blend between the hero's own clips and the retargeted combat frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body") float BlendTime = 0.15f;
};
