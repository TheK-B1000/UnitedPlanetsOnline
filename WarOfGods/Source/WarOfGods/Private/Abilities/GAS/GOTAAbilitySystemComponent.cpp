// White Tiger Studios 2024

#include "Abilities/GAS/GOTAAbilitySystemComponent.h"

#include "Abilities/GAS/GOTACombatAttributeSet.h"
#include "Abilities/GAS/GOTAGameplayTags.h"
#include "Abilities/GAS/GOTAStatusEffects.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Characters/StatsComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "Events/CombatEvents.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAAbilitySystemComponent)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAGAS, Log, All);

UGOTAAbilitySystemComponent::UGOTAAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	ReplicationMode = EGameplayEffectReplicationMode::Mixed;
}

UGOTAAbilitySystemComponent* UGOTAAbilitySystemComponent::EnsureOnPawn(APawn* Pawn)
{
	if (!IsValid(Pawn) || !Pawn->HasAuthority())
	{
		return nullptr;
	}
	UGOTAAbilitySystemComponent* ASC = Find(Pawn);
	if (!ASC)
	{
		ASC = NewObject<UGOTAAbilitySystemComponent>(Pawn, TEXT("GOTA_AbilitySystem"));
		Pawn->AddInstanceComponent(ASC);
		ASC->RegisterComponent(); // InitializeComponent: InitAbilityActorInfo(Pawn, Pawn)
		UE_LOG(LogGOTAGAS, Log, TEXT("Attached ability system to %s"), *GetNameSafe(Pawn));
	}
	if (!ASC->GetSet<UGOTACombatAttributeSet>())
	{
		ASC->AddSet<UGOTACombatAttributeSet>();
	}
	ASC->BindServerEvents(); // idempotent; picks up a stats component created after the ASC
	ASC->BindControlEvents();
	return ASC;
}

UGOTAAbilitySystemComponent* UGOTAAbilitySystemComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UGOTAAbilitySystemComponent>() : nullptr;
}

void UGOTAAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	// Cues are routed on every peer, so each process registers the native cue classes before the first one fires.
	GOTAGAS::RegisterNativeCues();
	BindServerEvents();
	BindControlEvents();
}

void UGOTAAbilitySystemComponent::BindControlEvents()
{
	if (bControlEventsBound || !GetOwner())
	{
		return;
	}
	bControlEventsBound = true;
	RegisterGameplayTagEvent(GOTATags::State_Incapacitated, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UGOTAAbilitySystemComponent::HandleIncapacitatedChanged);
}

void UGOTAAbilitySystemComponent::HandleIncapacitatedChanged(const FGameplayTag Tag, int32 NewCount)
{
	GOTAGAS::SetControlLock(Cast<APawn>(GetOwner()), NewCount > 0);
}

void UGOTAAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UStatsComponent* Stats = GetOwner() ? GetOwner()->FindComponentByClass<UStatsComponent>() : nullptr)
	{
		Stats->OnDied.RemoveDynamic(this, &UGOTAAbilitySystemComponent::HandleOwnerDied);
	}
	Super::EndPlay(EndPlayReason);
}

void UGOTAAbilitySystemComponent::BindServerEvents()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	if (!bServerEventsBound)
	{
		bServerEventsBound = true;
		RegisterGameplayTagEvent(GOTATags::Status_Veiled, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UGOTAAbilitySystemComponent::HandleVeiledChanged);
		RegisterGameplayTagEvent(GOTATags::Status_Unbreakable, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UGOTAAbilitySystemComponent::HandleUnbreakableChanged);
		OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UGOTAAbilitySystemComponent::HandleEffectAdded);
		OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UGOTAAbilitySystemComponent::HandleEffectRemoved);
	}
	if (UStatsComponent* Stats = Owner->FindComponentByClass<UStatsComponent>())
	{
		Stats->OnDied.AddUniqueDynamic(this, &UGOTAAbilitySystemComponent::HandleOwnerDied);
	}
}

void UGOTAAbilitySystemComponent::HandleVeiledChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Same flag, same writes as the pre-GAS slot: set on apply, cleared on removal. The damage system reads it.
	if (UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(GetOwner()))
	{
		FGOTABlueprintCombat::SetBool(DS, TEXT("IsInvincible"), NewCount > 0);
		UE_LOG(LogGOTAGAS, Log, TEXT("%s: Veiled %s -> IsInvincible=%d"), *GetNameSafe(GetOwner()), NewCount > 0 ? TEXT("on") : TEXT("off"), NewCount > 0 ? 1 : 0);
	}
}

void UGOTAAbilitySystemComponent::HandleUnbreakableChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(GetOwner()))
	{
		FGOTABlueprintCombat::SetBool(DS, TEXT("IsInterruptible"), NewCount == 0); // hyper armour while the tag is held
		UE_LOG(LogGOTAGAS, Log, TEXT("%s: Unbreakable %s -> IsInterruptible=%d"), *GetNameSafe(GetOwner()), NewCount > 0 ? TEXT("on") : TEXT("off"), NewCount == 0 ? 1 : 0);
	}
}

void UGOTAAbilitySystemComponent::HandleOwnerDied(const FCombatActorEvent& Event)
{
	ClearStatusEffects(TEXT("death"));
}

void UGOTAAbilitySystemComponent::ApplyStatusPolicy(const FGOTAStatusPolicy& InPolicy, FName PolicyOwner)
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}
	if (PolicyHandle.IsValid())
	{
		RemoveActiveGameplayEffect(PolicyHandle);
		PolicyHandle.Invalidate();
	}
	Policy = InPolicy;
	if (!Policy.Immunities.IsEmpty())
	{
		const FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(UGOTAGameplayEffect_StatusPolicy::StaticClass(), 1.f, MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->DynamicGrantedTags.AppendTags(Policy.Immunities);
			PolicyHandle = ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	UE_LOG(LogGOTAGAS, Log, TEXT("%s: status policy %s immune=[%s] resistances=%d"), *GetNameSafe(GetOwner()), *PolicyOwner.ToString(),
		*DescribeImmunities(), Policy.Resistances.Num());
}

void UGOTAAbilitySystemComponent::SetElementResistance(const FGameplayTag& Element, float DamageScale)
{
	if (!IsOwnerActorAuthoritative() || !Element.IsValid())
	{
		return;
	}
	FGOTAElementResistance* Existing = Policy.ElementResistances.FindByPredicate([&Element](const FGOTAElementResistance& E) { return E.Element == Element; });
	if (!Existing)
	{
		Existing = &Policy.ElementResistances.AddDefaulted_GetRef();
		Existing->Element = Element;
	}
	Existing->DamageScale = FMath::Clamp(DamageScale, 0.f, 3.f);
	UE_LOG(LogGOTAGAS, Log, TEXT("%s: element resistance %s x%.2f"), *GetNameSafe(GetOwner()), *Element.ToString(), Existing->DamageScale);
}

FString UGOTAAbilitySystemComponent::DescribeImmunities() const
{
	FGameplayTagContainer Owned;
	GetOwnedGameplayTags(Owned);
	FString Out;
	for (const FGameplayTag& Tag : Owned.Filter(FGameplayTagContainer(GOTATags::Immunity)))
	{
		Out += (Out.IsEmpty() ? TEXT("") : TEXT(" ")) + Tag.GetTagName().ToString().RightChop(14); // drop "GOTA.Immunity."
	}
	return Out;
}

float UGOTAAbilitySystemComponent::ReadMultiplier(const FGameplayAttribute& Attribute) const
{
	if (!GetSet<UGOTACombatAttributeSet>())
	{
		return 1.f;
	}
	return FMath::Clamp(GetNumericAttribute(Attribute), UGOTACombatAttributeSet::MinMultiplier, UGOTACombatAttributeSet::MaxMultiplier);
}

float UGOTAAbilitySystemComponent::GetMoveSpeedMultiplier() const
{
	return ReadMultiplier(UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute());
}

float UGOTAAbilitySystemComponent::GetOutgoingDamageMultiplier() const
{
	return ReadMultiplier(UGOTACombatAttributeSet::GetOutgoingDamageMultiplierAttribute());
}

float UGOTAAbilitySystemComponent::GetIncomingDamageMultiplier() const
{
	return ReadMultiplier(UGOTACombatAttributeSet::GetIncomingDamageMultiplierAttribute());
}

float UGOTAAbilitySystemComponent::GetShieldPool() const
{
	return GetSet<UGOTACombatAttributeSet>() ? FMath::Max(0.f, GetNumericAttribute(UGOTACombatAttributeSet::GetShieldPoolAttribute())) : 0.f;
}

void UGOTAAbilitySystemComponent::SetShieldPool(float Value)
{
	if (IsOwnerActorAuthoritative() && GetSet<UGOTACombatAttributeSet>())
	{
		SetNumericAttributeBase(UGOTACombatAttributeSet::GetShieldPoolAttribute(), FMath::Max(0.f, Value));
	}
}

void UGOTAAbilitySystemComponent::SetShieldBaseMaxHealth(float Value)
{
	UWorld* World = GetWorld();
	if (!IsOwnerActorAuthoritative() || !World)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (Value > 0.f)
	{
		Timers.ClearTimer(ShieldBaseClearTimer);
		ShieldBaseMaxHealth = Value;
	}
	else if (ShieldBaseMaxHealth > 0.f && !Timers.IsTimerActive(ShieldBaseClearTimer))
	{
		Timers.SetTimer(ShieldBaseClearTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ShieldBaseMaxHealth = 0.f; }), 1.f, false);
	}
}

int32 UGOTAAbilitySystemComponent::ClearStatusEffects(const TCHAR* Why)
{
	if (!IsOwnerActorAuthoritative())
	{
		return 0;
	}
	const int32 Removed = RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(GOTATags::Status));
	if (Removed > 0)
	{
		UE_LOG(LogGOTAGAS, Log, TEXT("%s: cleared %d status effect(s) (%s)"), *GetNameSafe(GetOwner()), Removed, Why);
	}
	return Removed;
}

void UGOTAAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGOTAAbilitySystemComponent, StatusTimers);
	DOREPLIFETIME(UGOTAAbilitySystemComponent, ShieldBaseMaxHealth);
	DOREPLIFETIME(UGOTAAbilitySystemComponent, ShieldedHealth);
}

float UGOTAAbilitySystemComponent::GetStatusTimeLeft(const FGOTAStatusTimer& Timer) const
{
	const UWorld* World = GetWorld();
	const float Left = World ? static_cast<float>(Timer.LocalEndTime - World->GetTimeSeconds()) : 0.f;
	return FMath::Clamp(Left, 0.f, Timer.Duration);
}

void UGOTAAbilitySystemComponent::MarkStatusTimersDirty()
{
	UWorld* World = GetWorld();
	if (bStatusTimersDirty || !World || World->bIsTearingDown || !IsOwnerActorAuthoritative())
	{
		return;
	}
	bStatusTimersDirty = true;
	// Next tick: a removal callback runs while the effect is still listed.
	World->GetTimerManager().SetTimerForNextTick(this, &UGOTAAbilitySystemComponent::RebuildStatusTimers);
}

void UGOTAAbilitySystemComponent::RebuildStatusTimers()
{
	bStatusTimersDirty = false;
	TArray<FGOTAStatusTimer> Fresh;
	GOTAGAS::CollectStatusTimers(this, Fresh);
	if (Fresh != StatusTimers)
	{
		StatusTimers = MoveTemp(Fresh);
		LogStatusTimers(TEXT("server"));
	}
}

void UGOTAAbilitySystemComponent::OnRep_StatusTimers()
{
	if (const UWorld* World = GetWorld())
	{
		const AGameStateBase* GS = World->GetGameState();
		const APlayerController* PC = World->GetFirstPlayerController();
		const float OneWay = PC && PC->PlayerState ? PC->PlayerState->GetPingInMilliseconds() * 0.0005f : 0.f;
		const double Now = World->GetTimeSeconds();
		for (FGOTAStatusTimer& Timer : StatusTimers)
		{
			// Time left as sent, less the trip here. The synced clock also answers, and catches a stale value from a late
			// join or a pawn becoming relevant. Both errors overstate the time, so the smaller is the better answer.
			double Left = Timer.RemainingAtSend - OneWay;
			if (GS)
			{
				Left = FMath::Min(Left, Timer.EndServerTime - GS->GetServerWorldTimeSeconds());
			}
			Timer.LocalEndTime = Now + FMath::Max(0.0, Left);
		}
	}
	LogStatusTimers(TEXT("client"));
}

void UGOTAAbilitySystemComponent::LogStatusTimers(const TCHAR* Where) const
{
	FString Out;
	for (const FGOTAStatusTimer& Timer : StatusTimers)
	{
		Out += FString::Printf(TEXT("%s%s %.1f/%.1f x%.2f"), Out.IsEmpty() ? TEXT("") : TEXT(" "),
			*StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(Timer.Status)), GetStatusTimeLeft(Timer), Timer.Duration, Timer.Magnitude);
	}
	UE_LOG(LogGOTAGAS, Log, TEXT("%s: STATUS TIMERS (%s) [%s]"), *GetNameSafe(GetOwner()), Where, *Out);
}

void UGOTAAbilitySystemComponent::HandleEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	MarkStatusTimersDirty();
}

void UGOTAAbilitySystemComponent::HandleEffectRemoved(const FActiveGameplayEffect& Effect)
{
	MarkStatusTimersDirty();
}

FString UGOTAAbilitySystemComponent::DescribeStatusEffects() const
{
	FString Out;
	const UWorld* World = GetWorld();
	if (!World)
	{
		return Out;
	}
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(GOTATags::Status));
	for (const FActiveGameplayEffectHandle& Handle : GetActiveEffects(Query))
	{
		const FActiveGameplayEffect* Effect = GetActiveGameplayEffect(Handle);
		if (!Effect || !Effect->Spec.Def)
		{
			continue;
		}
		FGameplayTagContainer Granted;
		Effect->Spec.GetAllGrantedTags(Granted);
		const FGameplayTag Status = Granted.Filter(FGameplayTagContainer(GOTATags::Status)).First();
		Out += FString::Printf(TEXT("%s%s %.1fs x%.2f"), Out.IsEmpty() ? TEXT("") : TEXT(" "),
			*Status.GetTagName().ToString().RightChop(12), // drop "GOTA.Status."
			Effect->GetTimeRemaining(World->GetTimeSeconds()),
			Effect->Spec.GetSetByCallerMagnitude(GOTATags::Data_Magnitude, false, 0.f));
	}
	return Out;
}
