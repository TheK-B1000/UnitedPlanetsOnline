// White Tiger Studios 2024

#include "UI/GOTAMenuSettingsAdapter.h"
#include "UI/GOTAGameUserSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

UGOTAGameUserSettings* UGOTAMenuSettingsAdapter::GetSettings() const
{
	return UGOTAGameUserSettings::GetGOTAGameUserSettings();
}

void UGOTAMenuSettingsAdapter::CaptureBaseline()
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		const FIntPoint Res = Settings->GetScreenResolution();
		BaselineResolutionX = Res.X;
		BaselineResolutionY = Res.Y;
		BaselineFullscreenMode = static_cast<int32>(Settings->GetFullscreenMode());
		bHasBaseline = true;
		CachedSettings = Settings;
	}
}

bool UGOTAMenuSettingsAdapter::Apply(bool bCheckCommandLine)
{
	UGOTAGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		OnApplyResult.Broadcast(false);
		return false;
	}

	CaptureBaseline();
	Settings->ApplySettings(bCheckCommandLine);
	Settings->SaveSettings();
	OnApplyResult.Broadcast(true);
	NotifyChanged();
	return true;
}

void UGOTAMenuSettingsAdapter::Cancel()
{
	RevertDisplayChanges();
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->LoadSettings(true);
	}
	NotifyChanged();
}

void UGOTAMenuSettingsAdapter::RestoreDefaults()
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->SetToDefaults();
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::ConfirmDisplayChanges()
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->ConfirmVideoMode();
		Settings->SaveSettings();
		CaptureBaseline();
	}
}

void UGOTAMenuSettingsAdapter::RevertDisplayChanges()
{
	if (!bHasBaseline)
	{
		return;
	}
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->SetScreenResolution(FIntPoint(BaselineResolutionX, BaselineResolutionY));
		Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(BaselineFullscreenMode));
		Settings->ApplyResolutionSettings(false);
	}
}

TArray<FIntPoint> UGOTAMenuSettingsAdapter::GetSupportedResolutions() const
{
	TArray<FIntPoint> Results;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Results);
	if (Results.Num() == 0)
	{
		UKismetSystemLibrary::GetConvenientWindowedResolutions(Results);
	}
	return Results;
}

void UGOTAMenuSettingsAdapter::SetResolution(FIntPoint Resolution)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->SetScreenResolution(Resolution);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetFullscreenMode(EWindowMode::Type Mode)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->SetFullscreenMode(Mode);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetOverallQualityPreset(int32 PresetIndex)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->SetOverallScalabilityLevel(FMath::Clamp(PresetIndex, 0, 4));
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetMasterVolume(float Volume)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->MasterVolume = FMath::Clamp(Volume, 0.f, 1.f);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetMusicVolume(float Volume)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->MusicVolume = FMath::Clamp(Volume, 0.f, 1.f);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetSFXVolume(float Volume)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->SFXVolume = FMath::Clamp(Volume, 0.f, 1.f);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetDialogueVolume(float Volume)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->DialogueVolume = FMath::Clamp(Volume, 0.f, 1.f);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetUIVolume(float Volume)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->UIVolume = FMath::Clamp(Volume, 0.f, 1.f);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetMouseSensitivity(float Sensitivity)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->MouseSensitivity = FMath::Max(0.05f, Sensitivity);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetControllerSensitivity(float Sensitivity)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->ControllerSensitivity = FMath::Max(0.05f, Sensitivity);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetInvertCameraY(bool bInvert)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->bInvertCameraY = bInvert;
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetCameraShakeStrength(float Strength)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->CameraShakeStrength = FMath::Clamp(Strength, 0.f, 1.f);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetSubtitlesEnabled(bool bEnabled)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->bSubtitlesEnabled = bEnabled;
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetCombatVFXIntensity(float Intensity)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->CombatVFXIntensity = FMath::Clamp(Intensity, 0.f, 1.f);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetUIScale(float Scale)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->UIScale = FMath::Clamp(Scale, 0.75f, 1.5f);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetVSyncEnabled(bool bEnabled)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->bVSyncEnabled = bEnabled;
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::SetFrameRateLimit(int32 Limit)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->FrameRateLimitSetting = FMath::Max(0, Limit);
		NotifyChanged();
	}
}

void UGOTAMenuSettingsAdapter::NotifyChanged()
{
	OnSettingsChanged.Broadcast();
}
