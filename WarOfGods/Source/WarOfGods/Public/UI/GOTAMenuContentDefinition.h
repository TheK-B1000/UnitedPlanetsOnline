// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Arenas/ArenaTypes.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuContentDefinition.generated.h"

class UTexture2D;
class USoundBase;
class USoundClass;
class USoundMix;
class UFont;

/** Design tokens shared by every frontend widget. Edit here, not in widgets. */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAMenuTheme
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor Background = FLinearColor(0.006f, 0.009f, 0.022f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor Surface = FLinearColor(0.012f, 0.016f, 0.032f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor Gold = FLinearColor(0.78f, 0.55f, 0.22f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor GoldBright = FLinearColor(1.f, 0.82f, 0.48f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor TechBlue = FLinearColor(0.25f, 0.55f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor Text = FLinearColor(0.93f, 0.9f, 0.84f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor TextMuted = FLinearColor(0.56f, 0.6f, 0.68f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor TextDisabled = FLinearColor(0.32f, 0.33f, 0.37f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor Danger = FLinearColor(0.62f, 0.12f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FLinearColor Success = FLinearColor(0.3f, 0.75f, 0.85f, 1.f);

	/** Seconds for focus/hover transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (ClampMin = "0.01"))
	float FocusBlendSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (ClampMin = "0.01"))
	float ScreenEnterSeconds = 0.28f;

	/** Pixels a focused feature card lifts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
	float CardLift = 8.f;
};

/** One frontend destination (left navigation row + optional bottom feature card). */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAMenuDestination
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destination")
	EGOTAMenuAction Action = EGOTAMenuAction::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destination")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destination")
	FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destination")
	FText CardSubtitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destination")
	FText Tooltip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destination")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destination")
	TSoftObjectPtr<UTexture2D> CardArt;
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTALoreEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore", meta = (MultiLine = true))
	FText Body;
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAStoryChapterInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FName ChapterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story", meta = (MultiLine = true))
	FText Summary;

	/** Only chapters with a real map + GameMode path may be playable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	bool bPlayable = false;
};

/**
 * Designer-owned frontend content: copy, art, audio, theme, lore.
 * Asset: /Game/MainMenu/Data/DA_GOTA_MenuContent (authored by Tools/GOTA_BuildMenuUI.py).
 */
UCLASS(BlueprintType)
class WAROFGODS_API UGOTAMenuContentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool FindDestination(EGOTAMenuAction Action, FGOTAMenuDestination& OutDestination) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	FGOTAMenuTheme Theme;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destinations")
	TArray<FGOTAMenuDestination> Destinations;

	// --- Identity copy ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText FactionName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText FactionTagline;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText GameSubtitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TArray<FText> InstitutionalMessages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DefaultRankTitle;

	// --- Story ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	FText CampaignTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story", meta = (MultiLine = true))
	FText CampaignSynopsis;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
	TArray<FGOTAStoryChapterInfo> Chapters;

	// --- Arena ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	TMap<EArenaId, TSoftObjectPtr<UTexture2D>> ArenaPreviews;

	/** Flip on only after Horde passes a full PIE wave run from the menu (see GOTA_UPG_MENU_TEST_RESULTS). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	bool bHordeLaunchEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena")
	FText HordeUnavailableReason;

	// --- Extras ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Extras")
	TArray<FGOTALoreEntry> LoreEntries;

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> MenuMusic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> MenuAmbience;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> FocusCue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> ConfirmCue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> BackCue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> ErrorCue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundBase> TransitionCue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MusicBaseVolume = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AmbienceBaseVolume = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float UIBaseVolume = 0.6f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("GOTAMenuContent"), GetFName());
	}
};
