// White Tiger Studios 2024

#include "Abilities/GAS/GOTAElements.h"

#include "Abilities/GAS/GOTAGameplayTags.h"
#include "Particles/ParticleSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAElements)

UGOTAElementSettings::UGOTAElementSettings()
{
	auto Add = [this](const FGameplayTag& Tag, const TCHAR* Name) -> FGOTAElementDef&
	{
		FGOTAElementDef& Def = Elements.AddDefaulted_GetRef();
		Def.Element = Tag;
		Def.DisplayName = FText::FromString(Name);
		return Def;
	};
	// Fire: Burning, 2 damage per second for 3 s in 1 s ticks (3 ticks, 6 damage). First user: Drakon's Molten Strike.
	FGOTAElementDef& Fire = Add(GOTATags::Element_Fire, TEXT("Fire"));
	Fire.Status = EGOTAStatus::Burning;
	Fire.StatusDuration = 3.f;
	Fire.StatusMagnitude = 2.f;
	Fire.TickInterval = 1.f;
	// The brazier fire the arenas already use (Realistic Starter VFX Vol 2): a continuous flame, visible on a standing body.
	Fire.StatusFX = FSoftObjectPath(TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Small.P_Fire_Small"));
	// Frost (milestone C): Chilled, 30% slower for 3 s; a second Frost hit while chilled deep-freezes for 1.5 s (Frozen:
	// hard crowd control with its own rules, so the recovery immunity also stops a third freeze).
	FGOTAElementDef& Frost = Add(GOTATags::Element_Frost, TEXT("Frost"));
	Frost.Status = EGOTAStatus::Chilled;
	Frost.StatusDuration = 3.f;
	Frost.StatusMagnitude = 0.3f;
	Frost.FollowUpStatus = EGOTAStatus::Frozen;
	Frost.FollowUpDuration = 1.5f;
	// Lightning: Shocked, takes 20% more damage for 3 s.
	FGOTAElementDef& Lightning = Add(GOTATags::Element_Lightning, TEXT("Lightning"));
	Lightning.Status = EGOTAStatus::Shocked;
	Lightning.StatusDuration = 3.f;
	Lightning.StatusMagnitude = 0.2f;
	// Shadow: Dread (slower, weaker), the status Nyx and Varoth already use, at a lighter 0.25 for 3 s.
	FGOTAElementDef& Shadow = Add(GOTATags::Element_Shadow, TEXT("Shadow"));
	Shadow.Status = EGOTAStatus::Dread;
	Shadow.StatusDuration = 3.f;
	Shadow.StatusMagnitude = 0.25f;
}

const FGOTAElementDef* UGOTAElementSettings::Find(const FGameplayTag& Element)
{
	if (!Element.IsValid())
	{
		return nullptr;
	}
	return GetDefault<UGOTAElementSettings>()->Elements.FindByPredicate([&Element](const FGOTAElementDef& Def) { return Def.Element == Element; });
}

const FGOTAElementDef* UGOTAElementSettings::FindByStatus(EGOTAStatus Status)
{
	if (Status == EGOTAStatus::None)
	{
		return nullptr;
	}
	// Dread is also Shadow's status, but it is not owned by Shadow (Soul Drain, World Burn): only Burning reads this today.
	return GetDefault<UGOTAElementSettings>()->Elements.FindByPredicate([Status](const FGOTAElementDef& Def) { return Def.Status == Status; });
}
