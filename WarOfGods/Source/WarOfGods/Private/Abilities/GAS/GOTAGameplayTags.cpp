// White Tiger Studios 2024

#include "Abilities/GAS/GOTAGameplayTags.h"

namespace GOTATags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status, "GOTA.Status", "Parent of every status granted by a status effect.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Slowed, "GOTA.Status.Slowed", "Movement speed reduced (Kairos Time Lash).");
	UE_DEFINE_GAMEPLAY_TAG(Status_Dread, "GOTA.Status.Dread");
	UE_DEFINE_GAMEPLAY_TAG(Status_Veiled, "GOTA.Status.Veiled");
	UE_DEFINE_GAMEPLAY_TAG(Status_Unbreakable, "GOTA.Status.Unbreakable");
	UE_DEFINE_GAMEPLAY_TAG(Status_Burning, "GOTA.Status.Burning");
	UE_DEFINE_GAMEPLAY_TAG(Status_Frozen, "GOTA.Status.Frozen");
	UE_DEFINE_GAMEPLAY_TAG(Status_Stunned, "GOTA.Status.Stunned");

	UE_DEFINE_GAMEPLAY_TAG(Immunity_Slow, "GOTA.Immunity.Slow");
	UE_DEFINE_GAMEPLAY_TAG(Immunity_Stun, "GOTA.Immunity.Stun");

	UE_DEFINE_GAMEPLAY_TAG(Element_Fire, "GOTA.Element.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Element_Frost, "GOTA.Element.Frost");
	UE_DEFINE_GAMEPLAY_TAG(Element_Shadow, "GOTA.Element.Shadow");
	UE_DEFINE_GAMEPLAY_TAG(Element_Void, "GOTA.Element.Void");
	UE_DEFINE_GAMEPLAY_TAG(Element_Time, "GOTA.Element.Time");

	UE_DEFINE_GAMEPLAY_TAG(Data_Duration, "GOTA.Data.Duration");
	UE_DEFINE_GAMEPLAY_TAG(Data_Multiplier, "GOTA.Data.Multiplier");
	UE_DEFINE_GAMEPLAY_TAG(Data_Magnitude, "GOTA.Data.Magnitude");

	UE_DEFINE_GAMEPLAY_TAG(Cue_Status_Slowed, "GameplayCue.GOTA.Status.Slowed");
}
