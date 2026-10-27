// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuCameraDirector.generated.h"

class ACameraActor;
class ACineCameraActor;
class UCameraComponent;

USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAMenuCameraShot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	EGOTAMenuScreen Screen = EGOTAMenuScreen::MainMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<AActor> CameraActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BlendTime = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunction = VTBlend_EaseInOut;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float BlendExp = 2.f;
};

/**
 * Place in L_GOTA_MainMenu. Owns named cinematic shots and blends the local PC view target.
 * Supports skip so UI never waits on unfinished blends.
 */
UCLASS(Blueprintable)
class WAROFGODS_API AGOTAMenuCameraDirector : public AActor
{
	GENERATED_BODY()

public:
	AGOTAMenuCameraDirector();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Camera")
	void RegisterShot(EGOTAMenuScreen Screen, AActor* Camera, float BlendTime = 1.25f);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Camera")
	bool TransitionTo(EGOTAMenuScreen Screen, bool bSkipBlend = false);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Camera")
	void SkipActiveTransition();

	UFUNCTION(BlueprintPure, Category = "GOTA|Camera")
	EGOTAMenuScreen GetActiveScreen() const { return ActiveScreen; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Camera")
	bool IsTransitioning() const { return bIsTransitioning; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Camera")
	TArray<FGOTAMenuCameraShot> Shots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Camera")
	EGOTAMenuScreen StartupScreen = EGOTAMenuScreen::MainMenu;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Camera")
	FOnGOTAMenuTransitionFinished OnTransitionFinished;

protected:
	const FGOTAMenuCameraShot* FindShot(EGOTAMenuScreen Screen) const;
	void FinishTransition();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GOTA|Camera")
	EGOTAMenuScreen ActiveScreen = EGOTAMenuScreen::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GOTA|Camera")
	bool bIsTransitioning = false;

	FTimerHandle TransitionTimer;
};
