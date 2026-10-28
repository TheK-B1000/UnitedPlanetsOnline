// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Arenas/ArenaTypes.h"

class UArenaVisualDressDefinition;
class UObject;

/**
 * Resolves biome dress without hardcoding Marketplace paths inside each Build* layout.
 * Prefer Editor DataAsset; fall back to transient soft refs when the DA is missing.
 */
namespace ArenaVisualDressDefaults
{
	WAROFGODS_API FSoftObjectPath GetCanonicalDressPath(EArenaGrayboxLayout Layout);

	WAROFGODS_API void FillTitansQuarryPilot(UArenaVisualDressDefinition& Dress);
	WAROFGODS_API void FillAshfallPilot(UArenaVisualDressDefinition& Dress);
	WAROFGODS_API void FillFrostboundPilot(UArenaVisualDressDefinition& Dress);
	WAROFGODS_API void FillVerdantPilot(UArenaVisualDressDefinition& Dress);
	WAROFGODS_API void FillSunscarPilot(UArenaVisualDressDefinition& Dress);
	WAROFGODS_API void FillStormbreakPilot(UArenaVisualDressDefinition& Dress);
	WAROFGODS_API void FillDominionPilot(UArenaVisualDressDefinition& Dress);
	WAROFGODS_API void FillFallenSanctuaryPilot(UArenaVisualDressDefinition& Dress);

	/** Load DA at canonical path, or create a transient dress for known layouts. */
	WAROFGODS_API UArenaVisualDressDefinition* ResolveDress(
		EArenaGrayboxLayout Layout,
		const TSoftObjectPtr<UArenaVisualDressDefinition>& Assigned,
		UObject* TransientOuter);
}
