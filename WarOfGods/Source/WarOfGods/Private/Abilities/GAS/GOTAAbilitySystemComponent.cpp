// White Tiger Studios 2024

#include "Abilities/GAS/GOTAAbilitySystemComponent.h"

#include "Abilities/GAS/GOTACombatAttributeSet.h"
#include "Abilities/GAS/GOTAGameplayTags.h"
#include "Abilities/GAS/GOTAStatusEffects.h"
#include "GameFramework/Pawn.h"

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
}

float UGOTAAbilitySystemComponent::GetMoveSpeedMultiplier() const
{
	if (!GetSet<UGOTACombatAttributeSet>())
	{
		return 1.f;
	}
	return FMath::Clamp(GetNumericAttribute(UGOTACombatAttributeSet::GetMoveSpeedMultiplierAttribute()),
		UGOTACombatAttributeSet::MinMoveSpeedMultiplier, 2.f);
}

int32 UGOTAAbilitySystemComponent::ClearStatusEffects()
{
	if (!IsOwnerActorAuthoritative())
	{
		return 0;
	}
	const int32 Removed = RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(GOTATags::Status));
	if (Removed > 0)
	{
		UE_LOG(LogGOTAGAS, Log, TEXT("%s: cleared %d status effect(s)"), *GetNameSafe(GetOwner()), Removed);
	}
	return Removed;
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
			Effect->Spec.GetSetByCallerMagnitude(GOTATags::Data_Multiplier, false, 1.f));
	}
	return Out;
}
