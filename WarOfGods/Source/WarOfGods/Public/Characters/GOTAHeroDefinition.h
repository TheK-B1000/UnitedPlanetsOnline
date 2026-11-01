// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Arenas/ArenaTypes.h"
#include "GOTAHeroDefinition.generated.h"

class UTexture2D;
class USkeletalMesh;
class UIKRetargeter;

/** Whether a hero can actually be played. Never inferred from the existence of artwork. */
UENUM(BlueprintType)
enum class EGOTAHeroAvailability : uint8
{
	/** Has a validated gameplay configuration (pawn, combat) — may be spawned. */
	Playable,
	/** Card/lore approved; no gameplay character exists yet. Viewable, not selectable for matches. */
	PreviewOnly,
	/** Gameplay character partially built; not selectable. */
	InDevelopment,
	/** Requires a real unlock system (none implemented yet). */
	Locked
};

/** Presentation-only card values (0–10). NOT combat multipliers — see GOTA_HERO_ROSTER.md. */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAHeroCardStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Attack = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Defense = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Mobility = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Technique = 0;
};

/** One "Divine Ability" as presented on the approved card (presentation, not gameplay). */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAHeroAbilityInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * What a hero IS. One asset per hero in /Game/Heroes/Data (primary asset type "GOTAHero").
 * Editorial lore/presentation is kept separate from the authoritative gameplay block.
 * Large art is soft-referenced so the roster never loads every portrait at once.
 */
UCLASS(BlueprintType)
class WAROFGODS_API UGOTAHeroDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("GOTAHero"), HeroId); }

	UFUNCTION(BlueprintPure, Category = "Hero")
	bool IsPlayable() const { return Availability == EGOTAHeroAvailability::Playable && !GameplayPawnClass.IsNull(); }

	// ---------------------------------------------------------------- Identity (stable)
	/** Stable id used for replication, save data and URLs. Never derived from the display name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName HeroId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	int32 SortOrder = 0;

	// ---------------------------------------------------------------- Editorial (from approved card)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText DisplayName;

	/** e.g. "DESERT STORM". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText Quote;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore", meta = (MultiLine = true))
	FText Biography;

	/** Empty = not stated on the approved card (shown as "UNDISCLOSED", never invented). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText Faction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText ClassLabel;

	/** e.g. "MOBILITY • ELEMENTAL • PRECISION" — only where the card states it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText Specialization;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FText WeaponStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	TArray<FGOTAHeroAbilityInfo> Abilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lore")
	FGOTAHeroCardStats CardStats;

	// ---------------------------------------------------------------- Presentation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	/** Defaults to Portrait when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UTexture2D> LoadingArt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FLinearColor AccentColor = FLinearColor(0.78f, 0.55f, 0.22f, 1.f);

	// ---------------------------------------------------------------- Gameplay (authoritative)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	EGOTAHeroAvailability Availability = EGOTAHeroAvailability::PreviewOnly;

	/** Pawn the SERVER spawns for this hero. Empty = no gameplay character (cannot be Playable). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftClassPtr<APawn> GameplayPawnClass;

	/** Modes this hero may enter. Empty = all modes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TArray<EArenaMode> AllowedModes;

	/** Why the hero is not playable (shown verbatim in the selection UI). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	FText UnavailableReason;

	// ---------------------------------------------------------------- Body (cosmetic, prototype)
	/**
	 * Visible body worn over the gameplay pawn. The pawn's own mesh stays hidden but keeps animating,
	 * so combat (montages, hit windows, notifies) is unchanged; this mesh follows it through
	 * BodyRetargeter. Empty = the pawn's own mesh is shown. Paragon meshes here are PROTOTYPE bodies.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	/** Retargeter from the pawn's skeleton to BodyMesh (Tools/GOTA_BuildParagonRigs.py). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TSoftObjectPtr<UIKRetargeter> BodyRetargeter;

	/** Uniform scale of the body relative to the pawn mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.3", ClampMax = "2.0"))
	float BodyScale = 1.f;

	/** Hide meshes attached to the pawn (e.g. the shared prototype sword) when the body carries its own weapon. Cosmetic only — collision is untouched. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	bool bHideAttachedWeaponMeshes = true;

	/** Divine abilities (slots E/R/C + ultimate V). Empty = no hero abilities (shared kit only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TSoftObjectPtr<class UGOTAAbilitySet> AbilitySet;

	/** Extra meshes on the body's skeleton (e.g. Echo's hair cards); they follow the body's pose (leader pose). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TArray<TSoftObjectPtr<USkeletalMesh>> BodyExtraMeshes;

	/** Material slots of BodyMesh to hide (e.g. a body's own weapon, so only the gameplay weapon shows). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TArray<FName> BodyHiddenMaterialSlots;

	/** Material used for hidden slots (an invisible/masked-out material). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TSoftObjectPtr<class UMaterialInterface> BodyHiddenMaterial;

	/** Where the body comes from, for credits/QA (e.g. "Paragon: Greystone (prototype)"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FText BodySourceNote;
};
