// White Tiger Studios 2024

#include "Abilities/GAS/GOTAStatusEffects.h"

#include "Abilities/GAS/GOTAAbilitySystemComponent.h"
#include "Abilities/GAS/GOTACombatAttributeSet.h"
#include "Abilities/GAS/GOTAGameplayTags.h"
#include "Abilities/GOTAHeroAbilityComponent.h"
#include "AbilitySystemGlobals.h"
#include "Characters/GOTAHeroBodyComponent.h"
#include "GameFramework/Character.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "Materials/MaterialInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAStatusEffects)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAStatus, Log, All);

namespace
{
	struct FStatusInfo
	{
		EGOTAStatus Status;
		const FNativeGameplayTag& Tag;
		const FNativeGameplayTag& CueTag;
		/** Overlay materials authored by Tools/GOTA_BuildHeroAbilities.py. A missing asset just means no overlay. */
		const TCHAR* OverlayPath;
	};

	// Overlay priority order: the first active entry wins the body's single overlay slot.
	const FStatusInfo Statuses[] = {
		{ EGOTAStatus::Veiled, GOTATags::Status_Veiled, GOTATags::Cue_Status_Veiled, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Veiled.M_GOTA_Overlay_Veiled") },
		{ EGOTAStatus::Unbreakable, GOTATags::Status_Unbreakable, GOTATags::Cue_Status_Unbreakable, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Unbreakable.M_GOTA_Overlay_Unbreakable") },
		{ EGOTAStatus::Dread, GOTATags::Status_Dread, GOTATags::Cue_Status_Dread, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Dread.M_GOTA_Overlay_Dread") },
		{ EGOTAStatus::Slowed, GOTATags::Status_Slowed, GOTATags::Cue_Status_Slowed, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Slowed.M_GOTA_Overlay_Slowed") },
	};

	const FStatusInfo* Info(EGOTAStatus Status)
	{
		for (const FStatusInfo& Entry : Statuses)
		{
			if (Entry.Status == Status)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	EGOTAStatus StatusForCue(const FGameplayCueParameters& Parameters)
	{
		const FGameplayTag Tag = Parameters.MatchedTagName.IsValid() ? Parameters.MatchedTagName : Parameters.OriginalTag;
		for (const FStatusInfo& Entry : Statuses)
		{
			if (Tag == Entry.CueTag.GetTag())
			{
				return Entry.Status;
			}
		}
		return EGOTAStatus::None;
	}

	TSubclassOf<UGameplayEffect> EffectClass(EGOTAStatus Status)
	{
		switch (Status)
		{
		case EGOTAStatus::Slowed: return UGOTAGameplayEffect_Slowed::StaticClass();
		case EGOTAStatus::Dread: return UGOTAGameplayEffect_Dread::StaticClass();
		case EGOTAStatus::Veiled: return UGOTAGameplayEffect_Veiled::StaticClass();
		case EGOTAStatus::Unbreakable: return UGOTAGameplayEffect_Unbreakable::StaticClass();
		default: return nullptr;
		}
	}

	const FActiveGameplayEffect* FindStatusEffect(const AActor* Actor, EGOTAStatus Status, const UGOTAAbilitySystemComponent*& OutASC)
	{
		OutASC = UGOTAAbilitySystemComponent::Find(Actor);
		const FStatusInfo* Entry = Info(Status);
		if (!OutASC || !Entry)
		{
			return nullptr;
		}
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Entry->Tag.GetTag()));
		const TArray<FActiveGameplayEffectHandle> Handles = OutASC->GetActiveEffects(Query);
		return Handles.Num() > 0 ? OutASC->GetActiveGameplayEffect(Handles[0]) : nullptr;
	}

	USkeletalMeshComponent* VisibleBody(AActor* Actor)
	{
		ACharacter* Char = Cast<ACharacter>(Actor);
		if (!Char)
		{
			return nullptr;
		}
		if (const UGOTAHeroBodyComponent* Body = Char->FindComponentByClass<UGOTAHeroBodyComponent>())
		{
			if (USkeletalMeshComponent* Mesh = Body->GetBodyMesh())
			{
				return Mesh;
			}
		}
		return Char->GetMesh();
	}
}

// ============================================================================ effects

void UGOTAStatusGameplayEffect::ConfigureStatus(const FGameplayTag& StatusTag, const FGameplayTag& CueTag, const FGameplayTag& ImmunityTag)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat DurationCaller;
	DurationCaller.DataTag = GOTATags::Data_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationCaller);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(CueTag);
	GameplayCues.Add(Cue);

	FInheritedTagContainer Granted;
	Granted.Added.AddTag(StatusTag);
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	TargetTags->SetAndApplyTargetTagChanges(Granted);

	FInheritedTagContainer Asset;
	Asset.Added.AddTag(StatusTag);
	UAssetTagsGameplayEffectComponent* AssetTags = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	GEComponents.Add(AssetTags);
	AssetTags->SetAndApplyAssetTagChanges(Asset);

	if (ImmunityTag.IsValid())
	{
		UTargetTagRequirementsGameplayEffectComponent* Requirements = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("Requirements"));
		GEComponents.Add(Requirements);
		Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(ImmunityTag);
	}
}

void UGOTAStatusGameplayEffect::AddMultiplier(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
{
	FSetByCallerFloat Caller;
	Caller.DataTag = DataTag;
	FGameplayModifierInfo Mod;
	Mod.Attribute = Attribute;
	Mod.ModifierOp = EGameplayModOp::MultiplyAdditive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Caller);
	Modifiers.Add(Mod);
}

UGOTAGameplayEffect_Slowed::UGOTAGameplayEffect_Slowed()
{
	ConfigureStatus(GOTATags::Status_Slowed, GOTATags::Cue_Status_Slowed, GOTATags::Immunity_Slow);
	AddMultiplier(UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute(), GOTATags::Data_Multiplier);
}

UGOTAGameplayEffect_Dread::UGOTAGameplayEffect_Dread()
{
	ConfigureStatus(GOTATags::Status_Dread, GOTATags::Cue_Status_Dread);
	AddMultiplier(UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute(), GOTATags::Data_Multiplier);
	AddMultiplier(UGOTACombatAttributeSet::GetOutgoingAbilityDamageMultiplierAttribute(), GOTATags::Data_DamageMultiplier);
}

UGOTAGameplayEffect_Veiled::UGOTAGameplayEffect_Veiled()
{
	ConfigureStatus(GOTATags::Status_Veiled, GOTATags::Cue_Status_Veiled);
}

UGOTAGameplayEffect_Unbreakable::UGOTAGameplayEffect_Unbreakable()
{
	ConfigureStatus(GOTATags::Status_Unbreakable, GOTATags::Cue_Status_Unbreakable);
}

// ============================================================================ overlay cue

bool UGOTAGameplayCue_StatusOverlay::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UE_LOG(LogGOTAStatus, Log, TEXT("cue %s on %s"), *Parameters.MatchedTagName.ToString(), *GetNameSafe(MyTarget));
	GOTAGAS::RefreshStatusOverlay(MyTarget, StatusForCue(Parameters));
	return true;
}

bool UGOTAGameplayCue_StatusOverlay::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// A replicated effect can reach a client as WhileActive only (no OnActive), so this logs the same way.
	UE_LOG(LogGOTAStatus, Log, TEXT("cue %s on %s (while active)"), *Parameters.MatchedTagName.ToString(), *GetNameSafe(MyTarget));
	GOTAGAS::RefreshStatusOverlay(MyTarget, StatusForCue(Parameters));
	return true;
}

bool UGOTAGameplayCue_StatusOverlay::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UE_LOG(LogGOTAStatus, Log, TEXT("cue %s removed from %s"), *Parameters.MatchedTagName.ToString(), *GetNameSafe(MyTarget));
	GOTAGAS::RefreshStatusOverlay(MyTarget, EGOTAStatus::None, StatusForCue(Parameters));
	return true;
}

// ============================================================================ adapter

namespace GOTAGAS
{
	bool OwnsStatus(EGOTAStatus Status)
	{
		// Migration order (one status per verified step): Slowed, Dread, Veiled, Unbreakable.
		return Status == EGOTAStatus::Slowed || Status == EGOTAStatus::Dread || Status == EGOTAStatus::Veiled || Status == EGOTAStatus::Unbreakable;
	}

	bool ApplyStatus(APawn* Victim, EGOTAStatus Status, float Duration, float Magnitude, AActor* Source)
	{
		const TSubclassOf<UGameplayEffect> Class = EffectClass(Status);
		if (!OwnsStatus(Status) || !Class || !Victim || !Victim->HasAuthority() || Duration <= 0.f)
		{
			return false;
		}
		RegisterNativeCues();
		UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::EnsureOnPawn(Victim);
		if (!ASC)
		{
			return false;
		}
		const FString Name = StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(Status));
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(Source, Source);
		Context.AddSourceObject(Source);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Class, 1.f, Context);
		if (!Spec.IsValid())
		{
			UE_LOG(LogGOTAStatus, Warning, TEXT("%s: %s spec failed"), *GetNameSafe(Victim), *Name);
			return false;
		}
		// Every number comes from the ability definition. The pre-GAS formulas and clamps, unchanged.
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Duration, Duration);
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Magnitude, Magnitude);
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Multiplier, FMath::Clamp(1.f - Magnitude, UGOTACombatAttributeSet::MinMultiplier, 1.f));
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_DamageMultiplier, FMath::Clamp(1.f - Magnitude * 0.5f, UGOTACombatAttributeSet::MinMultiplier, 1.f));
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (!Handle.WasSuccessfullyApplied())
		{
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s rejected (immune) from %s"), *GetNameSafe(Victim), *Name, *GetNameSafe(Source));
			return false;
		}
		// Same leading wording as the ability component's slot log, so the regression suite reads both.
		UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s %.1fs x%.2f (from %s) [GAS walk x%.2f dmg x%.2f]"), *GetNameSafe(Victim), *Name, Duration, Magnitude,
			*GetNameSafe(Source), ASC->GetMoveSpeedMultiplier(), ASC->GetOutgoingAbilityDamageMultiplier());
		return true;
	}

	bool HasStatus(const AActor* Actor, EGOTAStatus Status)
	{
		const UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(Actor);
		const FStatusInfo* Entry = Info(Status);
		return ASC && Entry && ASC->HasMatchingGameplayTag(Entry->Tag.GetTag());
	}

	float GetStatusRemaining(const AActor* Actor, EGOTAStatus Status)
	{
		const UGOTAAbilitySystemComponent* ASC = nullptr;
		const FActiveGameplayEffect* Effect = FindStatusEffect(Actor, Status, ASC);
		return Effect && ASC->GetWorld() ? FMath::Max(0.f, Effect->GetTimeRemaining(ASC->GetWorld()->GetTimeSeconds())) : 0.f;
	}

	float GetStatusMagnitude(const AActor* Actor, EGOTAStatus Status)
	{
		const UGOTAAbilitySystemComponent* ASC = nullptr;
		const FActiveGameplayEffect* Effect = FindStatusEffect(Actor, Status, ASC);
		return Effect ? Effect->Spec.GetSetByCallerMagnitude(GOTATags::Data_Magnitude, false, 0.f) : 0.f;
	}

	void RefreshStatusOverlay(AActor* Actor, EGOTAStatus Adding, EGOTAStatus Removing)
	{
		USkeletalMeshComponent* Mesh = VisibleBody(Actor);
		if (!Mesh)
		{
			return;
		}
		// Statuses not migrated yet still live on the ability component's slot.
		const UGOTAHeroAbilityComponent* Slot = Actor->FindComponentByClass<UGOTAHeroAbilityComponent>();
		const EGOTAStatus SlotStatus = Slot && Slot->GetStatusRemaining() > 0.f ? Slot->GetStatus() : EGOTAStatus::None;
		const TCHAR* Path = nullptr;
		for (const FStatusInfo& Entry : Statuses)
		{
			const bool bActive = Entry.Status != Removing
				&& (Entry.Status == Adding || Entry.Status == SlotStatus || HasStatus(Actor, Entry.Status));
			if (bActive)
			{
				Path = Entry.OverlayPath;
				break;
			}
		}
		UMaterialInterface* Overlay = Path ? LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn) : nullptr;
		if (Mesh->GetOverlayMaterial() != Overlay)
		{
			UE_LOG(LogGOTAStatus, Log, TEXT("overlay %s -> %s on %s"), *GetNameSafe(Mesh->GetOverlayMaterial()), *GetNameSafe(Overlay), *GetNameSafe(Actor));
			Mesh->SetOverlayMaterial(Overlay);
		}
	}

	void RegisterNativeCues()
	{
		UGameplayCueManager* Manager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
		UGameplayCueSet* Set = Manager ? Manager->GetRuntimeCueSet() : nullptr;
		if (!Set)
		{
			return;
		}
		TArray<FGameplayCueReferencePair> Missing;
		for (const FStatusInfo& Entry : Statuses)
		{
			if (!Set->GameplayCueDataMap.Contains(Entry.CueTag.GetTag()))
			{
				Missing.Emplace(Entry.CueTag.GetTag(), FSoftObjectPath(UGOTAGameplayCue_StatusOverlay::StaticClass()));
			}
		}
		if (Missing.Num() == 0)
		{
			return;
		}
		// Native cue classes are not found by the asset scan; this is the runtime set's public registration path.
		Set->AddCues(Missing);
		// The class is native and always loaded: hook it up now so the first cue never waits on an async load.
		for (FGameplayCueNotifyData& Data : Set->GameplayCueData)
		{
			for (const FGameplayCueReferencePair& Pair : Missing)
			{
				if (Data.GameplayCueTag == Pair.GameplayCueTag)
				{
					Data.LoadedGameplayCueClass = UGOTAGameplayCue_StatusOverlay::StaticClass();
				}
			}
		}
		UE_LOG(LogGOTAStatus, Log, TEXT("Registered %d native gameplay cue(s)"), Missing.Num());
	}
}
