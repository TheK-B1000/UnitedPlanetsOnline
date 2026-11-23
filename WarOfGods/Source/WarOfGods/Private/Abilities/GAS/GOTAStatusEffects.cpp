// White Tiger Studios 2024

#include "Abilities/GAS/GOTAStatusEffects.h"

#include "Abilities/GAS/GOTAAbilitySystemComponent.h"
#include "Abilities/GAS/GOTACombatAttributeSet.h"
#include "Abilities/GAS/GOTAElements.h"
#include "Abilities/GAS/GOTAGameplayTags.h"
#include "Abilities/GOTAHeroAbilityComponent.h"
#include "AbilitySystemGlobals.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Characters/GOTAHeroBodyComponent.h"
#include "Combat/GOTACombatLive.h"
#include "Combat/GOTACombatantObserver.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"
#include "HAL/IConsoleManager.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAStatusEffects)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAStatus, Log, All);

namespace
{
	struct FStatusInfo
	{
		EGOTAStatus Status;
		const FNativeGameplayTag& Tag;
		const FNativeGameplayTag& CueTag;
		/** Overlay materials authored by Tools/GOTA_BuildHeroAbilities.py. Null or a missing asset means no overlay. */
		const TCHAR* OverlayPath;
	};

	/** Marks the one looping status effect component a burning actor carries. */
	const FName BurnFXTag(TEXT("GOTA_BurnFX"));

	// Overlay priority order: the first active entry wins the body's single overlay slot. Hard crowd control is the most
	// important thing to read on a body, so it outranks every other status.
	const FStatusInfo Statuses[] = {
		{ EGOTAStatus::Frozen, GOTATags::Status_Frozen, GOTATags::Cue_Status_Frozen, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Frozen.M_GOTA_Overlay_Frozen") },
		{ EGOTAStatus::Stunned, GOTATags::Status_Stunned, GOTATags::Cue_Status_Stunned, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Stunned.M_GOTA_Overlay_Stunned") },
		{ EGOTAStatus::Veiled, GOTATags::Status_Veiled, GOTATags::Cue_Status_Veiled, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Veiled.M_GOTA_Overlay_Veiled") },
		{ EGOTAStatus::Shielded, GOTATags::Status_Shielded, GOTATags::Cue_Status_Shielded, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Shielded.M_GOTA_Overlay_Shielded") },
		{ EGOTAStatus::Unbreakable, GOTATags::Status_Unbreakable, GOTATags::Cue_Status_Unbreakable, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Unbreakable.M_GOTA_Overlay_Unbreakable") },
		{ EGOTAStatus::Vulnerable, GOTATags::Status_Vulnerable, GOTATags::Cue_Status_Vulnerable, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Vulnerable.M_GOTA_Overlay_Vulnerable") },
		{ EGOTAStatus::Dread, GOTATags::Status_Dread, GOTATags::Cue_Status_Dread, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Dread.M_GOTA_Overlay_Dread") },
		{ EGOTAStatus::Weakened, GOTATags::Status_Weakened, GOTATags::Cue_Status_Weakened, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Weakened.M_GOTA_Overlay_Weakened") },
		{ EGOTAStatus::Shocked, GOTATags::Status_Shocked, GOTATags::Cue_Status_Shocked, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Shocked.M_GOTA_Overlay_Shocked") },
		{ EGOTAStatus::Chilled, GOTATags::Status_Chilled, GOTATags::Cue_Status_Chilled, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Chilled.M_GOTA_Overlay_Chilled") },
		{ EGOTAStatus::Slowed, GOTATags::Status_Slowed, GOTATags::Cue_Status_Slowed, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Slowed.M_GOTA_Overlay_Slowed") },
		{ EGOTAStatus::Fortified, GOTATags::Status_Fortified, GOTATags::Cue_Status_Fortified, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Fortified.M_GOTA_Overlay_Fortified") },
		{ EGOTAStatus::Empowered, GOTATags::Status_Empowered, GOTATags::Cue_Status_Empowered, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Empowered.M_GOTA_Overlay_Empowered") },
		{ EGOTAStatus::Hasted, GOTATags::Status_Hasted, GOTATags::Cue_Status_Hasted, TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Hasted.M_GOTA_Overlay_Hasted") },
		// Burning shows as attached flames (UGOTAGameplayCue_Burning), not as an overlay, so it never hides another status.
		{ EGOTAStatus::Burning, GOTATags::Status_Burning, GOTATags::Cue_Status_Burning, nullptr },
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
		case EGOTAStatus::Burning: return UGOTAGameplayEffect_Burning::StaticClass();
		case EGOTAStatus::Stunned: return UGOTAGameplayEffect_Stunned::StaticClass();
		case EGOTAStatus::Frozen: return UGOTAGameplayEffect_Frozen::StaticClass();
		case EGOTAStatus::Shielded: return UGOTAGameplayEffect_Shielded::StaticClass();
		case EGOTAStatus::Empowered: return UGOTAGameplayEffect_Empowered::StaticClass();
		case EGOTAStatus::Fortified: return UGOTAGameplayEffect_Fortified::StaticClass();
		case EGOTAStatus::Vulnerable: return UGOTAGameplayEffect_Vulnerable::StaticClass();
		case EGOTAStatus::Hasted: return UGOTAGameplayEffect_Hasted::StaticClass();
		case EGOTAStatus::Weakened: return UGOTAGameplayEffect_Weakened::StaticClass();
		case EGOTAStatus::Chilled: return UGOTAGameplayEffect_Chilled::StaticClass();
		case EGOTAStatus::Shocked: return UGOTAGameplayEffect_Shocked::StaticClass();
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

	UParticleSystemComponent* FindBurnFX(const AActor* Actor)
	{
		TInlineComponentArray<UParticleSystemComponent*> Found(Actor);
		for (UParticleSystemComponent* PSC : Found)
		{
			if (PSC && PSC->ComponentHasTag(BurnFXTag))
			{
				return PSC;
			}
		}
		return nullptr;
	}

	/** One looping flame per burning actor, however many times the cue arrives. Owned by the actor, so it dies with it. */
	void EnsureBurnFX(AActor* Actor)
	{
		if (!Actor || Actor->GetNetMode() == NM_DedicatedServer || FindBurnFX(Actor))
		{
			return;
		}
		const FGOTAElementDef* Element = UGOTAElementSettings::FindByStatus(EGOTAStatus::Burning);
		UParticleSystem* FX = Element ? Element->StatusFX.LoadSynchronous() : nullptr;
		if (!FX || !Actor->GetRootComponent())
		{
			UE_LOG(LogGOTAStatus, Warning, TEXT("burn fx missing for %s (element StatusFX not set or not cooked)"), *GetNameSafe(Actor));
			return;
		}
		// The root (capsule centre) rather than the mesh: chest height on every body, and unaffected by mesh scale.
		if (UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAttached(FX, Actor->GetRootComponent(), NAME_None, FVector::ZeroVector,
			FRotator::ZeroRotator, FVector(Element->StatusFXScale), EAttachLocation::KeepRelativeOffset, false))
		{
			PSC->ComponentTags.Add(BurnFXTag);
			UE_LOG(LogGOTAStatus, Log, TEXT("burn fx %s on %s"), *GetNameSafe(FX), *GetNameSafe(Actor));
		}
	}

	void RemoveBurnFX(AActor* Actor)
	{
		while (UParticleSystemComponent* PSC = Actor ? FindBurnFX(Actor) : nullptr)
		{
			PSC->ComponentTags.Remove(BurnFXTag);
			PSC->DeactivateSystem();
			PSC->DestroyComponent();
			UE_LOG(LogGOTAStatus, Log, TEXT("burn fx removed from %s"), *GetNameSafe(Actor));
		}
	}

	int32 StatusDamageTicks = 0;
	float StatusDamageTotal = 0.f;

	TAutoConsoleVariable<float> CVarControlRecovery(TEXT("gota.GAS.ControlRecovery"), 2.f,
		TEXT("Seconds of hard crowd-control immunity (GOTA.Immunity.Stun) after a Stunned or Frozen ends. 0 = none."));

	/** Every skeletal mesh on the actor (the template mesh and the visible hero body). */
	void ForEachMesh(AActor* Actor, TFunctionRef<void(USkeletalMeshComponent*)> Fn)
	{
		TInlineComponentArray<USkeletalMeshComponent*> Meshes(Actor);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh)
			{
				Fn(Mesh);
			}
		}
	}

	/** Cut the current action's animation short (attack, block, cast). Returns how many montages were stopped. */
	int32 StopActionMontages(AActor* Actor)
	{
		int32 Stopped = 0;
		ForEachMesh(Actor, [&Stopped](USkeletalMeshComponent* Mesh)
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance(); Anim && Anim->IsAnyMontagePlaying())
			{
				Anim->Montage_Stop(0.15f);
				++Stopped;
			}
		});
		return Stopped;
	}

	/** Frozen: hold (or release) the body's animation. Cosmetic; movement is stopped by the lock. */
	void SetAnimationHeld(AActor* Actor, bool bHeld)
	{
		ForEachMesh(Actor, [bHeld](USkeletalMeshComponent* Mesh)
		{
			Mesh->bPauseAnims = bHeld;
		});
	}
}

// ============================================================================ effects

void UGOTAStatusGameplayEffect::ConfigureStatus(const FGameplayTag& StatusTag, const FGameplayTag& CueTag, const FGameplayTag& ImmunityTag,
	const FGameplayTag& ElementTag, const FGameplayTag& ExtraGrantedTag)
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
	if (ExtraGrantedTag.IsValid())
	{
		Granted.Added.AddTag(ExtraGrantedTag); // a derived state several statuses share (GOTA.State.Incapacitated)
	}
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	TargetTags->SetAndApplyTargetTagChanges(Granted);

	FInheritedTagContainer Asset;
	Asset.Added.AddTag(StatusTag);
	if (ElementTag.IsValid())
	{
		Asset.Added.AddTag(ElementTag); // the damage type, for future per-element resistances
	}
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
	if (ImmunityTag.IsValid())
	{
		Mod.TargetTags.IgnoreTags.AddTag(ImmunityTag);
	}
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

UGOTAGameplayEffect_Burning::UGOTAGameplayEffect_Burning()
{
	ConfigureStatus(GOTATags::Status_Burning, GOTATags::Cue_Status_Burning, GOTATags::Immunity_Burn, GOTATags::Element_Fire);
	// Periodic. The spec overrides the period with the element's TickInterval (GOTAGAS::ApplyStatus).
	Period.SetValue(1.f);
	bExecutePeriodicEffectOnApplication = false; // no free tick the moment it lands
	// Re-application refreshes the duration but keeps the running tick timer: the damage rate cannot be raised or
	// starved by re-applying faster than the period.
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	FGameplayEffectExecutionDefinition Tick;
	Tick.CalculationClass = UGOTABurnTickExecution::StaticClass();
	Executions.Add(Tick);
}

UGOTAGameplayEffect_Stunned::UGOTAGameplayEffect_Stunned()
{
	ConfigureStatus(GOTATags::Status_Stunned, GOTATags::Cue_Status_Stunned, GOTATags::Immunity_Stun, FGameplayTag(), GOTATags::State_Incapacitated);
	// Re-application never extends a running lock (GOTAGAS::ApplyStatus refuses it first; this is the engine-side rule).
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
}

UGOTAGameplayEffect_Frozen::UGOTAGameplayEffect_Frozen()
{
	ConfigureStatus(GOTATags::Status_Frozen, GOTATags::Cue_Status_Frozen, GOTATags::Immunity_Stun, GOTATags::Element_Frost, GOTATags::State_Incapacitated);
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
}

UGOTAGameplayEffect_Shielded::UGOTAGameplayEffect_Shielded()
{
	ConfigureStatus(GOTATags::Status_Shielded, GOTATags::Cue_Status_Shielded);
}

UGOTAGameplayEffect_Empowered::UGOTAGameplayEffect_Empowered()
{
	ConfigureStatus(GOTATags::Status_Empowered, GOTATags::Cue_Status_Empowered);
	AddMultiplier(UGOTACombatAttributeSet::GetOutgoingDamageMultiplierAttribute(), GOTATags::Data_DamageMultiplier, FGameplayTag());
}

UGOTAGameplayEffect_Fortified::UGOTAGameplayEffect_Fortified()
{
	ConfigureStatus(GOTATags::Status_Fortified, GOTATags::Cue_Status_Fortified);
	AddMultiplier(UGOTACombatAttributeSet::GetIncomingDamageMultiplierAttribute(), GOTATags::Data_IncomingMultiplier, FGameplayTag());
}

UGOTAGameplayEffect_Vulnerable::UGOTAGameplayEffect_Vulnerable()
{
	ConfigureStatus(GOTATags::Status_Vulnerable, GOTATags::Cue_Status_Vulnerable, GOTATags::Immunity_Vulnerable);
	AddMultiplier(UGOTACombatAttributeSet::GetIncomingDamageMultiplierAttribute(), GOTATags::Data_IncomingMultiplier, GOTATags::Immunity_Vulnerable);
}

UGOTAGameplayEffect_Hasted::UGOTAGameplayEffect_Hasted()
{
	ConfigureStatus(GOTATags::Status_Hasted, GOTATags::Cue_Status_Hasted);
	AddMultiplier(UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute(), GOTATags::Data_Multiplier, FGameplayTag());
}

UGOTAGameplayEffect_Weakened::UGOTAGameplayEffect_Weakened()
{
	ConfigureStatus(GOTATags::Status_Weakened, GOTATags::Cue_Status_Weakened, GOTATags::Immunity_Weaken);
	AddMultiplier(UGOTACombatAttributeSet::GetOutgoingDamageMultiplierAttribute(), GOTATags::Data_DamageMultiplier, GOTATags::Immunity_Weaken);
}

UGOTAGameplayEffect_Chilled::UGOTAGameplayEffect_Chilled()
{
	ConfigureStatus(GOTATags::Status_Chilled, GOTATags::Cue_Status_Chilled, GOTATags::Immunity_Slow, GOTATags::Element_Frost);
	AddMultiplier(UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute(), GOTATags::Data_Multiplier, GOTATags::Immunity_Slow);
}

UGOTAGameplayEffect_Shocked::UGOTAGameplayEffect_Shocked()
{
	ConfigureStatus(GOTATags::Status_Shocked, GOTATags::Cue_Status_Shocked, GOTATags::Immunity_Vulnerable, GOTATags::Element_Lightning);
	AddMultiplier(UGOTACombatAttributeSet::GetIncomingDamageMultiplierAttribute(), GOTATags::Data_IncomingMultiplier, GOTATags::Immunity_Vulnerable);
}

UGOTAGameplayEffect_ControlRecovery::UGOTAGameplayEffect_ControlRecovery()
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
	FInheritedTagContainer Granted;
	Granted.Added.AddTag(GOTATags::Immunity_Stun);
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	TargetTags->SetAndApplyTargetTagChanges(Granted);
}

void UGOTABurnTickExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	APawn* Victim = TargetASC ? Cast<APawn>(TargetASC->GetOwner()) : nullptr;
	if (!Victim || !Victim->HasAuthority())
	{
		return;
	}
	// The instigator is whoever applied (or last refreshed) the effect; null once that pawn is gone.
	GOTAGAS::DealStatusDamage(Victim, Spec.GetContext().GetInstigator(), Spec.GetSetByCallerMagnitude(GOTATags::Data_TickDamage, false, 0.f),
		EGOTAStatus::Burning, GOTATags::Element_Fire);
}

// ============================================================================ overlay cue

bool UGOTAGameplayCue_StatusOverlay::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UE_LOG(LogGOTAStatus, Log, TEXT("cue %s on %s"), *Parameters.MatchedTagName.ToString(), *GetNameSafe(MyTarget));
	GOTAGAS::RefreshStatusOverlay(MyTarget, StatusForCue(Parameters));
	GOTAGAS::OnStatusCue.Broadcast(MyTarget, StatusForCue(Parameters), true);
	return true;
}

bool UGOTAGameplayCue_StatusOverlay::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// A replicated effect can reach a client as WhileActive only (no OnActive), so this logs the same way.
	UE_LOG(LogGOTAStatus, Log, TEXT("cue %s on %s (while active)"), *Parameters.MatchedTagName.ToString(), *GetNameSafe(MyTarget));
	GOTAGAS::RefreshStatusOverlay(MyTarget, StatusForCue(Parameters));
	GOTAGAS::OnStatusCue.Broadcast(MyTarget, StatusForCue(Parameters), true);
	return true;
}

bool UGOTAGameplayCue_StatusOverlay::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UE_LOG(LogGOTAStatus, Log, TEXT("cue %s removed from %s"), *Parameters.MatchedTagName.ToString(), *GetNameSafe(MyTarget));
	GOTAGAS::RefreshStatusOverlay(MyTarget, EGOTAStatus::None, StatusForCue(Parameters));
	GOTAGAS::OnStatusCue.Broadcast(MyTarget, StatusForCue(Parameters), false);
	return true;
}

// ============================================================================ burning cue

bool UGOTAGameplayCue_Burning::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	EnsureBurnFX(MyTarget);
	return Super::OnActive_Implementation(MyTarget, Parameters);
}

bool UGOTAGameplayCue_Burning::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	EnsureBurnFX(MyTarget);
	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool UGOTAGameplayCue_Burning::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	RemoveBurnFX(MyTarget);
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

// ============================================================================ hard control cue

bool UGOTAGameplayCue_HardControl::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const EGOTAStatus Status = StatusForCue(Parameters);
	const int32 Stopped = StopActionMontages(MyTarget);
	if (Status == EGOTAStatus::Frozen)
	{
		SetAnimationHeld(MyTarget, true);
	}
	UE_LOG(LogGOTAStatus, Log, TEXT("control cue %s on %s: %d montage(s) cut, animation %s"), *Parameters.MatchedTagName.ToString(),
		*GetNameSafe(MyTarget), Stopped, Status == EGOTAStatus::Frozen ? TEXT("held") : TEXT("running"));
	return Super::OnActive_Implementation(MyTarget, Parameters);
}

bool UGOTAGameplayCue_HardControl::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// A client that only receives WhileActive (joined or replicated late) must show the same thing.
	if (StatusForCue(Parameters) == EGOTAStatus::Frozen)
	{
		StopActionMontages(MyTarget);
		SetAnimationHeld(MyTarget, true);
	}
	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool UGOTAGameplayCue_HardControl::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (StatusForCue(Parameters) == EGOTAStatus::Frozen && MyTarget)
	{
		SetAnimationHeld(MyTarget, false);
		UE_LOG(LogGOTAStatus, Log, TEXT("control cue %s removed from %s: animation released"), *Parameters.MatchedTagName.ToString(), *GetNameSafe(MyTarget));
	}
	return Super::OnRemove_Implementation(MyTarget, Parameters);
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
	FOnStatusCue OnStatusCue;
	FOnStatusDamage OnStatusDamage;

	int32 AspectsOf(EGOTAStatus Status)
	{
		switch (Status)
		{
		case EGOTAStatus::Slowed: return Aspect_Movement;
		case EGOTAStatus::Dread: return Aspect_Movement | Aspect_Weaken;
		case EGOTAStatus::Burning: return Aspect_Burn;
		case EGOTAStatus::Stunned:
		case EGOTAStatus::Frozen: return Aspect_Control;
		case EGOTAStatus::Weakened: return Aspect_Weaken;
		case EGOTAStatus::Vulnerable: return Aspect_Vulnerable;
		case EGOTAStatus::Chilled: return Aspect_Movement;
		case EGOTAStatus::Shocked: return Aspect_Vulnerable;
		default: return 0; // buffs (Shielded, Empowered, Fortified, Hasted) and the self statuses
		}
	}

	bool KeepsStronger(EGOTAStatus Status)
	{
		return Status == EGOTAStatus::Shielded || Status == EGOTAStatus::Empowered || Status == EGOTAStatus::Fortified
			|| Status == EGOTAStatus::Vulnerable || Status == EGOTAStatus::Hasted || Status == EGOTAStatus::Weakened
			|| Status == EGOTAStatus::Chilled || Status == EGOTAStatus::Shocked;
	}

	float GetIncomingDamageMultiplier(const AActor* Actor)
	{
		const UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(Actor);
		return ASC ? ASC->GetIncomingDamageMultiplier() : 1.f;
	}

	float GetElementDamageScale(const AActor* Victim, const FGameplayTag& Element)
	{
		const UGOTAAbilitySystemComponent* ASC = Element.IsValid() ? UGOTAAbilitySystemComponent::Find(Victim) : nullptr;
		return ASC ? ASC->GetElementDamageScale(Element) : 1.f;
	}

	EGOTAStatus ApplyElementOnHit(APawn* Victim, const FGameplayTag& Element, AActor* Source)
	{
		const FGOTAElementDef* Row = UGOTAElementSettings::Find(Element);
		static const IConsoleVariable* ElementStatusVar = IConsoleManager::Get().FindConsoleVariable(TEXT("gota.GAS.ElementStatus"));
		if (!Row || !Victim || !Victim->HasAuthority() || UGOTACombatantObserver::IsCombatantDefeated(Victim)
			|| (ElementStatusVar && ElementStatusVar->GetInt() == 0))
		{
			return EGOTAStatus::None;
		}
		UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(Victim);
		const FString Who = GetNameSafe(Victim);
		// The interaction table: few, fixed, readable. Each one replaces the hit's own status.
		if (Element == GOTATags::Element_Fire && ASC && (HasStatus(Victim, EGOTAStatus::Chilled) || HasStatus(Victim, EGOTAStatus::Frozen)))
		{
			FGameplayTagContainer Ice;
			Ice.AddTag(GOTATags::Status_Chilled);
			Ice.AddTag(GOTATags::Status_Frozen);
			ASC->RemoveActiveEffectsWithGrantedTags(Ice);
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: ELEMENT Fire thawed Chilled/Frozen (no Burning from this hit) from %s"), *Who, *GetNameSafe(Source));
			return EGOTAStatus::None;
		}
		if (Element == GOTATags::Element_Frost && ASC && HasStatus(Victim, EGOTAStatus::Burning))
		{
			ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(GOTATags::Status_Burning));
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: ELEMENT Frost extinguished Burning (no Chilled from this hit) from %s"), *Who, *GetNameSafe(Source));
			return EGOTAStatus::None;
		}
		if (Row->Status == EGOTAStatus::None || Row->StatusDuration <= 0.f)
		{
			return EGOTAStatus::None;
		}
		if (Row->FollowUpStatus != EGOTAStatus::None && Row->FollowUpDuration > 0.f && HasStatus(Victim, Row->Status) && ASC)
		{
			const FStatusInfo* First = Info(Row->Status);
			ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(First->Tag.GetTag()));
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: ELEMENT %s follow-up: %s -> %s %.1fs from %s"), *Who, *Row->DisplayName.ToString(),
				*StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(Row->Status)),
				*StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(Row->FollowUpStatus)), Row->FollowUpDuration, *GetNameSafe(Source));
			UGOTAHeroAbilityComponent::ApplyStatusTo(Victim, Row->FollowUpStatus, Row->FollowUpDuration, 0.f, Source);
			return Row->FollowUpStatus;
		}
		UGOTAHeroAbilityComponent::ApplyStatusTo(Victim, Row->Status, Row->StatusDuration, Row->StatusMagnitude, Source);
		return Row->Status;
	}

	bool DealElementalHit(APawn* Victim, AActor* Source, float Amount, const FGameplayTag& Element, bool bCanBeBlocked, const FString& Response)
	{
		if (!Victim || !Victim->HasAuthority() || Amount <= 0.f || UGOTACombatantObserver::IsCombatantDefeated(Victim))
		{
			return false;
		}
		const float Scale = GetElementDamageScale(Victim, Element);
		const float Scaled = Amount * Scale;
		const bool bLanded = Scaled > 0.f && FGOTABlueprintCombat::ApplyDamage(Victim, Source, Scaled, bCanBeBlocked, false, Response);
		UE_LOG(LogGOTAStatus, Log, TEXT("%s: ELEMENT hit %s %.1f (x%.2f element) -> %.1f (landed=%d) from %s"), *GetNameSafe(Victim), *Element.ToString(),
			Amount, Scale, Scaled, bLanded ? 1 : 0, *GetNameSafe(Source));
		if (bLanded)
		{
			ApplyElementOnHit(Victim, Element, Source);
		}
		return bLanded;
	}

	bool IsIncapacitated(const AActor* Actor)
	{
		return HasTag(Actor, GOTATags::State_Incapacitated);
	}

	float GetControlRecoverySeconds()
	{
		return FMath::Max(0.f, CVarControlRecovery.GetValueOnGameThread());
	}

	bool HasTag(const AActor* Actor, const FGameplayTag& Tag)
	{
		const UGOTAAbilitySystemComponent* ASC = UGOTAAbilitySystemComponent::Find(Actor);
		return ASC && ASC->HasMatchingGameplayTag(Tag);
	}

	bool OwnsStatus(EGOTAStatus Status)
	{
		// Migration order (one status per verified step): Slowed, Dread, Veiled, Unbreakable.
		// Burning has only ever been a gameplay effect.
		return Status == EGOTAStatus::Slowed || Status == EGOTAStatus::Dread || Status == EGOTAStatus::Veiled || Status == EGOTAStatus::Unbreakable
			|| Status == EGOTAStatus::Burning || IsHardControl(Status) || KeepsStronger(Status);
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

		// Hard crowd control rules (before the policy, which would show "resists" for these cases):
		if (IsHardControl(Status))
		{
			// An invincible target (Veiled, a dodge's i-frames, a boss's floating phase) is untouchable by control too.
			if (const UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Victim);
				DS && FGOTABlueprintCombat::GetBool(DS, TEXT("IsInvincible")))
			{
				UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s refused (invincible) from %s"), *GetNameSafe(Victim), *Name, *GetNameSafe(Source));
				return false;
			}
			// One hard CC at a time, never extended: two casters cannot keep a target locked.
			if (IsIncapacitated(Victim))
			{
				UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s refused (already incapacitated, %s %.1fs left; not extended) from %s"), *GetNameSafe(Victim),
					*Name, HasStatus(Victim, EGOTAStatus::Frozen) ? TEXT("Frozen") : TEXT("Stunned"),
					FMath::Max(GetStatusRemaining(Victim, EGOTAStatus::Frozen), GetStatusRemaining(Victim, EGOTAStatus::Stunned)), *GetNameSafe(Source));
				return false;
			}
		}

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
		if ((Aspects & Aspect_Burn) && ASC->HasMatchingGameplayTag(GOTATags::Immunity_Burn))
		{
			Blocked |= Aspect_Burn;
		}
		if ((Aspects & Aspect_Control) && ASC->HasMatchingGameplayTag(GOTATags::Immunity_Stun))
		{
			Blocked |= Aspect_Control; // a boss policy, or the recovery right after a stun
		}
		if ((Aspects & Aspect_Vulnerable) && ASC->HasMatchingGameplayTag(GOTATags::Immunity_Vulnerable))
		{
			Blocked |= Aspect_Vulnerable;
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
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s resisted (%s%s%s%s%s%s) from %s"), *GetNameSafe(Victim), *Name,
				bWhole ? TEXT("whole ") : TEXT(""), (Report & Aspect_Movement) ? TEXT("movement ") : TEXT(""),
				(Report & Aspect_Weaken) ? TEXT("weaken ") : TEXT(""), (Report & Aspect_Burn) ? TEXT("burn ") : TEXT(""),
				(Report & Aspect_Control) ? TEXT("control ") : TEXT(""), (Report & Aspect_Reduced) ? TEXT("reduced") : TEXT(""), *GetNameSafe(Source));
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

		// Buffs and debuffs: one per target. Re-applying refreshes the duration and keeps the running strength; a stronger
		// application replaces the running one (two sources of different strength: the stronger wins, never both).
		if (KeepsStronger(Status) && HasStatus(Victim, Status))
		{
			const float Running = GetStatusMagnitude(Victim, Status);
			if (Magnitude > Running + KINDA_SMALL_NUMBER)
			{
				const FStatusInfo* Entry = Info(Status);
				ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(Entry->Tag.GetTag()));
				UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s replaced (x%.2f -> x%.2f, stronger) from %s"), *GetNameSafe(Victim), *Name, Running, Magnitude,
					*GetNameSafe(Source));
			}
			else
			{
				UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s refreshed (keeps x%.2f; x%.2f is not stronger) from %s"), *GetNameSafe(Victim), *Name, Running,
					Magnitude, *GetNameSafe(Source));
				// A stacked re-application takes the new spec's numbers in GAS, so it is made with the running strength:
				// only the duration changes.
				Magnitude = Running;
			}
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
		// A blocked aspect gets a neutral x1 (it then never wins the "strongest penalty" aggregation). Each status's
		// formula; every multiplier stays inside the attribute range [0.2, 3].
		const float MinM = UGOTACombatAttributeSet::MinMultiplier;
		const float MaxM = UGOTACombatAttributeSet::MaxMultiplier;
		float MoveMult = 1.f, DamageMult = 1.f, IncomingMult = 1.f;
		switch (Status)
		{
		case EGOTAStatus::Slowed: MoveMult = FMath::Clamp(1.f - Magnitude, MinM, 1.f); break;
		case EGOTAStatus::Dread:
			MoveMult = FMath::Clamp(1.f - Magnitude, MinM, 1.f);
			DamageMult = FMath::Clamp(1.f - Magnitude * 0.5f, MinM, 1.f);
			break;
		case EGOTAStatus::Hasted: MoveMult = FMath::Clamp(1.f + Magnitude, 1.f, MaxM); break;
		case EGOTAStatus::Empowered: DamageMult = FMath::Clamp(1.f + Magnitude, 1.f, MaxM); break;
		case EGOTAStatus::Weakened: DamageMult = FMath::Clamp(1.f - Magnitude, MinM, 1.f); break;
		case EGOTAStatus::Fortified: IncomingMult = FMath::Clamp(1.f - Magnitude, MinM, 1.f); break;
		case EGOTAStatus::Vulnerable:
		case EGOTAStatus::Shocked: IncomingMult = FMath::Clamp(1.f + Magnitude, 1.f, MaxM); break;
		case EGOTAStatus::Chilled: MoveMult = FMath::Clamp(1.f - Magnitude, MinM, 1.f); break;
		default: break;
		}
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Multiplier, (Blocked & Aspect_Movement) ? 1.f : MoveMult);
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_DamageMultiplier, (Blocked & Aspect_Weaken) ? 1.f : DamageMult);
		Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_IncomingMultiplier, (Blocked & Aspect_Vulnerable) ? 1.f : IncomingMult);
		// Damage over time: Magnitude is damage per second; the element row gives the tick interval.
		float TickInterval = 0.f;
		float TickDamage = 0.f;
		const bool bRefresh = HasStatus(Victim, Status);
		if (Status == EGOTAStatus::Burning)
		{
			const FGOTAElementDef* Element = UGOTAElementSettings::FindByStatus(Status);
			TickInterval = FMath::Max(Element ? Element->TickInterval : 1.f, 0.1f);
			TickDamage = Magnitude * TickInterval;
			Spec.Data->Period = TickInterval;
			Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_TickDamage, TickDamage);
		}
		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (!Handle.WasSuccessfullyApplied())
		{
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s rejected (immune) from %s"), *GetNameSafe(Victim), *Name, *GetNameSafe(Source));
			return false;
		}
		// Same leading wording as the ability component's slot log, so the regression suite reads both.
		UE_LOG(LogGOTAStatus, Log, TEXT("%s: status %s %.1fs x%.2f (from %s) [GAS walk x%.2f dmg x%.2f]"), *GetNameSafe(Victim), *Name, Duration, Magnitude,
			*GetNameSafe(Source), ASC->GetMoveSpeedMultiplier(), ASC->GetOutgoingDamageMultiplier());
		if (TickInterval > 0.f)
		{
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: %s %s: %.1f per tick every %.2fs for %.1fs, owner %s"), *GetNameSafe(Victim), *Name,
				bRefresh ? TEXT("refreshed") : TEXT("applied"), TickDamage, TickInterval, Duration, *GetNameSafe(Source));
		}
		return true;
	}

	float DealStatusDamage(APawn* Victim, AActor* Instigator, float Amount, EGOTAStatus Status, const FGameplayTag& Element)
	{
		if (!Victim || !Victim->HasAuthority() || Amount <= 0.f || UGOTACombatantObserver::IsCombatantDefeated(Victim)
			|| !GOTACombatLive::IsCombatLive(Victim->GetWorld()))
		{
			return 0.f;
		}
		// The victim's resistance to the tick's element (Burning: Fire), as for a hit of that element.
		Amount *= GetElementDamageScale(Victim, Element);
		if (Amount <= 0.f)
		{
			UE_LOG(LogGOTAStatus, Log, TEXT("%s: %s tick on %s resisted by element (%s)"), *GetNameSafe(Instigator),
				*StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(Status)), *GetNameSafe(Victim), *Element.ToString());
			return 0.f;
		}
		const float Before = UGOTACombatantObserver::ReadVitals(Victim).Health;
		// "None" response and no forced interrupt: a tick is not a hit, so it must not stagger or cancel an attack.
		const double Started = FPlatformTime::Seconds();
		const bool bLanded = FGOTABlueprintCombat::ApplyDamage(Victim, Instigator, Amount, false, false, TEXT("None"), false);
		const double DamageMs = (FPlatformTime::Seconds() - Started) * 1000.0; // the Blueprint damage call alone, for Horde load checks
		const float After = UGOTACombatantObserver::ReadVitals(Victim).Health;
		const float Dealt = FMath::Max(0.f, Before - After);
		++StatusDamageTicks;
		StatusDamageTotal += Dealt;
		UE_LOG(LogGOTAStatus, Log, TEXT("%s: %s tick on %s for %.1f (landed=%d) hp %.1f -> %.1f [%s] %.2fms"), *GetNameSafe(Instigator),
			*StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(Status)), *GetNameSafe(Victim), Amount, bLanded ? 1 : 0, Before, After,
			*Element.ToString(), DamageMs);
		if (Dealt > 0.f)
		{
			// Hit-driven rules (cast interrupt, "hit landed" momentum) watch health, so tell them this was not a hit.
			OnStatusDamage.Broadcast(Victim, Dealt);
		}
		return Dealt;
	}

	FStatusDamageStats GetStatusDamageStats()
	{
		return { StatusDamageTicks, StatusDamageTotal };
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

	void SetControlLock(APawn* Pawn, bool bLocked)
	{
		if (!Pawn)
		{
			return;
		}
		const bool bDefeated = UGOTACombatantObserver::IsCombatantDefeated(Pawn);
		if (Pawn->HasAuthority())
		{
			ACharacter* Char = Cast<ACharacter>(Pawn);
			AAIController* AIC = Cast<AAIController>(Pawn->GetController());
			if (bLocked)
			{
				// Cut the current action short: hero-ability cast, block, attack animation. Damage already dealt stands.
				if (UGOTAHeroAbilityComponent* Abilities = Pawn->FindComponentByClass<UGOTAHeroAbilityComponent>())
				{
					Abilities->InterruptCast(TEXT("incapacitated"));
				}
				const UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Pawn);
				const bool bWasBlocking = DS && FGOTABlueprintCombat::GetBool(DS, TEXT("IsBlocking"));
				if (bWasBlocking)
				{
					FGOTABlueprintCombat::Call(Pawn, TEXT("EndBlock"));
				}
				const int32 Stopped = StopActionMontages(Pawn);
				if (Char)
				{
					Char->GetCharacterMovement()->StopMovementImmediately();
				}
				if (AIC)
				{
					AIC->StopMovement();
					AIC->ClearFocus(EAIFocusPriority::Gameplay);
					if (UBrainComponent* Brain = AIC->GetBrainComponent())
					{
						Brain->PauseLogic(TEXT("GOTA control"));
					}
				}
				UE_LOG(LogGOTAStatus, Log, TEXT("%s: CONTROL lock on (block ended=%d, montages cut=%d, ai paused=%d)"), *GetNameSafe(Pawn),
					bWasBlocking ? 1 : 0, Stopped, AIC ? 1 : 0);
			}
			else
			{
				// Never revive a defeated AI's brain (UStatsComponent::StopDefeatedAI stopped it on purpose).
				if (AIC && !bDefeated)
				{
					if (UBrainComponent* Brain = AIC->GetBrainComponent())
					{
						// Resume, then restart from the root: the move the lock stopped is gone, and a resumed move task would
						// wait for it forever (RC14 Horde: released enemies far from players never walked again). The
						// blackboard (state, attack target) is kept, so the tree picks up the same fight.
						Brain->ResumeLogic(TEXT("GOTA control"));
						Brain->RestartLogic();
					}
				}
				const float Recovery = GetControlRecoverySeconds();
				if (!bDefeated && Recovery > 0.f && Pawn->GetWorld())
				{
					// Next tick: this runs inside the ASC's own effect removal, which must finish before a new effect is added.
					TWeakObjectPtr<APawn> WeakPawn(Pawn);
					Pawn->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakPawn, Recovery]()
					{
						APawn* P = WeakPawn.Get();
						UGOTAAbilitySystemComponent* ASC = P ? UGOTAAbilitySystemComponent::Find(P) : nullptr;
						if (!ASC || UGOTACombatantObserver::IsCombatantDefeated(P))
						{
							return;
						}
						const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGOTAGameplayEffect_ControlRecovery::StaticClass(), 1.f, ASC->MakeEffectContext());
						if (Spec.IsValid())
						{
							Spec.Data->SetSetByCallerMagnitude(GOTATags::Data_Duration, Recovery);
							ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
							UE_LOG(LogGOTAStatus, Log, TEXT("%s: control recovery: immune to Stun/Freeze for %.1fs"), *GetNameSafe(P), Recovery);
						}
					});
				}
				UE_LOG(LogGOTAStatus, Log, TEXT("%s: CONTROL lock off (ai resumed=%d, recovery immunity %.1fs%s)"), *GetNameSafe(Pawn),
					AIC && !bDefeated ? 1 : 0, bDefeated ? 0.f : Recovery, bDefeated ? TEXT(", defeated") : TEXT(""));
			}
		}
		// The player at the keyboard: no move, attack, block or dodge input while locked (the server enforces the
		// rest). On a listen server the host's own pawn takes both branches.
		if (Pawn->IsLocallyControlled())
		{
			if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
			{
				if (bLocked)
				{
					Pawn->DisableInput(PC);
				}
				else
				{
					Pawn->EnableInput(PC);
				}
				UE_LOG(LogGOTAStatus, Log, TEXT("%s: CONTROL input %s (local)"), *GetNameSafe(Pawn), bLocked ? TEXT("disabled") : TEXT("enabled"));
			}
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
			Wanted.Emplace(Entry.CueTag.GetTag(), Entry.Status == EGOTAStatus::Burning ? UGOTAGameplayCue_Burning::StaticClass()
				: IsHardControl(Entry.Status) ? UGOTAGameplayCue_HardControl::StaticClass() : UGOTAGameplayCue_StatusOverlay::StaticClass());
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
