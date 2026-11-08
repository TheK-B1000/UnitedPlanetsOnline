// White Tiger Studios 2024
//
// Native Gameplay Tag taxonomy for the GAS layer (Docs/GOTA_GAS_ARCHITECTURE.md, "Tags").
// Tags marked "reserved" have no reader yet; they fix the names before Phase 2 effects use them.

#pragma once

#include "NativeGameplayTags.h"

namespace GOTATags
{
	// --- Statuses: granted to the target while a status effect is active (one tag per status) ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Slowed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Dread);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Veiled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Unbreakable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Burning);     // reserved: Phase 2
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Frozen);      // reserved: Phase 2
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stunned);     // reserved: Phase 2

	// --- Immunities: a target with one of these rejects the matching statuses (reserved: Phase 2/4) ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Slow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Stun);

	// --- Elements: asset tags on effects, for resistances and vulnerabilities (reserved: Phase 2) ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Frost);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Shadow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Void);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Time);

	// --- SetByCaller keys: balance numbers come from FGOTAAbilityDef, never from the effect class ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration);
	/** The attribute multiplier the effect applies (Slowed: 1 - slow fraction). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Multiplier);
	/** The ability definition's original Magnitude, kept on the spec for HUD and logs. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Magnitude);
	/** Outgoing ability damage multiplier (Dread: 1 - Magnitude * 0.5). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_DamageMultiplier);

	// --- Gameplay Cues (presentation only) ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Slowed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Dread);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Veiled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Unbreakable);
}
