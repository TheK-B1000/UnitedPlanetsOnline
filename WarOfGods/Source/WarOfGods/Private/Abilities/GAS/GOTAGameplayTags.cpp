// White Tiger Studios 2024

#include "Abilities/GAS/GOTAGameplayTags.h"

namespace GOTATags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status, "GOTA.Status", "Parent of every status granted by a status effect.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Slowed, "GOTA.Status.Slowed", "Movement speed reduced (Kairos Time Lash).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Dread, "GOTA.Status.Dread", "Slower and weaker ability damage (Nyx Soul Drain, Varoth World Burn).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Veiled, "GOTA.Status.Veiled", "Untargetable by abilities and immune to Blueprint hits (Nyx Void Shield).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Unbreakable, "GOTA.Status.Unbreakable", "Hyper armour and partial damage refund (Varoth Colossus Form).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Burning, "GOTA.Status.Burning", "Fire damage over time (Drakon Molten Strike).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Frozen, "GOTA.Status.Frozen", "Hard crowd control (Frost): held in place, ends on the first real hit.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Stunned, "GOTA.Status.Stunned", "Hard crowd control: no movement or actions.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Shielded, "GOTA.Status.Shielded", "Absorbs damage before health until used up or expired.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Empowered, "GOTA.Status.Empowered", "Deals more ability and boss damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Fortified, "GOTA.Status.Fortified", "Takes less damage from every source.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Vulnerable, "GOTA.Status.Vulnerable", "Takes more damage from every source.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hasted, "GOTA.Status.Hasted", "Moves faster.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Weakened, "GOTA.Status.Weakened", "Deals less ability and boss damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Chilled, "GOTA.Status.Chilled", "Frost: slowed; another Frost hit deep-freezes.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Shocked, "GOTA.Status.Shocked", "Lightning: takes more damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Incapacitated, "GOTA.State.Incapacitated", "Cannot move or act (granted by Stunned and Frozen).");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity, "GOTA.Immunity", "Parent of the effect-aspect immunities a status policy grants.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Slow, "GOTA.Immunity.Slow", "Immune to movement impairment from effects.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Weaken, "GOTA.Immunity.Weaken", "Immune to attack-power reduction from effects.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Stun, "GOTA.Immunity.Stun", "Immune to hard crowd-control effects (not a boss's own staggers).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Interrupt, "GOTA.Immunity.Interrupt", "Hero-ability casts are not interrupted by damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Burn, "GOTA.Immunity.Burn", "Cannot be set alight (Burning is refused).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Immunity_Vulnerable, "GOTA.Immunity.Vulnerable", "Incoming-damage amplification is refused.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element, "GOTA.Element", "Parent of the damage types. No element tag means untyped (physical) damage.");
	UE_DEFINE_GAMEPLAY_TAG(Element_Fire, "GOTA.Element.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Element_Lightning, "GOTA.Element.Lightning");
	UE_DEFINE_GAMEPLAY_TAG(Element_Frost, "GOTA.Element.Frost");
	UE_DEFINE_GAMEPLAY_TAG(Element_Shadow, "GOTA.Element.Shadow");
	UE_DEFINE_GAMEPLAY_TAG(Element_Void, "GOTA.Element.Void");
	UE_DEFINE_GAMEPLAY_TAG(Element_Time, "GOTA.Element.Time");

	UE_DEFINE_GAMEPLAY_TAG(Data_Duration, "GOTA.Data.Duration");
	UE_DEFINE_GAMEPLAY_TAG(Data_Multiplier, "GOTA.Data.Multiplier");
	UE_DEFINE_GAMEPLAY_TAG(Data_Magnitude, "GOTA.Data.Magnitude");
	UE_DEFINE_GAMEPLAY_TAG(Data_DamageMultiplier, "GOTA.Data.DamageMultiplier");
	UE_DEFINE_GAMEPLAY_TAG(Data_TickDamage, "GOTA.Data.TickDamage");
	UE_DEFINE_GAMEPLAY_TAG(Data_IncomingMultiplier, "GOTA.Data.IncomingMultiplier");

	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Slowed, "GameplayCue.GOTA.Status.Slowed");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Dread, "GameplayCue.GOTA.Status.Dread");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Veiled, "GameplayCue.GOTA.Status.Veiled");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Unbreakable, "GameplayCue.GOTA.Status.Unbreakable");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Burning, "GameplayCue.GOTA.Status.Burning");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Stunned, "GameplayCue.GOTA.Status.Stunned");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Frozen, "GameplayCue.GOTA.Status.Frozen");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Shielded, "GameplayCue.GOTA.Status.Shielded");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Empowered, "GameplayCue.GOTA.Status.Empowered");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Fortified, "GameplayCue.GOTA.Status.Fortified");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Vulnerable, "GameplayCue.GOTA.Status.Vulnerable");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Hasted, "GameplayCue.GOTA.Status.Hasted");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Weakened, "GameplayCue.GOTA.Status.Weakened");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Chilled, "GameplayCue.GOTA.Status.Chilled");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Shocked, "GameplayCue.GOTA.Status.Shocked");
	UE_DEFINE_GAMEPLAY_TAG(Cue_Resisted, "GameplayCue.GOTA.Resisted");
	UE_DEFINE_GAMEPLAY_TAG(Cue_ShieldBroken, "GameplayCue.GOTA.ShieldBroken");
}
