// White Tiger Studios 2024

#include "UI/GOTAGameUserSettings.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/UserInterfaceSettings.h"
#include "Sound/AudioSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UGOTAGameUserSettings::UGOTAGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UGOTAGameUserSettings* UGOTAGameUserSettings::GetGOTAGameUserSettings()
{
	return GEngine ? Cast<UGOTAGameUserSettings>(UGameUserSettings::GetGameUserSettings()) : nullptr;
}

void UGOTAGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);
	// Non-display settings take effect at boot without waiting for an Apply.
	ApplyUIScale();
}

void UGOTAGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	SetVSyncEnabled(bVSyncEnabled);
	SetFrameRateLimit(FrameRateLimitSetting > 0 ? static_cast<float>(FrameRateLimitSetting) : 0.f);

	Super::ApplySettings(bCheckForCommandLineOverrides);
	ApplyAudioVolumes();
	ApplyUIScale();
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
	bReducedMotion = false;
}

void UGOTAGameUserSettings::ApplyAudioVolumes()
{
	if (!GEngine)
	{
		return;
	}
	FAudioDeviceHandle Device = GEngine->GetMainAudioDevice();
	if (!Device)
	{
		return;
	}

	Device->SetTransientPrimaryVolume(FMath::Clamp(MasterVolume, 0.f, 1.f));

	USoundClass* DefaultClass = GetDefault<UAudioSettings>()->GetDefaultSoundClass();
	if (!DefaultClass)
	{
		return;
	}
	if (!RuntimeVolumeMix)
	{
		RuntimeVolumeMix = NewObject<USoundMix>(this, TEXT("GOTA_RuntimeVolumeMix"));
	}
	// Gameplay SFX ride the project default sound class. Music/UI use their own classes
	// (no parent) so this override never double-attenuates them.
	Device->SetSoundMixClassOverride(RuntimeVolumeMix, DefaultClass, FMath::Clamp(SFXVolume, 0.f, 1.f), 1.f, 0.1f, true);
	Device->PushSoundMixModifier(RuntimeVolumeMix);
}

void UGOTAGameUserSettings::ApplyUIScale()
{
	if (UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>())
	{
		UISettings->ApplicationScale = FMath::Clamp(UIScale, 0.75f, 1.5f);
	}
}
