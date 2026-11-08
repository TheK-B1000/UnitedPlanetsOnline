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

void UGOTAStatusGameplayEffect::AddMultiplier(const FGameplayAttribute& Attribute, const FGameplayTag& DataTag, const FGameplayTag& ImmunityTag)
{
	FSetByCallerFloat Caller;
	Caller.DataTag = DataTag;
	FGameplayModifierInfo Mod;
	Mod.Attribute = Attribute;
	Mod.ModifierOp = EGameplayModOp::MultiplyAdditive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Caller);
	// For instant and periodic effects the engine skips this modifier on an immune target. Duration effects aggregate
	// without target tags (an engine limitation), so GOTAGAS::ApplyStatus neutralises a blocked aspect's SetByCaller.
	Mod.TargetTags.IgnoreTags.AddTag(ImmunityTag);
	Modifiers.Add(Mod);
}

UGOTAGameplayEffect_StatusPolicy::UGOTAGameplayEffect_StatusPolicy()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite; // tags come from the spec (DynamicGrantedTags)
}

UGOTAGameplayEffect_Slowed::UGOTAGameplayEffect_Slowed()
{
	ConfigureStatus(GOTATags::Status_Slowed, GOTATags::Cue_Status_Slowed, GOTATags::Immunity_Slow);
	AddMultiplier(UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute(), GOTATags::Data_Multiplier, GOTATags::Immunity_Slow);
}

UGOTAGameplayEffect_Dread::UGOTAGameplayEffect_Dread()
{
	ConfigureStatus(GOTATags::Status_Dread, GOTATags::Cue_Status_Dread);
	AddMultiplier(UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute(), GOTATags::Data_Multiplier, GOTATags::Immunity_Slow);
	AddMultiplier(UGOTACombatAttributeSet::GetOutgoingDamageMultiplierAttribute(), GOTATags::Data_DamageMultiplier, GOTATags::Immunity_Weaken);
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

// ============================================================================ resisted cue

bool UGOTAGameplayCue_Resisted::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const EGOTAStatus Status = static_cast<EGOTAStatus>(FMath::RoundToInt(Parameters.RawMagnitude));
	const int32 Aspects = FMath::RoundToInt(Parameters.NormalizedMagnitude);
	AActor* Instigator = Parameters.Instigator.Get();
	UE_LOG(LogGOTAStatus, Log, TEXT("cue Resisted %s aspects=%d on %s by %s"), *StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(Status)),
		Aspects, *GetNameSafe(MyTarget), *GetNameSafe(Instigator));
	GOTAGAS::OnStatusResisted.Broadcast(MyTarget, Status, Aspects, Instigator);
	return true;
}

// ============================================================================ adapter

namespace GOTAGAS
{
	FOnStatusResisted OnStatusResisted;

	int32 AspectsOf(EGOTAStatus Status)
	{
		switch (Status)
		{
		case EGOTAStatus::Slowed: return Aspect_Movement;
		case EGOTAStatus::Dread: return Aspect_Movement | Aspect_Weaken;
		default: return 0;
		}
	}

	bool HasTag(const AActor* Actor, const FGameplayTag& Tag)
	{
		const UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(Actor);
		return ASC && ASC->HasMatchingGameplayTag(Tag);
	}

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

		// Status policy (bosses): aspect immunities, then per-status resistance. Damage was already dealt by the ability.
		const int32 Aspects = AspectsOf(Status);
		int32 Blocked = 0;
		if ((Aspects & Aspect_Movement) && ASC->HasMatchingGameplayTag(GOTATags::Immunity_Slow))
		{
			Blocked |= Aspect_Movement;
		}
		if ((Aspects & Aspect_Weaken) && ASC->HasMatchingGameplayTag(GOTATags::Immunity_Weaken))
		{
			Blocked |= Aspect_Weaken;
		}
		bool bWhole = Aspects != 0 && Blocked == Aspects;
		if (const FGOTAStatusResistance* Rule = ASC->FindStatusResistance(Status))
		{
			Duration *= Rule->DurationScale;
			Magnitude *= Rule->MagnitudeScale;
			if (Duration <= 0.f)
			{
				bWhole = true;
			}
			else if (Rule->DurationScale < 1.f || Rule->MagnitudeScale < 1.f)
			{
				Blocked |= Aspect_Reduced;
			}
		}
		if (bWhole || Blocked != 0)
		{
			const int32 Report = Blocked | (bWhole ? Aspect_Whole : 0);
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s resisted (%s%s%s%s) from %s"), *GetNameSafe(Victim), *Name,
				bWhole ? TEXT("whole ") : TEXT(""), (Report & Aspect_Movement) ? TEXT("movement ") : TEXT(""),
				(Report & Aspect_Weaken) ? TEXT("weaken ") : TEXT(""), (Report & Aspect_Reduced) ? TEXT("reduced") : TEXT(""), *GetNameSafe(Source));
			FGameplayCueParameters Feedback;
			Feedback.RawMagnitude = static_cast<float>(Status);
			Feedback.NormalizedMagnitude = static_cast<float>(Report);
			Feedback.Instigator = Source;
			Feedback.EffectCauser = Source;
			Feedback.Location = Victim->GetActorLocation();
			ASC->ExecuteGameplayCue(GOTATags::Cue_Resisted, Feedback);
		}
		if (bWhole)
		{
			return false;
		}

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
		// A blocked aspect gets a neutral x1 (it then never wins the "strongest penalty" aggregation).
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Multiplier,
			(Blocked & Aspect_Movement) ? 1.f : FMath::Clamp(1.f - Magnitude, UGOTACombatAttributeSet::MinMultiplier, 1.f));
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_DamageMultiplier,
			(Blocked & Aspect_Weaken) ? 1.f : FMath::Clamp(1.f - Magnitude * 0.5f, UGOTACombatAttributeSet::MinMultiplier, 1.f));
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (!Handle.WasSuccessfullyApplied())
		{
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s rejected (immune) from %s"), *GetNameSafe(Victim), *Name, *GetNameSafe(Source));
			return false;
		}
		// Same leading wording as the ability component's slot log, so the regression suite reads both.
		UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s %.1fs x%.2f (from %s) [GAS walk x%.2f dmg x%.2f]"), *GetNameSafe(Victim), *Name, Duration, Magnitude,
			*GetNameSafe(Source), ASC->GetMoveSpeedMultiplier(), ASC->GetOutgoingDamageMultiplier());
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
		TArray<TPair<FGameplayTag, UClass*>> Wanted;
		for (const FStatusInfo& Entry : Statuses)
		{
			Wanted.Emplace(Entry.CueTag.GetTag(), UGOTAGameplayCue_StatusOverlay::StaticClass());
		}
		Wanted.Emplace(GOTATags::Cue_Resisted.GetTag(), UGOTAGameplayCue_Resisted::StaticClass());
		TArray<FGameplayCueReferencePair> Missing;
		for (const TPair<FGameplayTag, UClass*>& W : Wanted)
		{
			if (!Set->GameplayCueDataMap.Contains(W.Key))
			{
				Missing.Emplace(W.Key, FSoftObjectPath(W.Value));
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
					Data.LoadedGameplayCueClass = Cast<UClass>(Pair.StringRef.ResolveObject());
				}
			}
		}
		UE_LOG(LogGOTAStatus, Log, TEXT("Registered %d native gameplay cue(s)"), Missing.Num());
	}
}
