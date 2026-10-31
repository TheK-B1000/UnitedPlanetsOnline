// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "UObject/Object.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuSettingsAdapter.generated.h"

class UGOTAGameUserSettings;

/**
 * Validates and applies supported settings. UI should not write GameUserSettings directly.
 */
UCLASS(BlueprintType)
class WAROFGODS_API UGOTAMenuSettingsAdapter : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void CaptureBaseline();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	bool Apply(bool bCheckCommandLine = false);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void Cancel();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void RestoreDefaults();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void ConfirmDisplayChanges();

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void RevertDisplayChanges();

	UFUNCTION(BlueprintPure, Category = "GOTA|Settings")
	UGOTAGameUserSettings* GetSettings() const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	TArray<FIntPoint> GetSupportedResolutions() const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetResolution(FIntPoint Resolution);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetFullscreenMode(EWindowMode::Type Mode);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetOverallQualityPreset(int32 PresetIndex);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetSFXVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetDialogueVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetUIVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetMouseSensitivity(float Sensitivity);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetControllerSensitivity(float Sensitivity);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetInvertCameraY(bool bInvert);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetCameraShakeStrength(float Strength);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetSubtitlesEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetCombatVFXIntensity(float Intensity);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetUIScale(float Scale);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetVSyncEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetFrameRateLimit(int32 Limit);

	/** Engine scalability group: 0 ViewDistance 1 AA 2 Shadow 3 GI 4 Reflection 5 PostProcess 6 Texture 7 Effects 8 Foliage 9 Shading. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetScalabilityGroup(int32 Group, int32 Level);

	UFUNCTION(BlueprintPure, Category = "GOTA|Settings")
	int32 GetScalabilityGroup(int32 Group) const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Settings")
	void SetReducedMotion(bool bReduced);

	/** Read-only access for presenting current values. Writes go through the setters above. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Settings")
	UGOTAGameUserSettings* GetSettingsForDisplay() const { return GetSettings(); }

	/** True if the last Apply changed resolution or window mode (UI should ask to keep it). */
	UFUNCTION(BlueprintPure, Category = "GOTA|Settings")
	bool DidLastApplyChangeDisplay() const { return bLastApplyChangedDisplay; }

	/** True when DLSS/FSR/XeSS plugins are not present — UI should hide upscaling. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Settings")
	bool IsUpscalingSupported() const { return false; }

	/** Key remapping UI is not wired yet — Controls tab should show Unavailable. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Settings")
	bool AreKeyBindingsEditable() const { return false; }

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Settings")
	FOnGOTASettingsChanged OnSettingsChanged;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Settings")
	FOnGOTASettingsApplyResult OnApplyResult;

protected:
	void NotifyChanged();

	UPROPERTY()
	TObjectPtr<UGOTAGameUserSettings> CachedSettings;

	bool bHasBaseline = false;
	bool bLastApplyChangedDisplay = false;
	int32 BaselineResolutionX = 0;
	int32 BaselineResolutionY = 0;
	int32 BaselineFullscreenMode = 0;
};
