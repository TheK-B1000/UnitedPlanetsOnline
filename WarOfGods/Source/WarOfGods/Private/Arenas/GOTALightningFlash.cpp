// White Tiger Studios 2024

#include "Arenas/GOTALightningFlash.h"

#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/GOTAGameUserSettings.h"

AGOTALightningFlash::AGOTALightningFlash()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	Flash = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Flash"));
	RootComponent = Flash;
	Flash->SetMobility(EComponentMobility::Movable);
	Flash->SetIntensity(0.f);
	Flash->SetLightColor(FLinearColor(0.75f, 0.82f, 1.f));
	Flash->SetCastShadows(false);
	Flash->bAffectsWorld = true;
}

void AGOTALightningFlash::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}
	ScheduleNext();
}

void AGOTALightningFlash::ScheduleNext()
{
	NextStrikeTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(IntervalSeconds.X, IntervalSeconds.Y);
}

void AGOTALightningFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Now = GetWorld()->GetTimeSeconds();
	const UGOTAGameUserSettings* Settings = UGOTAGameUserSettings::GetGOTAGameUserSettings();
	const bool bReduced = Settings && Settings->bReducedMotion;

	if (StrikeStart < 0.0 && Now >= NextStrikeTime)
	{
		StrikeStart = Now;
		StrikeScale = FMath::FRandRange(0.5f, 1.f);
		SetActorRotation(FRotator(FMath::FRandRange(-60.f, -25.f), FMath::FRandRange(0.f, 360.f), 0.f));
		bThunderPending = Thunder != nullptr;
		ThunderTime = Now + FMath::FRandRange(0.6, 2.5);
	}
	float Intensity = 0.f;
	if (StrikeStart >= 0.0)
	{
		// Double flicker: 0–0.07 s, gap, 0.14–0.24 s, then a soft tail.
		const double T = Now - StrikeStart;
		if (T < 0.07) { Intensity = 1.f; }
		else if (T < 0.14) { Intensity = 0.15f; }
		else if (T < 0.24) { Intensity = 0.8f; }
		else if (T < 0.6) { Intensity = 0.8f * FMath::Max(0.f, 1.f - float((T - 0.24) / 0.36)); }
		else
		{
			StrikeStart = -1.0;
			ScheduleNext();
		}
	}
	Flash->SetIntensity(bReduced ? 0.f : Intensity * StrikeScale * PeakIntensity);
	if (bThunderPending && Now >= ThunderTime)
	{
		bThunderPending = false;
		UGameplayStatics::PlaySound2D(this, Thunder, ThunderVolume * StrikeScale);
	}
}
