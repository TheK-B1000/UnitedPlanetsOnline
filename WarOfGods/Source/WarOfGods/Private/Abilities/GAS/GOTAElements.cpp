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
	// Named so abilities can already be typed; they inflict no status until their gameplay is designed.
	Add(GOTATags::Element_Frost, TEXT("Frost"));
	Add(GOTATags::Element_Lightning, TEXT("Lightning"));
	Add(GOTATags::Element_Shadow, TEXT("Shadow"));
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
	return GetDefault<UGOTAElementSettings>()->Elements.FindByPredicate([Status](const FGOTAElementDef& Def) { return Def.Status == Status; });
}
