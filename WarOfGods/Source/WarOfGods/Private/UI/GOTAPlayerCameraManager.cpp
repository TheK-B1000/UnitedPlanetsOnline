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
