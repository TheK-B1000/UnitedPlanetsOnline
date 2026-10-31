// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "GOTAGameUserSettings.generated.h"

/**
 * Project GameUserSettings: display via engine base + GOTA audio/gameplay/accessibility.
 * Set as Engine.GameUserSettingsClassName in DefaultEngine.ini.
 */
UCLASS(config = GameUserSettings, configdonotcheckdefaults)
class WAROFGODS_API UGOTAGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UGOTAGameUserSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Settings")
	static UGOTAGameUserSettings* GetGOTAGameUserSettings();

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
	virtual void SetToDefaults() override;

	// --- Audio (0..1) ---
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float MasterVolume = 1.f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float MusicVolume = 0.8f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float SFXVolume = 1.f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float DialogueVolume = 1.f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Audio")
	float UIVolume = 0.9f;

	// --- Controls ---
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Controls")
	float MouseSensitivity = 1.f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Controls")
	float ControllerSensitivity = 1.f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Controls")
	bool bInvertCameraY = false;

	// --- Gameplay / Accessibility ---
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Gameplay", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CameraShakeStrength = 1.f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Gameplay")
	bool bSubtitlesEnabled = true;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Gameplay", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CombatVFXIntensity = 1.f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Gameplay", meta = (ClampMin = "0.75", ClampMax = "1.5"))
	float UIScale = 1.f;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Display")
	int32 FrameRateLimitSetting = 0;

	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Display")
	bool bVSyncEnabled = true;

	/** Trial of Champions (Fight AI) difficulty: 0 Recruit, 1 Veteran, 2 Ascendant. */
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Gameplay", meta = (ClampMin = "0", ClampMax = "2"))
	int32 FightAIDifficulty = 1;

	/** Skips menu tweens, backdrop drift and camera blends. */
	UPROPERTY(config, BlueprintReadWrite, Category = "Settings|Accessibility")
	bool bReducedMotion = false;

	/**
	 * Applies Master (transient primary volume) and SFX (sound-mix override of the default
	 * sound class). Music and UI are applied by the menu audio owners via SC_GOTA_Music / SC_GOTA_UI.
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void ApplyAudioVolumes();

	/** Scales all UMG via the project DPI curve multiplier. */
	UFUNCTION(BlueprintCallable, Category = "Settings|Accessibility")
	void ApplyUIScale();

	virtual void LoadSettings(bool bForceReload) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<class USoundMix> RuntimeVolumeMix;
};
