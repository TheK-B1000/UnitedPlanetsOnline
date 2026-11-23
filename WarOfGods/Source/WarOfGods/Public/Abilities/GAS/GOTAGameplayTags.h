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
	/** Damage over time (Drakon Molten Strike). Ticks go through BPC_DamageSystem; see UGOTABurnTickExecution. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Burning);
	/** Hard crowd control, Frost: held in place, ends early on the first real hit. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Frozen);
	/** Hard crowd control: no movement or actions for its duration. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stunned);

	// Buffs and debuffs (milestone B). Each is one effect with SetByCaller numbers from the ability definition.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Shielded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Empowered);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Fortified);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Vulnerable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Hasted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Weakened);
	// Element statuses (milestone C): Frost and Lightning. Fire is Burning, Shadow is Dread.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Chilled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Shocked);

	// --- States: derived conditions several statuses grant (one reader instead of a list of statuses) ---
	/** Cannot move or act (Stunned, Frozen). Every lock and its release hangs off this one tag. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Incapacitated);

	// --- Immunities: one per effect aspect, granted by a status policy (bosses). An effect whose every aspect is
	// blocked is refused; otherwise only the blocked modifiers stop counting (modifier target-tag filters). ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity);
	/** Movement impairment from any effect: Slowed entirely, Dread's walk modifier. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Slow);
	/** Attack-power reduction from any effect: Dread's damage modifier. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Weaken);
	/** Hard crowd control applied as an effect (Stunned, Frozen); also the short recovery after one ends. Does NOT cover
	 *  a boss's own designed staggers, parry recoil or the Divine Counter stun, which belong to UGOTABossComponent. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Stun);
	/** A hero-ability cast is not interrupted by taking damage (the same check Unbreakable passes). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Interrupt);
	/** Cannot be set alight: Burning is refused (the hit that carried it still lands). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Burn);
	/** Incoming-damage amplification (Vulnerable) is refused. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Immunity_Vulnerable);

	// --- Elements: the damage type of an ability or a status. No element tag means untyped (physical) damage.
	// Each element's tuning is one FGOTAElementDef row (Abilities/GAS/GOTAElements.h). Only Fire has gameplay so far. ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Lightning);
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
	/** Damage one periodic tick deals (Burning: damage per second x tick interval). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_TickDamage);
	/** Incoming damage multiplier (Fortified 1 - Magnitude, Vulnerable 1 + Magnitude). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_IncomingMultiplier);

	// --- Gameplay Cues (presentation only) ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Slowed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Dread);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Veiled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Unbreakable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Burning);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Stunned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Frozen);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Shielded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Empowered);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Fortified);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Vulnerable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Hasted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Weakened);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Chilled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Status_Shocked);
	/** Executed on a target that resisted all or part of a status (immunity / resistance feedback). */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cue_Resisted);
}
