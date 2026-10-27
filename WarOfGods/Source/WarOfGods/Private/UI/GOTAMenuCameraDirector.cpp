// White Tiger Studios 2024

#include "UI/GOTAMenuCameraDirector.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AGOTAMenuCameraDirector::AGOTAMenuCameraDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void AGOTAMenuCameraDirector::BeginPlay()
{
	Super::BeginPlay();
	TransitionTo(StartupScreen, true);
}

void AGOTAMenuCameraDirector::RegisterShot(EGOTAMenuScreen Screen, AActor* Camera, float BlendTime)
{
	if (!Camera)
	{
		return;
	}

	for (FGOTAMenuCameraShot& Shot : Shots)
	{
		if (Shot.Screen == Screen)
		{
			Shot.CameraActor = Camera;
			Shot.BlendTime = BlendTime;
			return;
		}
	}

	FGOTAMenuCameraShot NewShot;
	NewShot.Screen = Screen;
	NewShot.CameraActor = Camera;
	NewShot.BlendTime = BlendTime;
	Shots.Add(NewShot);
}

bool AGOTAMenuCameraDirector::TransitionTo(EGOTAMenuScreen Screen, bool bSkipBlend)
{
	const FGOTAMenuCameraShot* Shot = FindShot(Screen);
	if (!Shot || !Shot->CameraActor)
	{
		return false;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return false;
	}

	GetWorldTimerManager().ClearTimer(TransitionTimer);

	const float BlendTime = bSkipBlend ? 0.f : Shot->BlendTime;
	bIsTransitioning = BlendTime > KINDA_SMALL_NUMBER;
	ActiveScreen = Screen;

	FViewTargetTransitionParams Params;
	Params.BlendTime = BlendTime;
	Params.BlendFunction = Shot->BlendFunction;
	Params.BlendExp = Shot->BlendExp;
	Params.bLockOutgoing = false;
	PC->SetViewTarget(Shot->CameraActor, Params);

	if (bIsTransitioning)
	{
		GetWorldTimerManager().SetTimer(
			TransitionTimer,
			this,
			&AGOTAMenuCameraDirector::FinishTransition,
			BlendTime,
			false);
	}
	else
	{
		FinishTransition();
	}

	return true;
}

void AGOTAMenuCameraDirector::SkipActiveTransition()
{
	if (!bIsTransitioning)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TransitionTimer);

	if (const FGOTAMenuCameraShot* Shot = FindShot(ActiveScreen))
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			FViewTargetTransitionParams Instant;
			Instant.BlendTime = 0.f;
			PC->SetViewTarget(Shot->CameraActor, Instant);
		}
	}

	FinishTransition();
}

const FGOTAMenuCameraShot* AGOTAMenuCameraDirector::FindShot(EGOTAMenuScreen Screen) const
{
	for (const FGOTAMenuCameraShot& Shot : Shots)
	{
		if (Shot.Screen == Screen)
		{
			return &Shot;
		}
	}
	return nullptr;
}

void AGOTAMenuCameraDirector::FinishTransition()
{
	bIsTransitioning = false;
	OnTransitionFinished.Broadcast();
}
