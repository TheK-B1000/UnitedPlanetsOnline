// White Tiger Studios 2024

#include "UI/GOTAPlayerCameraManager.h"
#include "UI/GOTAGameUserSettings.h"

UCameraShakeBase* UGOTACameraShakeModifier::AddCameraShake(TSubclassOf<UCameraShakeBase> NewShake, const FAddCameraShakeParams& Params)
{
	const UGOTAGameUserSettings* Settings = UGOTAGameUserSettings::GetGOTAGameUserSettings();
	const float Strength = Settings ? FMath::Clamp(Settings->CameraShakeStrength, 0.f, 1.f) : 1.f;
	if (Strength <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}
	FAddCameraShakeParams Scaled = Params;
	Scaled.Scale *= Strength;
	return Super::AddCameraShake(NewShake, Scaled);
}

AGOTAPlayerCameraManager::AGOTAPlayerCameraManager()
{
	for (TSubclassOf<UCameraModifier>& Modifier : DefaultModifiers)
	{
		if (Modifier == UCameraModifier_CameraShake::StaticClass())
		{
			Modifier = UGOTACameraShakeModifier::StaticClass();
		}
	}
	DefaultModifiers.AddUnique(UGOTACameraShakeModifier::StaticClass());
}

void AGOTAPlayerCameraManager::ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
	if (const UGOTAGameUserSettings* Settings = UGOTAGameUserSettings::GetGOTAGameUserSettings())
	{
		const float Sensitivity = FMath::Clamp(Settings->MouseSensitivity, 0.05f, 5.f);
		OutDeltaRot.Yaw *= Sensitivity;
		OutDeltaRot.Pitch *= Sensitivity * (Settings->bInvertCameraY ? -1.f : 1.f);
	}
	Super::ProcessViewRotation(DeltaTime, OutViewRotation, OutDeltaRot);
}

void AGOTAPlayerCameraManager::AddImpactKick(float Strength)
{
	const UGOTAGameUserSettings* Settings = UGOTAGameUserSettings::GetGOTAGameUserSettings();
	const float Scale = Settings ? (Settings->bReducedMotion ? 0.f : FMath::Clamp(Settings->CameraShakeStrength, 0.f, 1.f)) : 1.f;
	const float Scaled = Strength * Scale;
	if (Scaled > KickStrength * FMath::Max(0.f, 1.f - KickAge / 0.28f))
	{
		KickStrength = Scaled;
		KickAge = 0.f;
		KickSeed = FMath::FRandRange(0.f, 100.f);
	}
}

void AGOTAPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);
	static constexpr float KickDuration = 0.28f;
	if (KickStrength <= 0.f)
	{
		return;
	}
	KickAge += DeltaTime;
	const float Alpha = FMath::Clamp(1.f - KickAge / KickDuration, 0.f, 1.f);
	if (Alpha <= 0.f)
	{
		KickStrength = 0.f;
		return;
	}
	// Decaying, high-frequency rotation jitter plus a small FOV punch.
	const float A = KickStrength * Alpha * Alpha;
	const float T = KickAge * 38.f + KickSeed;
	OutVT.POV.Rotation.Pitch += FMath::Sin(T * 1.3f) * 0.9f * A;
	OutVT.POV.Rotation.Yaw += FMath::Sin(T * 0.9f + 1.7f) * 0.7f * A;
	OutVT.POV.Rotation.Roll += FMath::Sin(T * 1.7f + 0.4f) * 0.5f * A;
	OutVT.POV.FOV = FMath::Clamp(OutVT.POV.FOV - 2.5f * A, 5.f, 170.f);
}
