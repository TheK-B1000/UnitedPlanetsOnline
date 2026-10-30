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

	if (!bHasBaseline)
	{
		CaptureBaseline();
	}
	// Compare pending display values to the last *applied* baseline (not to themselves).
	const FIntPoint PendingRes = Settings->GetScreenResolution();
	const int32 PendingMode = static_cast<int32>(Settings->GetFullscreenMode());
	bLastApplyChangedDisplay = PendingRes.X != BaselineResolutionX || PendingRes.Y != BaselineResolutionY
		|| PendingMode != BaselineFullscreenMode;

	Settings->ApplySettings(bCheckCommandLine);
	Settings->SaveSettings();
	if (!bLastApplyChangedDisplay)
	{
		CaptureBaseline();
	}
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
	bLastApplyChangedDisplay = false;
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
		Settings->SaveSettings();
	}
	bLastApplyChangedDisplay = false;
	NotifyChanged();
}

void UGOTAMenuSettingsAdapter::SetScalabilityGroup(int32 Group, int32 Level)
{
	UGOTAGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return;
	}
	const int32 L = FMath::Clamp(Level, 0, 4);
	switch (Group)
	{
	case 0: Settings->SetViewDistanceQuality(L); break;
	case 1: Settings->SetAntiAliasingQuality(L); break;
	case 2: Settings->SetShadowQuality(L); break;
	case 3: Settings->SetGlobalIlluminationQuality(L); break;
	case 4: Settings->SetReflectionQuality(L); break;
	case 5: Settings->SetPostProcessingQuality(L); break;
	case 6: Settings->SetTextureQuality(L); break;
	case 7: Settings->SetVisualEffectQuality(L); break;
	case 8: Settings->SetFoliageQuality(L); break;
	case 9: Settings->SetShadingQuality(L); break;
	default: return;
	}
	NotifyChanged();
}

int32 UGOTAMenuSettingsAdapter::GetScalabilityGroup(int32 Group) const
{
	const UGOTAGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return 0;
	}
	switch (Group)
	{
	case 0: return Settings->GetViewDistanceQuality();
	case 1: return Settings->GetAntiAliasingQuality();
	case 2: return Settings->GetShadowQuality();
	case 3: return Settings->GetGlobalIlluminationQuality();
	case 4: return Settings->GetReflectionQuality();
	case 5: return Settings->GetPostProcessingQuality();
	case 6: return Settings->GetTextureQuality();
	case 7: return Settings->GetVisualEffectQuality();
	case 8: return Settings->GetFoliageQuality();
	case 9: return Settings->GetShadingQuality();
	default: return 0;
	}
}

void UGOTAMenuSettingsAdapter::SetReducedMotion(bool bReduced)
{
	if (UGOTAGameUserSettings* Settings = GetSettings())
	{
		Settings->bReducedMotion = bReduced;
		NotifyChanged();
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
