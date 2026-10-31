// White Tiger Studios 2024

#include "Combat/GOTACombatantObserver.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace GOTACombatReflection
{
	// Blueprint variables may carry spaces in their FName ("Max Health", "Is Dead").
	static const FName HealthNames[] = { TEXT("Health"), TEXT("Current Health"), TEXT("CurrentHealth") };
	static const FName MaxHealthNames[] = { TEXT("MaxHealth"), TEXT("Max Health") };
	static const FName IsDeadNames[] = { TEXT("IsDead"), TEXT("Is Dead"), TEXT("bIsDead") };

	template <int32 N>
	static const FProperty* FindAny(const UObject* Obj, const FName (&Names)[N])
	{
		for (const FName& Name : Names)
		{
			if (const FProperty* Prop = Obj->GetClass()->FindPropertyByName(Name))
			{
				return Prop;
			}
		}
		return nullptr;
	}

	static bool ReadNumber(const UObject* Obj, const FProperty* Prop, float& Out)
	{
		if (const FDoubleProperty* D = CastField<FDoubleProperty>(Prop))
		{
			Out = static_cast<float>(D->GetPropertyValue_InContainer(Obj));
			return true;
		}
		if (const FFloatProperty* F = CastField<FFloatProperty>(Prop))
		{
			Out = F->GetPropertyValue_InContainer(Obj);
			return true;
		}
		if (const FIntProperty* I = CastField<FIntProperty>(Prop))
		{
			Out = static_cast<float>(I->GetPropertyValue_InContainer(Obj));
			return true;
		}
		return false;
	}

	static bool ReadBool(const UObject* Obj, const FProperty* Prop, bool& Out)
	{
		if (const FBoolProperty* B = CastField<FBoolProperty>(Prop))
		{
			Out = B->GetPropertyValue_InContainer(Obj);
			return true;
		}
		return false;
	}
}

UActorComponent* UGOTACombatantObserver::FindDamageSystem(const AActor* Combatant)
{
	if (!Combatant)
	{
		return nullptr;
	}
	UActorComponent* Fallback = nullptr;
	for (UActorComponent* Comp : Combatant->GetComponents())
	{
		if (!Comp || !GOTACombatReflection::FindAny(Comp, GOTACombatReflection::HealthNames))
		{
			continue;
		}
		// Prefer the project damage system; accept any component exposing Health otherwise.
		if (Comp->GetClass()->GetName().Contains(TEXT("DamageSystem")))
		{
			return Comp;
		}
		Fallback = Fallback ? Fallback : Comp;
	}
	return Fallback;
}

FGOTACombatantVitals UGOTACombatantObserver::ReadVitals(const AActor* Combatant)
{
	FGOTACombatantVitals Vitals;
	const UActorComponent* Damage = FindDamageSystem(Combatant);
	if (!Damage)
	{
		return Vitals;
	}
	Vitals.bHasDamageSystem = true;
	GOTACombatReflection::ReadNumber(Damage, GOTACombatReflection::FindAny(Damage, GOTACombatReflection::HealthNames), Vitals.Health);
	if (!GOTACombatReflection::ReadNumber(Damage, GOTACombatReflection::FindAny(Damage, GOTACombatReflection::MaxHealthNames), Vitals.MaxHealth))
	{
		Vitals.MaxHealth = 0.f;
	}
	GOTACombatReflection::ReadBool(Damage, GOTACombatReflection::FindAny(Damage, GOTACombatReflection::IsDeadNames), Vitals.bIsDead);
	if (Vitals.MaxHealth <= 0.f)
	{
		Vitals.MaxHealth = FMath::Max(Vitals.Health, 1.f);
	}
	return Vitals;
}

bool UGOTACombatantObserver::IsCombatantDefeated(const AActor* Combatant)
{
	if (!IsValid(Combatant) || Combatant->IsActorBeingDestroyed())
	{
		return true;
	}
	const FGOTACombatantVitals Vitals = ReadVitals(Combatant);
	return Vitals.bHasDamageSystem && (Vitals.bIsDead || Vitals.Health <= 0.f);
}
