// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraModifier_CameraShake.h"
#include "GOTAPlayerCameraManager.generated.h"

/** Scales every camera shake by the player's Camera Shake Intensity setting (0 disables). */
UCLASS()
class WAROFGODS_API UGOTACameraShakeModifier : public UCameraModifier_CameraShake
{
	GENERATED_BODY()

public:
	virtual UCameraShakeBase* AddCameraShake(TSubclassOf<UCameraShakeBase> NewShake, const FAddCameraShakeParams& Params) override;
};

/**
 * Applies player look settings without touching gameplay Blueprints:
 * Look Sensitivity and Invert Y act on the rotation delta before it reaches the view.
 * Assigned as PlayerCameraManagerClass on BP_WarOfGods_PlayerController.
 */
UCLASS()
class WAROFGODS_API AGOTAPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AGOTAPlayerCameraManager();

	virtual void ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;
};
