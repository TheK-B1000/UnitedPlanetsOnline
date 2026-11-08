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

	// --- Immunities: one per effect aspect, granted by a status policy (bosses). An effect whose every aspect is
	// blocked is refused; otherwise only the blocked modifiers stop counting (modifier target-tag filters). ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity);
	/** Movement impairment from any effect: Slowed entirely, Dread's walk modifier. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Slow);
	/** Attack-power reduction from any effect: Dread's damage modifier. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Weaken);
	/** Hard crowd control applied as an effect (future Frozen / Stunned). Does NOT cover a boss's own designed
	 *  staggers, parry recoil or the Divine Counter stun, which belong to UGOTABossComponent and are not effects. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Stun);
	/** A hero-ability cast is not interrupted by taking damage (the same check Unbreakable passes). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Interrupt);

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
	/** Outgoing damage multiplier (Dread: 1 - Magnitude * 0.5). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_DamageMultiplier);

	// --- Gameplay Cues (presentation only) ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Slowed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Dread);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Veiled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Unbreakable);
	/** Executed on a target that resisted all or part of a status (immunity / resistance feedback). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Resisted);
}
