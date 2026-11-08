// White Tiger Studios 2024

#include "Abilities/GAS/GOTAStatusEffects.h"

#include "Abilities/GAS/GOTAAbilitySystemComponent.h"
#include "Abilities/GAS/GOTACombatAttributeSet.h"
#include "Abilities/GAS/GOTAGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "Characters/GOTAHeroBodyComponent.h"
#include "GameFramework/Character.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Materials/MaterialInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAStatusEffects)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAStatus, Log, All);

namespace
{
	struct FStatusOverlay
	{
		const FNativeGameplayTag& CueTag;
		const TCHAR* MaterialPath;
	};

	// Overlay materials authored by Tools/GOTA_BuildHeroAbilities.py. A missing asset just means no overlay.
	const FStatusOverlay StatusOverlays[] = {
		{ GOTATags::Cue_Status_Slowed, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Slowed.M_GOTA_Overlay_Slowed") },
	};

	UMaterialInterface* OverlayFor(const FGameplayCueParameters& Parameters)
	{
		const FGameplayTag Tag = Parameters.MatchedTagName.IsValid() ? Parameters.MatchedTagName : Parameters.OriginalTag;
		for (const FStatusOverlay& Entry : StatusOverlays)
		{
			if (Tag == Entry.CueTag.GetTag())
			{
				return LoadObject<UMaterialInterface>(nullptr, Entry.MaterialPath, nullptr, LOAD_NoWarn);
			}
		}
		return nullptr;
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

	FGameplayTag StatusTag(EGOTAStatus Status)
	{
		switch (Status)
		{
		case EGOTAStatus::Slowed: return GOTATags::Status_Slowed;
		case EGOTAStatus::Dread: return GOTATags::Status_Dread;
		case EGOTAStatus::Veiled: return GOTATags::Status_Veiled;
		case EGOTAStatus::Unbreakable: return GOTATags::Status_Unbreakable;
		default: return FGameplayTag();
		}
	}
}

// ============================================================================ Slowed effect

UGOTAGameplayEffect_Slowed::UGOTAGameplayEffect_Slowed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat DurationCaller;
	DurationCaller.DataTag = GOTATags::Data_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationCaller);

	FSetByCallerFloat MultiplierCaller;
	MultiplierCaller.DataTag = GOTATags::Data_Multiplier;
	FGameplayModifierInfo Slow;
	Slow.Attribute = UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute();
	Slow.ModifierOp = EGameplayModOp::MultiplyAdditive;
	Slow.ModifierMagnitude = FGameplayEffectModifierMagnitude(MultiplierCaller);
	Modifiers.Add(Slow);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(GOTATags::Cue_Status_Slowed);
	GameplayCues.Add(Cue);

	// Native effects declare their components in the constructor (UGameplayEffect::PostInitProperties checks this).
	FInheritedTagContainer Granted;
	Granted.Added.AddTag(GOTATags::Status_Slowed);
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	TargetTags->SetAndApplyTargetTagChanges(Granted);

	FInheritedTagContainer Asset;
	Asset.Added.AddTag(GOTATags::Status_Slowed);
	Asset.Added.AddTag(GOTATags::Element_Time);
	UAssetTagsGameplayEffectComponent* AssetTags = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	GEComponents.Add(AssetTags);
	AssetTags->SetAndApplyAssetTagChanges(Asset);

	UTargetTagRequirementsGameplayEffectComponent* Requirements = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("Requirements"));
	GEComponents.Add(Requirements);
	Requirements->ApplicationTagRequirements.IgnoreTags.AddTag(GOTATags::Immunity_Slow);
}

// ============================================================================ overlay cue

bool UGOTAGameplayCue_StatusOverlay::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	return WhileActive_Implementation(MyTarget, Parameters);
}

bool UGOTAGameplayCue_StatusOverlay::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	USkeletalMeshComponent* Mesh = VisibleBody(MyTarget);
	UMaterialInterface* Overlay = OverlayFor(Parameters);
	if (Mesh && Overlay && Mesh->GetOverlayMaterial() != Overlay)
	{
		Mesh->SetOverlayMaterial(Overlay);
		UE_LOG(LogGOTAStatus, Log, TEXT("cue %s on %s"), *Parameters.MatchedTagName.ToString(), *GetNameSafe(MyTarget));
	}
	return true;
}

bool UGOTAGameplayCue_StatusOverlay::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	USkeletalMeshComponent* Mesh = VisibleBody(MyTarget);
	UMaterialInterface* Overlay = OverlayFor(Parameters);
	if (Mesh && Overlay && Mesh->GetOverlayMaterial() == Overlay)
	{
		Mesh->SetOverlayMaterial(nullptr);
	}
	return true;
}

// ============================================================================ adapter

namespace GOTAGAS
{
	bool OwnsStatus(EGOTAStatus Status)
	{
		return Status == EGOTAStatus::Slowed;
	}

	bool ApplyStatus(APawn* Victim, EGOTAStatus Status, float Duration, float Magnitude, AActor* Source)
	{
		if (!OwnsStatus(Status) || !Victim || !Victim->HasAuthority() || Duration <= 0.f)
		{
			return false;
		}
		RegisterNativeCues();
		UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::EnsureOnPawn(Victim);
		if (!ASC)
		{
			return false;
		}
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(Source, Source);
		Context.AddSourceObject(Source);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGOTAGameplayEffect_Slowed::StaticClass(), 1.f, Context);
		if (!Spec.IsValid())
		{
			UE_LOG(LogGOTAStatus, Warning, TEXT("%s: Slowed spec failed"), *GetNameSafe(Victim));
			return false;
		}
		const float Multiplier = FMath::Clamp(1.f - Magnitude, UGOTACombatAttributeSet::MinMoveSpeedMultiplier, 1.f);
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Duration, Duration);
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Multiplier, Multiplier);
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Magnitude, Magnitude);
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (!Handle.WasSuccessfullyApplied())
		{
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: status Slowed rejected (immune) from %s"), *GetNameSafe(Victim), *GetNameSafe(Source));
			return false;
		}
		// Same wording as the ability component's slot log, so the regression suite reads both.
		UE_LOG(LogGOTAStatus, Log, TEXT("%s: status Slowed %.1fs x%.2f (from %s) [GAS walk x%.2f]"), *GetNameSafe(Victim), Duration, Magnitude,
			*GetNameSafe(Source), ASC->GetMoveSpeedMultiplier());
		return true;
	}

	bool HasStatus(const AActor* Actor, EGOTAStatus Status)
	{
		const UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(Actor);
		const FGameplayTag Tag = StatusTag(Status);
		return ASC && Tag.IsValid() && ASC->HasMatchingGameplayTag(Tag);
	}

	float GetStatusRemaining(const AActor* Actor, EGOTAStatus Status)
	{
		const UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(Actor);
		const FGameplayTag Tag = StatusTag(Status);
		if (!ASC || !Tag.IsValid())
		{
			return 0.f;
		}
		float Best = 0.f;
		for (const float Remaining : ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag))))
		{
			Best = FMath::Max(Best, Remaining);
		}
		return Best;
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
		for (const FStatusOverlay& Entry : StatusOverlays)
		{
			if (!Set->GameplayCueDataMap.Contains(Entry.CueTag.GetTag()))
			{
				Missing.Emplace(Entry.CueTag.GetTag(), FSoftObjectPath(UGOTAGameplayCue_StatusOverlay::StaticClass()));
			}
		}
		if (Missing.Num() > 0)
		{
			// Native cue classes are not found by the asset scan; this is the runtime set's public registration path.
			Set->AddCues(Missing);
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
}
