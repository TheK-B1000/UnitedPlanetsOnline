// White Tiger Studios 2024

#include "UI/GOTAGameUserSettings.h"
#include "Engine/Engine.h"

UGOTAGameUserSettings::UGOTAGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UGOTAGameUserSettings* UGOTAGameUserSettings::GetGOTAGameUserSettings()
{
	return Cast<UGOTAGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UGOTAGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	SetVSyncEnabled(bVSyncEnabled);
	if (FrameRateLimitSetting > 0)
	{
		SetFrameRateLimit(static_cast<float>(FrameRateLimitSetting));
	}
	else
	{
		SetFrameRateLimit(0.f);
	}

	Super::ApplySettings(bCheckForCommandLineOverrides);
	ApplyAudioVolumes();
}

void UGOTAGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	MasterVolume = 1.f;
	MusicVolume = 0.8f;
	SFXVolume = 1.f;
	DialogueVolume = 1.f;
	UIVolume = 0.9f;
	MouseSensitivity = 1.f;
	ControllerSensitivity = 1.f;
	bInvertCameraY = false;
	CameraShakeStrength = 1.f;
	bSubtitlesEnabled = true;
	CombatVFXIntensity = 1.f;
	UIScale = 1.f;
	FrameRateLimitSetting = 0;
	bVSyncEnabled = true;
}

void UGOTAGameUserSettings::ApplyAudioVolumes()
{
	// Volumes persist via config. Bind SoundMix / SoundClass assets from WBP_GOTA_Settings
	// when audio assets are authored — avoid null SoundMix overrides here.
}
