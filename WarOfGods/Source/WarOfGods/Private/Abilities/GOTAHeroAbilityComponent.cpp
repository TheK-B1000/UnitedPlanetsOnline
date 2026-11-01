// White Tiger Studios 2024

#include "Abilities/GOTAHeroAbilityComponent.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "BrainComponent.h"
#include "Characters/GOTAHeroBodyComponent.h"
#include "Characters/GOTAHeroDefinition.h"
#include "Characters/GOTAHeroRegistry.h"
#include "Characters/StatsComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/GOTAPlayerCameraManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAHeroAbilityComponent)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAAbility, Log, All);

static TAutoConsoleVariable<int32> CVarGOTAAbilityAnimDebug(TEXT("gota.Ability.AnimDebug"), 0, TEXT("1 = log when hero ability body montages end."));

namespace GOTAAbilityAssets
{
	// Status overlays (Tools/GOTA_BuildHeroAbilities.py). Missing assets simply mean no overlay.
	static const TCHAR* OverlayUnbreakable = TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Unbreakable.M_GOTA_Overlay_Unbreakable");
	static const TCHAR* OverlayVeiled = TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Veiled.M_GOTA_Overlay_Veiled");
	static const TCHAR* OverlayDread = TEXT("/Game/Heroes/Abilities/Materials/M_GOTA_Overlay_Dread.M_GOTA_Overlay_Dread");
	static const TCHAR* DeniedSound = TEXT("/Game/Audio/UI/S_GOTA_UI_Error.S_GOTA_UI_Error");
	static const FName BodySlot(TEXT("DefaultSlot"));
}

UGOTAHeroAbilityComponent::UGOTAHeroAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f; // charge / multi-hit timing needs every frame on the server
	SetIsReplicatedByDefault(true);
}

UGOTAHeroAbilityComponent* UGOTAHeroAbilityComponent::ApplyToPawn(APawn* Pawn, FName InHeroId)
{
	if (!Pawn || !Pawn->HasAuthority() || InHeroId.IsNone())
	{
		return nullptr;
	}
	const UGameInstance* GI = Pawn->GetGameInstance();
	const UGOTAHeroRegistry* Registry = GI ? GI->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	const UGOTAHeroDefinition* Hero = Registry ? Registry->FindHero(InHeroId) : nullptr;
	if (!Hero || Hero->AbilitySet.IsNull())
	{
		return nullptr;
	}
	UGOTAHeroAbilityComponent* Comp = Pawn->FindComponentByClass<UGOTAHeroAbilityComponent>();
	if (!Comp)
	{
		Comp = NewObject<UGOTAHeroAbilityComponent>(Pawn, TEXT("GOTAHeroAbilities"));
		Comp->RegisterComponent();
		Pawn->AddInstanceComponent(Comp);
	}
	Comp->HeroId = InHeroId;
	Comp->LoadAbilitySet();
	UE_LOG(LogGOTAAbility, Log, TEXT("Abilities for %s on %s: %d"), *InHeroId.ToString(), *GetNameSafe(Pawn), Comp->GetNumAbilities());
	return Comp;
}

void UGOTAHeroAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGOTAHeroAbilityComponent, HeroId);
	DOREPLIFETIME(UGOTAHeroAbilityComponent, CooldownEnd);
	DOREPLIFETIME(UGOTAHeroAbilityComponent, ActiveSlot);
	DOREPLIFETIME(UGOTAHeroAbilityComponent, ActiveStatus);
	DOREPLIFETIME(UGOTAHeroAbilityComponent, StatusEnd);
	DOREPLIFETIME(UGOTAHeroAbilityComponent, StatusMagnitude);
}

void UGOTAHeroAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!HeroId.IsNone() && !AbilitySet)
	{
		LoadAbilitySet();
	}
	TryBindLocalInput();
}

void UGOTAHeroAbilityComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ResetAbilities(false);
	}
	if (bInputBound)
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		if (PC && PC->InputComponent)
		{
			PC->InputComponent->KeyBindings.RemoveAll([this](const FInputKeyBinding& B) { return B.KeyDelegate.IsBoundToObject(this); });
		}
		bInputBound = false;
	}
	Super::EndPlay(Reason);
}

void UGOTAHeroAbilityComponent::LoadAbilitySet()
{
	const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UGOTAHeroRegistry* Registry = GI ? GI->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	const UGOTAHeroDefinition* Hero = Registry ? Registry->FindHero(HeroId) : nullptr;
	AbilitySet = Hero ? Hero->AbilitySet.LoadSynchronous() : nullptr;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CooldownEnd.Init(0.f, GetNumAbilities());
	}
	// Preload presentation so the first cast doesn't hitch, and keep it referenced (soft pointers
	// alone don't stop garbage collection from unloading the assets again).
	PreloadedAssets.Reset();
	for (int32 i = 0; i < GetNumAbilities(); ++i)
	{
		const FGOTAAbilityDef& Def = AbilitySet->Abilities[i];
		PreloadedAssets.Add(Def.Animation.LoadSynchronous());
		PreloadedAssets.Add(Def.CastFX.LoadSynchronous());
		PreloadedAssets.Add(Def.ImpactFX.LoadSynchronous());
		PreloadedAssets.Add(Def.CastSound.LoadSynchronous());
		PreloadedAssets.Add(Def.ImpactSound.LoadSynchronous());
	}
}

void UGOTAHeroAbilityComponent::OnRep_HeroId()
{
	LoadAbilitySet();
}

int32 UGOTAHeroAbilityComponent::GetNumAbilities() const
{
	return AbilitySet ? AbilitySet->Abilities.Num() : 0;
}

const FGOTAAbilityDef* UGOTAHeroAbilityComponent::GetAbility(int32 Slot) const
{
	return (AbilitySet && AbilitySet->Abilities.IsValidIndex(Slot)) ? &AbilitySet->Abilities[Slot] : nullptr;
}

double UGOTAHeroAbilityComponent::ServerNow() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	return GS ? GS->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

float UGOTAHeroAbilityComponent::GetCooldownRemaining(int32 Slot) const
{
	return CooldownEnd.IsValidIndex(Slot) ? FMath::Max(0.f, CooldownEnd[Slot] - static_cast<float>(ServerNow())) : 0.f;
}

float UGOTAHeroAbilityComponent::GetStatusRemaining() const
{
	return ActiveStatus == EGOTAStatus::None ? 0.f : FMath::Max(0.f, StatusEnd - static_cast<float>(ServerNow()));
}

UStatsComponent* UGOTAHeroAbilityComponent::GetStats() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UStatsComponent>() : nullptr;
}

bool UGOTAHeroAbilityComponent::IsSlotReady(int32 Slot) const
{
	const FGOTAAbilityDef* Def = GetAbility(Slot);
	if (!Def || IsBusy() || GetCooldownRemaining(Slot) > 0.f)
	{
		return false;
	}
	if (Def->MomentumCost > 0.f)
	{
		float Have = 0.f;
		if (GetOwner()->HasAuthority())
		{
			const UStatsComponent* Stats = GetStats();
			Have = Stats ? Stats->GetDivineMomentum() : 0.f;
		}
		else if (const UGOTADuelFighterComponent* Fighter = GetOwner()->FindComponentByClass<UGOTADuelFighterComponent>())
		{
			Have = Fighter->GetMomentum(); // replicated mirror for the HUD
		}
		if (Have + KINDA_SMALL_NUMBER < Def->MomentumCost)
		{
			return false;
		}
	}
	return true;
}

bool UGOTAHeroAbilityComponent::IsRoundLive() const
{
	const ADuelGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr;
	return GS && GS->DuelPhase == EDuelPhase::InRound && GS->bCombatEnabled;
}

bool UGOTAHeroAbilityComponent::IsOwnerDead() const
{
	return UGOTACombatantObserver::IsCombatantDefeated(GetOwner());
}

APawn* UGOTAHeroAbilityComponent::GetOpponentPawn() const
{
	const UGOTADuelFighterComponent* Fighter = GetOwner() ? GetOwner()->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr;
	APawn* Opp = Fighter ? Fighter->GetOpponent() : nullptr;
	return (IsValid(Opp) && !UGOTACombatantObserver::IsCombatantDefeated(Opp)) ? Opp : nullptr;
}

float UGOTAHeroAbilityComponent::GetOutgoingDamageScale() const
{
	return (ActiveStatus == EGOTAStatus::Dread && GetStatusRemaining() > 0.f) ? FMath::Clamp(1.f - StatusMagnitude * 0.5f, 0.2f, 1.f) : 1.f;
}

float UGOTAHeroAbilityComponent::GetIncomingDamageScale() const
{
	// Unbreakable reduces ALL incoming damage the same way (see TickServer: refund through the
	// Blueprint's own Heal), so ability damage is not scaled twice here.
	return 1.f;
}

FString UGOTAHeroAbilityComponent::DescribeState() const
{
	FString S = FString::Printf(TEXT("hero=%s active=%d status=%s(%.1fs) cd=["), *HeroId.ToString(), ActiveSlot,
		*StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(ActiveStatus)), GetStatusRemaining());
	for (int32 i = 0; i < GetNumAbilities(); ++i)
	{
		S += FString::Printf(TEXT("%s%s:%.1f"), i ? TEXT(" ") : TEXT(""), *GetAbility(i)->AbilityId.ToString(), GetCooldownRemaining(i));
	}
	return S + TEXT("]");
}

// ============================================================================ input

void UGOTAHeroAbilityComponent::TryBindLocalInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (bInputBound || !PC || !PC->IsLocalController() || !PC->InputComponent)
	{
		return;
	}
	// BP_Player already uses 0/1/2 (stances), F, Tab, LeftAlt and the mouse; Q is the Divine Counter.
	struct FBind { FKey Key; int32 Slot; };
	const FBind Binds[] = {
		{ EKeys::E, 0 }, { EKeys::R, 1 }, { EKeys::C, 2 }, { EKeys::V, 3 },
		{ EKeys::Gamepad_DPad_Left, 0 }, { EKeys::Gamepad_DPad_Up, 1 }, { EKeys::Gamepad_DPad_Right, 2 }, { EKeys::Gamepad_DPad_Down, 3 },
	};
	for (const FBind& B : Binds)
	{
		FInputKeyBinding Binding(FInputChord(B.Key), IE_Pressed);
		const int32 Slot = B.Slot;
		Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, Slot]() { RequestActivate(Slot); });
		PC->InputComponent->KeyBindings.Add(Binding);
	}
	bInputBound = true;
}

void UGOTAHeroAbilityComponent::RequestActivate(int32 Slot)
{
	if (!GetAbility(Slot))
	{
		return;
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ActivateServer(Slot);
	}
	else
	{
		Server_Activate(static_cast<uint8>(Slot));
	}
}

void UGOTAHeroAbilityComponent::Server_Activate_Implementation(uint8 Slot)
{
	ActivateServer(Slot);
}

void UGOTAHeroAbilityComponent::Client_Denied_Implementation(uint8 Slot, EGOTAAbilityDenied Reason)
{
	LastDenied = Reason;
	LastDeniedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (USoundBase* S = LoadObject<USoundBase>(nullptr, GOTAAbilityAssets::DeniedSound, nullptr, LOAD_NoWarn))
	{
		UGameplayStatics::PlaySound2D(this, S, 0.5f);
	}
}

// ============================================================================ server: validate + activate

EGOTAAbilityDenied UGOTAHeroAbilityComponent::ValidateServer(int32 Slot, APawn*& OutTarget) const
{
	OutTarget = nullptr;
	const FGOTAAbilityDef* Def = GetAbility(Slot);
	APawn* Owner = Cast<APawn>(GetOwner());
	if (!Def || !Owner)
	{
		return EGOTAAbilityDenied::Invalid;
	}
	if (!IsRoundLive() || IsOwnerDead())
	{
		return EGOTAAbilityDenied::NotLive;
	}
	if (IsBusy())
	{
		return EGOTAAbilityDenied::Busy;
	}
	if (GetCooldownRemaining(Slot) > 0.f)
	{
		return EGOTAAbilityDenied::Cooldown;
	}
	const UStatsComponent* Stats = GetStats();
	if (Def->MomentumCost > 0.f && (!Stats || Stats->GetDivineMomentum() + KINDA_SMALL_NUMBER < Def->MomentumCost))
	{
		return EGOTAAbilityDenied::Momentum;
	}
	APawn* Opp = GetOpponentPawn();
	const UGOTAHeroAbilityComponent* OppAbilities = Opp ? Opp->FindComponentByClass<UGOTAHeroAbilityComponent>() : nullptr;
	const bool bOppTargetable = Opp && !(OppAbilities && OppAbilities->IsUntargetable());
	switch (Def->Kind)
	{
	case EGOTAAbilityKind::Pull:
	{
		// Targeted: needs a visible, targetable enemy in range.
		if (!bOppTargetable)
		{
			return EGOTAAbilityDenied::NoTarget;
		}
		if (FVector::Dist(Opp->GetActorLocation(), Owner->GetActorLocation()) > Def->Range)
		{
			return EGOTAAbilityDenied::OutOfRange;
		}
		if (!HasLineOfSight(Owner, Opp))
		{
			return EGOTAAbilityDenied::NoLineOfSight;
		}
		OutTarget = Opp;
		break;
	}
	case EGOTAAbilityKind::Teleport:
	{
		// Teleports behind the enemy when one is in range; otherwise forward. Destination checked at resolve too.
		if (bOppTargetable && FVector::Dist(Opp->GetActorLocation(), Owner->GetActorLocation()) <= Def->Range && HasLineOfSight(Owner, Opp))
		{
			OutTarget = Opp;
		}
		FVector Dest;
		FRotator Rot;
		if (!FindTeleportDestination(OutTarget, Def->Range, Dest, Rot))
		{
			return EGOTAAbilityDenied::NoSafeDestination;
		}
		break;
	}
	default:
		OutTarget = bOppTargetable ? Opp : nullptr;
		break;
	}
	return EGOTAAbilityDenied::None;
}

void UGOTAHeroAbilityComponent::ActivateServer(int32 Slot)
{
	APawn* Target = nullptr;
	const EGOTAAbilityDenied Denied = ValidateServer(Slot, Target);
	const FGOTAAbilityDef* Def = GetAbility(Slot);
	APawn* Owner = Cast<APawn>(GetOwner());
	if (Denied != EGOTAAbilityDenied::None || !Def || !Owner)
	{
		UE_LOG(LogGOTAAbility, Log, TEXT("%s: %s denied (%s)"), *GetNameSafe(Owner), Def ? *Def->AbilityId.ToString() : TEXT("?"),
			*StaticEnum<EGOTAAbilityDenied>()->GetNameStringByValue(static_cast<int64>(Denied)));
		if (Owner && Owner->IsPlayerControlled())
		{
			Client_Denied(static_cast<uint8>(Slot), Denied);
		}
		return;
	}
	if (Def->MomentumCost > 0.f)
	{
		UStatsComponent* Stats = GetStats();
		if (!Stats || !Stats->ConsumeDivineMomentum(Def->MomentumCost))
		{
			Client_Denied(static_cast<uint8>(Slot), EGOTAAbilityDenied::Momentum);
			return;
		}
	}
	const double Now = ServerNow();
	CooldownEnd[Slot] = static_cast<float>(Now + Def->Cooldown);
	ActiveSlot = Slot;
	CastTarget = Target;
	CastResolveTime = Now + Def->CastTime;
	CastFinishTime = -1.0;
	PendingHits = FMath::Max(1, Def->Kind == EGOTAAbilityKind::MultiHit ? Def->HitCount : 1);
	NextHitIndex = 0;
	NextHitTime = CastResolveTime;
	HealthAtCastStart = UGOTACombatantObserver::ReadVitals(Owner).Health;
	bCharging = false;
	ChargeHit.Reset();

	// Face the target; root the caster for the wind-up (the readable tell).
	if (Target)
	{
		FRotator Face = (Target->GetActorLocation() - Owner->GetActorLocation()).Rotation();
		Face.Pitch = 0.f;
		Owner->SetActorRotation(Face);
		if (AController* C = Owner->GetController())
		{
			C->SetControlRotation(Face);
		}
	}
	FGOTABlueprintCombat::SetBool(Owner, TEXT("CanMove"), false);
	FGOTABlueprintCombat::SetBool(Owner, TEXT("Attacking"), true); // visible wind-up: the AI may react to it
	if (AAIController* AIC = Cast<AAIController>(Owner->GetController()))
	{
		AIC->StopMovement();
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
		{
			Brain->PauseLogic(TEXT("GOTA ability"));
		}
	}
	if (ACharacter* Char = Cast<ACharacter>(Owner))
	{
		Char->GetCharacterMovement()->StopMovementImmediately();
	}
	UE_LOG(LogGOTAAbility, Log, TEXT("%s: %s (%s) cast %.2fs target=%s momentum-%.0f"), *GetNameSafe(Owner), *Def->AbilityId.ToString(),
		*StaticEnum<EGOTAAbilityKind>()->GetNameStringByValue(static_cast<int64>(Def->Kind)), Def->CastTime, *GetNameSafe(Target), Def->MomentumCost);
	Multicast_BeginCast(static_cast<uint8>(Slot));
}

// ============================================================================ server: timeline

void UGOTAHeroAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bInputBound)
	{
		TryBindLocalInput();
	}
	ApplyMovementStatusLocally();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		TickServer(DeltaTime);
	}
}

void UGOTAHeroAbilityComponent::TickServer(float DeltaTime)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	const double Now = ServerNow();
	if (!Owner)
	{
		return;
	}

	// Status expiry + Unbreakable refund (through the Blueprint's own Heal: health keeps one authority).
	if (ActiveStatus != EGOTAStatus::None && Now >= StatusEnd)
	{
		ApplyStatus(EGOTAStatus::None, 0.f, 0.f, nullptr);
	}
	const float Health = UGOTACombatantObserver::ReadVitals(Owner).Health;
	if (ActiveStatus == EGOTAStatus::Unbreakable && StatusTrackedHealth >= 0.f && Health < StatusTrackedHealth - 0.5f && Health > 0.f)
	{
		const float Refund = (StatusTrackedHealth - Health) * FMath::Clamp(StatusMagnitude, 0.f, 0.9f);
		if (UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Owner))
		{
			FGOTABlueprintCombat::Call(DS, TEXT("Heal"), nullptr, FString(), Refund);
			UE_LOG(LogGOTAAbility, Log, TEXT("%s: Unbreakable absorbed %.1f of %.1f"), *GetNameSafe(Owner), Refund, StatusTrackedHealth - Health);
		}
	}
	StatusTrackedHealth = ActiveStatus == EGOTAStatus::Unbreakable ? UGOTACombatantObserver::ReadVitals(Owner).Health : -1.f;

	if (!Owner->IsPlayerControlled())
	{
		if (APawn* Opp = GetOpponentPawn())
		{
			const float OppHealth = UGOTACombatantObserver::ReadVitals(Opp).Health;
			if (AITrackedOpponentHealth >= 0.f && OppHealth < AITrackedOpponentHealth - 0.5f && IsRoundLive())
			{
				if (UGOTADuelFighterComponent* Fighter = Owner->FindComponentByClass<UGOTADuelFighterComponent>())
				{
					Fighter->GrantMomentum(Fighter->HitLandedMomentum, TEXT("AI hit landed"));
				}
			}
			AITrackedOpponentHealth = OppHealth;
		}
	}

	if (ActiveSlot == INDEX_NONE)
	{
		TickAI(Now);
		return;
	}
	const FGOTAAbilityDef* Def = GetAbility(ActiveSlot);
	if (!Def || !IsRoundLive() || IsOwnerDead())
	{
		CancelCastServer(TEXT("round over"));
		return;
	}

	// Interrupt: hit during the wind-up (before the first resolve).
	if (Def->bInterruptible && NextHitIndex == 0 && Now < CastResolveTime && ActiveStatus != EGOTAStatus::Unbreakable
		&& Health < HealthAtCastStart - 0.5f)
	{
		CancelCastServer(TEXT("interrupted"));
		return;
	}

	if (bCharging)
	{
		ACharacter* Char = Cast<ACharacter>(Owner);
		APawn* Opp = GetOpponentPawn();
		if (Opp && !ChargeHit.Contains(Opp))
		{
			const float Reach = (Char ? Char->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f) + 90.f;
			if (FVector::Dist2D(Opp->GetActorLocation(), Owner->GetActorLocation()) <= Reach)
			{
				ChargeHit.Add(Opp);
				const bool bLanded = DealDamage(Opp, *Def, Def->Damage);
				if (bLanded)
				{
					Knockback(Opp, Def->Magnitude, 300.f);
				}
				Multicast_Impact(static_cast<uint8>(ActiveSlot), Opp->GetActorLocation(), Opp, 0);
				bCharging = false;
				if (Char)
				{
					Char->GetCharacterMovement()->StopMovementImmediately();
				}
			}
		}
		if (bCharging && Now >= CastResolveTime + Def->HitInterval)
		{
			bCharging = false; // ran out of charge (missed)
			// Stop at the end of the dash ("up to Range"); otherwise the launch velocity slides the caster on.
			if (ACharacter* Dasher = Cast<ACharacter>(Owner))
			{
				Dasher->GetCharacterMovement()->StopMovementImmediately();
			}
			Multicast_Impact(static_cast<uint8>(ActiveSlot), Owner->GetActorLocation(), nullptr, 0);
		}
		if (!bCharging && CastFinishTime < 0.0)
		{
			CastFinishTime = Now + Def->RecoveryTime;
		}
	}
	else if (PendingHits > 0 && Now >= NextHitTime)
	{
		ResolveServer(ActiveSlot, NextHitIndex);
		++NextHitIndex;
		--PendingHits;
		NextHitTime = Now + Def->HitInterval;
		if (PendingHits == 0 && !bCharging)
		{
			CastFinishTime = Now + Def->RecoveryTime;
		}
	}

	if (CastFinishTime >= 0.0 && Now >= CastFinishTime && !bCharging)
	{
		FinishCastServer(ActiveSlot);
	}
}

void UGOTAHeroAbilityComponent::ResolveServer(int32 Slot, int32 HitIndex)
{
	const FGOTAAbilityDef* Def = GetAbility(Slot);
	APawn* Owner = Cast<APawn>(GetOwner());
	ACharacter* Char = Cast<ACharacter>(Owner);
	if (!Def || !Owner)
	{
		return;
	}
	APawn* Target = CastTarget.Get();
	TArray<APawn*> Hit;
	FVector FxLoc = Owner->GetActorLocation() + Owner->GetActorForwardVector() * FMath::Min(Def->Range, 200.f);

	switch (Def->Kind)
	{
	case EGOTAAbilityKind::MeleeArc:
	case EGOTAAbilityKind::MultiHit:
	{
		// Keep facing the target between strikes (a fast combo tracks; a heavy cleave does not).
		if (Def->Kind == EGOTAAbilityKind::MultiHit && Target && GetOpponentPawn() == Target)
		{
			FRotator Face = (Target->GetActorLocation() - Owner->GetActorLocation()).Rotation();
			Face.Pitch = 0.f;
			Owner->SetActorRotation(Face);
		}
		DamageInArc(*Def, Def->Range, Def->ArcDegrees, 0.f, false, Hit);
		if (Hit.Num() > 0)
		{
			FxLoc = Hit[0]->GetActorLocation();
		}
		break;
	}
	case EGOTAAbilityKind::AreaBurst:
	{
		DamageInArc(*Def, Def->Radius, 360.f, Def->Radius, true, Hit);
		for (APawn* P : Hit)
		{
			if (Def->Magnitude > 0.f)
			{
				Knockback(P, Def->Magnitude, 250.f);
			}
		}
		FxLoc = Owner->GetActorLocation() - FVector(0.f, 0.f, Char ? Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f);
		break;
	}
	case EGOTAAbilityKind::Aura:
	{
		DamageInArc(*Def, Def->Radius, 360.f, Def->Radius, true, Hit);
		for (APawn* P : Hit)
		{
			if (UGOTAHeroAbilityComponent* Other = P->FindComponentByClass<UGOTAHeroAbilityComponent>())
			{
				Other->ApplyStatus(Def->Status, Def->StatusDuration, Def->Magnitude, Owner);
			}
			else
			{
				// Opponents without an ability component still get the movement debuff.
				UGOTAHeroAbilityComponent* Added = NewObject<UGOTAHeroAbilityComponent>(P, TEXT("GOTAHeroAbilities"));
				Added->RegisterComponent();
				Added->ApplyStatus(Def->Status, Def->StatusDuration, Def->Magnitude, Owner);
			}
		}
		FxLoc = Owner->GetActorLocation();
		break;
	}
	case EGOTAAbilityKind::SelfGuard:
	{
		ApplyStatus(Def->Status, Def->StatusDuration, Def->Magnitude, Owner);
		FxLoc = Owner->GetActorLocation();
		break;
	}
	case EGOTAAbilityKind::Teleport:
	{
		FVector Dest;
		FRotator Rot;
		APawn* LiveTarget = (Target && GetOpponentPawn() == Target) ? Target : nullptr;
		if (FindTeleportDestination(LiveTarget, Def->Range, Dest, Rot))
		{
			const FVector From = Owner->GetActorLocation();
			Owner->TeleportTo(Dest, Rot, false, false);
			if (AController* C = Owner->GetController())
			{
				C->SetControlRotation(Rot);
			}
			Multicast_Impact(static_cast<uint8>(Slot), From, nullptr, 0); // departure puff
			UE_LOG(LogGOTAAbility, Log, TEXT("%s: teleport %s -> %s"), *GetNameSafe(Owner), *From.ToCompactString(), *Dest.ToCompactString());
			if (Def->StatusDuration > 0.f && Def->Status != EGOTAStatus::None)
			{
				ApplyStatus(Def->Status, Def->StatusDuration, Def->Magnitude, Owner);
			}
			if (Def->Damage > 0.f)
			{
				DamageInArc(*Def, Def->Radius > 0.f ? Def->Radius : 220.f, 360.f, 0.f, true, Hit);
			}
		}
		else
		{
			UE_LOG(LogGOTAAbility, Log, TEXT("%s: teleport destination became invalid — no move"), *GetNameSafe(Owner));
		}
		FxLoc = Owner->GetActorLocation();
		break;
	}
	case EGOTAAbilityKind::Pull:
	{
		// Re-validate at resolve: the target may have moved behind cover or out of range during the cast.
		APawn* Opp = GetOpponentPawn();
		const UGOTAHeroAbilityComponent* OppAb = Opp ? Opp->FindComponentByClass<UGOTAHeroAbilityComponent>() : nullptr;
		if (Opp && Opp == Target && !(OppAb && OppAb->IsUntargetable())
			&& FVector::Dist(Opp->GetActorLocation(), Owner->GetActorLocation()) <= Def->Range * 1.15f && HasLineOfSight(Owner, Opp))
		{
			DealDamage(Opp, *Def, Def->Damage);
			if (ACharacter* Victim = Cast<ACharacter>(Opp))
			{
				FVector ToCaster = Owner->GetActorLocation() - Opp->GetActorLocation();
				ToCaster.Z = 0.f;
				const float Dist = ToCaster.Size();
				const FVector Dir = ToCaster.GetSafeNormal();
				// Speed scaled so the pull ends roughly in front of the caster, not inside it.
				const float Speed = FMath::Clamp((Dist - 230.f) * 1.9f, 0.f, Def->Magnitude);
				Victim->LaunchCharacter(Dir * Speed + FVector(0.f, 0.f, 220.f), true, true);
			}
			Hit.Add(Opp);
			FxLoc = Opp->GetActorLocation();
		}
		else
		{
			UE_LOG(LogGOTAAbility, Log, TEXT("%s: pull target lost (range/LOS/untargetable) — fizzled"), *GetNameSafe(Owner));
		}
		break;
	}
	case EGOTAAbilityKind::Charge:
	{
		FVector Dir = Owner->GetActorForwardVector();
		if (Target)
		{
			Dir = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
			Owner->SetActorRotation(Dir.Rotation());
		}
		ChargeDir = Dir;
		ChargeStart = Owner->GetActorLocation();
		bCharging = true;
		if (Char)
		{
			const float Speed = Def->Range / FMath::Max(Def->HitInterval, 0.1f);
			Char->LaunchCharacter(Dir * Speed, true, false);
		}
		// Charge impact is resolved per-frame in TickServer.
		Multicast_Impact(static_cast<uint8>(Slot), Owner->GetActorLocation(), nullptr, 255); // launch cue (255 = no impact kick)
		return;
	}
	}
	Multicast_Impact(static_cast<uint8>(Slot), FxLoc, Hit.Num() > 0 ? Hit[0] : nullptr, static_cast<uint8>(HitIndex));
}

void UGOTAHeroAbilityComponent::FinishCastServer(int32 Slot)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	if (Owner)
	{
		FGOTABlueprintCombat::SetBool(Owner, TEXT("CanMove"), true);
		FGOTABlueprintCombat::SetBool(Owner, TEXT("Attacking"), false);
		if (AAIController* AIC = Cast<AAIController>(Owner->GetController()))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->ResumeLogic(TEXT("GOTA ability"));
			}
		}
	}
	if (ActiveSlot != INDEX_NONE)
	{
		Multicast_CastCancelled(static_cast<uint8>(ActiveSlot)); // blend the body animation out with control
	}
	ActiveSlot = INDEX_NONE;
	CastResolveTime = CastFinishTime = -1.0;
	PendingHits = 0;
	bCharging = false;
	CastTarget.Reset();
}

void UGOTAHeroAbilityComponent::CancelCastServer(const TCHAR* Why)
{
	if (ActiveSlot == INDEX_NONE)
	{
		return;
	}
	const int32 Slot = ActiveSlot;
	UE_LOG(LogGOTAAbility, Log, TEXT("%s: %s cancelled (%s)"), *GetNameSafe(GetOwner()), GetAbility(Slot) ? *GetAbility(Slot)->AbilityId.ToString() : TEXT("?"), Why);
	FinishCastServer(Slot);
}

void UGOTAHeroAbilityComponent::ResetAbilities(bool bClearCooldowns)
{
	CancelCastServer(TEXT("reset"));
	ApplyStatus(EGOTAStatus::None, 0.f, 0.f, nullptr);
	if (bClearCooldowns)
	{
		for (float& C : CooldownEnd)
		{
			C = 0.f;
		}
	}
}

// ============================================================================ server: effects

int32 UGOTAHeroAbilityComponent::DamageInArc(const FGOTAAbilityDef& Def, float Range, float ArcDegrees, float Radius, bool bCenteredOnCaster, TArray<APawn*>& OutHit)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	if (!Owner)
	{
		return 0;
	}
	const FVector Origin = Owner->GetActorLocation();
	const FVector Fwd = Owner->GetActorForwardVector().GetSafeNormal2D();
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(ArcDegrees, 1.f, 360.f) * 0.5f));
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* P = *It;
		if (P == Owner || !UGOTACombatantObserver::FindDamageSystem(P) || UGOTACombatantObserver::IsCombatantDefeated(P))
		{
			continue;
		}
		const UGOTAHeroAbilityComponent* Other = P->FindComponentByClass<UGOTAHeroAbilityComponent>();
		if (Other && Other->IsUntargetable())
		{
			continue;
		}
		FVector To = P->GetActorLocation() - Origin;
		const float VerticalGap = FMath::Abs(To.Z);
		To.Z = 0.f;
		const ACharacter* PC = Cast<ACharacter>(P);
		const float TargetRadius = PC ? PC->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
		if (To.Size() > Range + TargetRadius || VerticalGap > 250.f)
		{
			continue;
		}
		if (ArcDegrees < 359.f && To.Size() > TargetRadius && FVector::DotProduct(To.GetSafeNormal(), Fwd) < CosHalf)
		{
			continue;
		}
		if (!HasLineOfSight(Owner, P))
		{
			continue;
		}
		if (Def.Damage > 0.f)
		{
			DealDamage(P, Def, Def.Damage); // blocked hits still count for FX placement
		}
		OutHit.Add(P);
	}
	return OutHit.Num();
}

bool UGOTAHeroAbilityComponent::DealDamage(APawn* Victim, const FGOTAAbilityDef& Def, float Amount)
{
	if (!Victim || Amount <= 0.f)
	{
		return false;
	}
	const float Scaled = Amount * GetOutgoingDamageScale();
	// The victim's own BPC_DamageSystem applies (and may block) the hit: the single health authority.
	const bool bLanded = FGOTABlueprintCombat::ApplyDamage(Victim, GetOwner(), Scaled, Def.bCanBeBlocked, false, Def.HitResponse);
	UE_LOG(LogGOTAAbility, Log, TEXT("%s: %s hit %s for %.1f (landed=%d)"), *GetNameSafe(GetOwner()), *Def.AbilityId.ToString(), *GetNameSafe(Victim), Scaled, bLanded ? 1 : 0);
	if (bLanded)
	{
		if (UGOTADuelFighterComponent* Fighter = GetOwner()->FindComponentByClass<UGOTADuelFighterComponent>())
		{
			Fighter->GrantMomentum(Def.bUltimate ? 0.f : 2.f, TEXT("ability hit"));
		}
	}
	return bLanded;
}

void UGOTAHeroAbilityComponent::Knockback(APawn* Victim, float Speed, float Up)
{
	ACharacter* Char = Cast<ACharacter>(Victim);
	if (!Char || Speed <= 0.f)
	{
		return;
	}
	FVector Dir = Victim->GetActorLocation() - GetOwner()->GetActorLocation();
	Dir.Z = 0.f;
	Char->LaunchCharacter(Dir.GetSafeNormal() * Speed + FVector(0.f, 0.f, Up), true, true);
}

bool UGOTAHeroAbilityComponent::HasLineOfSight(const AActor* A, const AActor* B) const
{
	if (!A || !B || !GetWorld())
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GOTAAbilityLOS), false, A);
	Params.AddIgnoredActor(B);
	TArray<AActor*> Attached;
	A->GetAttachedActors(Attached);
	B->GetAttachedActors(Attached, false);
	Params.AddIgnoredActors(Attached);
	const FVector From = A->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector To = B->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	// Walls and arena geometry only: fighters, weapons and bodies never count as cover.
	return !GetWorld()->LineTraceTestByChannel(From, To, ECC_WorldStatic, Params);
}

static void TeleportDebug(const TCHAR* Why, const FVector& C)
{
	if (CVarGOTAAbilityAnimDebug.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogGOTAAbility, Log, TEXT("teleport candidate %s rejected: %s"), *C.ToCompactString(), Why);
	}
}

bool UGOTAHeroAbilityComponent::FindTeleportDestination(APawn* Target, float Range, FVector& OutLocation, FRotator& OutRotation) const
{
	APawn* Owner = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}
	const FVector From = Owner->GetActorLocation();
	TArray<FVector> Candidates;
	if (Target)
	{
		const FVector T = Target->GetActorLocation();
		FVector Away = (T - From).GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = Target->GetActorForwardVector();
		}
		const FVector Side = FVector::CrossProduct(Away, FVector::UpVector);
		Candidates.Add(T + Away * 150.f);                 // behind the target
		Candidates.Add(T + (Away + Side).GetSafeNormal() * 150.f);
		Candidates.Add(T + (Away - Side).GetSafeNormal() * 150.f);
		Candidates.Add(T + Side * 160.f);
		Candidates.Add(T - Side * 160.f);
	}
	else
	{
		const FVector Fwd = Owner->GetActorForwardVector().GetSafeNormal2D();
		Candidates.Add(From + Fwd * Range * 0.7f);
		Candidates.Add(From + Fwd * Range * 0.45f);
	}
	const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	for (FVector C : Candidates)
	{
		if (FVector::Dist2D(C, From) > Range + 200.f)
		{
			{ TeleportDebug(TEXT("too far"), C); continue; }
		}
		// Must be on walkable navmesh (stays inside the arena), reachable by sight, and collision-free.
		if (Nav)
		{
			FNavLocation Projected;
			if (!Nav->ProjectPointToNavigation(C, Projected, FVector(120.f, 120.f, 300.f)))
			{
				{ TeleportDebug(TEXT("off navmesh"), C); continue; }
			}
			const ACharacter* OwnerChar = Cast<ACharacter>(Owner);
			C = Projected.Location + FVector(0.f, 0.f, OwnerChar ? OwnerChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f : 92.f);
		}
		if (FMath::Abs(C.Z - From.Z) > 220.f)
		{
			{ TeleportDebug(TEXT("height"), C); continue; }
		}
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GOTATeleportLOS), false, Owner);
		if (Target)
		{
			Params.AddIgnoredActor(Target);
		}
		if (World->LineTraceTestByChannel(From + FVector(0, 0, 40), C + FVector(0, 0, 40), ECC_WorldStatic, Params))
		{
			{ TeleportDebug(TEXT("wall in the way"), C); continue; } // no teleporting through walls
		}
		FRotator Rot = Target ? (Target->GetActorLocation() - C).Rotation() : Owner->GetActorRotation();
		Rot.Pitch = 0.f;
		FVector Spot = C;
		if (!World->FindTeleportSpot(Owner, Spot, Rot) || FVector::Dist2D(Spot, C) >= 120.f)
		{
			TeleportDebug(TEXT("no collision-free spot"), C);
		}
		else
		{
			OutLocation = Spot;
			OutRotation = Rot;
			return true;
		}
	}
	return false;
}

void UGOTAHeroAbilityComponent::ApplyStatus(EGOTAStatus Status, float Duration, float Magnitude, AActor* Source)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Owner);
	// Undo the previous status' Blueprint flags.
	if (ActiveStatus == EGOTAStatus::Veiled && DS)
	{
		FGOTABlueprintCombat::SetBool(DS, TEXT("IsInvincible"), false);
	}
	if (ActiveStatus == EGOTAStatus::Unbreakable && DS)
	{
		FGOTABlueprintCombat::SetBool(DS, TEXT("IsInterruptible"), true);
	}
	ActiveStatus = Duration > 0.f ? Status : EGOTAStatus::None;
	StatusEnd = ActiveStatus == EGOTAStatus::None ? 0.f : static_cast<float>(ServerNow() + Duration);
	StatusMagnitude = ActiveStatus == EGOTAStatus::None ? 0.f : Magnitude;
	StatusTrackedHealth = -1.f;
	if (ActiveStatus == EGOTAStatus::Veiled && DS)
	{
		FGOTABlueprintCombat::SetBool(DS, TEXT("IsInvincible"), true);
	}
	if (ActiveStatus == EGOTAStatus::Unbreakable && DS)
	{
		FGOTABlueprintCombat::SetBool(DS, TEXT("IsInterruptible"), false); // hyper armour
		StatusTrackedHealth = UGOTACombatantObserver::ReadVitals(Owner).Health;
	}
	if (Status != EGOTAStatus::None || Duration > 0.f)
	{
		UE_LOG(LogGOTAAbility, Log, TEXT("%s: status %s %.1fs x%.2f (from %s)"), *GetNameSafe(Owner),
			*StaticEnum<EGOTAStatus>()->GetNameStringByValue(static_cast<int64>(ActiveStatus)), Duration, Magnitude, *GetNameSafe(Source));
	}
	OnRep_Status();
}

void UGOTAHeroAbilityComponent::OnRep_Status()
{
	// Visible status: an overlay material on the body (or pawn mesh). Purely cosmetic.
	const ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		return;
	}
	USkeletalMeshComponent* Mesh = nullptr;
	if (const UGOTAHeroBodyComponent* BodyComp = Char->FindComponentByClass<UGOTAHeroBodyComponent>())
	{
		Mesh = BodyComp->GetBodyMesh();
	}
	if (!Mesh)
	{
		Mesh = Char->GetMesh();
	}
	const TCHAR* Path = nullptr;
	switch (ActiveStatus)
	{
	case EGOTAStatus::Unbreakable: Path = GOTAAbilityAssets::OverlayUnbreakable; break;
	case EGOTAStatus::Veiled: Path = GOTAAbilityAssets::OverlayVeiled; break;
	case EGOTAStatus::Dread: Path = GOTAAbilityAssets::OverlayDread; break;
	default: break;
	}
	if (Mesh)
	{
		Mesh->SetOverlayMaterial(Path ? LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn) : nullptr);
	}
}

void UGOTAHeroAbilityComponent::ApplyMovementStatusLocally()
{
	// Dread slows movement. Applied on every peer (server + owning client) so prediction agrees.
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* Move = Char ? Char->GetCharacterMovement() : nullptr;
	if (!Move)
	{
		return;
	}
	const bool bDread = ActiveStatus == EGOTAStatus::Dread && GetStatusRemaining() > 0.f;
	if (bDread)
	{
		if (BaseWalkSpeed < 0.f)
		{
			BaseWalkSpeed = Move->MaxWalkSpeed;
		}
		Move->MaxWalkSpeed = BaseWalkSpeed * FMath::Clamp(1.f - StatusMagnitude, 0.2f, 1.f);
	}
	else if (BaseWalkSpeed >= 0.f)
	{
		Move->MaxWalkSpeed = BaseWalkSpeed;
		BaseWalkSpeed = -1.f;
	}
	// Status ran out locally before the server's replication caught up: drop the overlay now.
	if (ActiveStatus != EGOTAStatus::None && GetStatusRemaining() <= 0.f && !GetOwner()->HasAuthority() && !bStatusVisualExpired)
	{
		bStatusVisualExpired = true;
		const EGOTAStatus Saved = ActiveStatus;
		ActiveStatus = EGOTAStatus::None;
		OnRep_Status();
		ActiveStatus = Saved;
	}
	else if (GetStatusRemaining() > 0.f)
	{
		bStatusVisualExpired = false;
	}
}

// ============================================================================ AI

void UGOTAHeroAbilityComponent::TickAI(double Now)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	if (!Owner || Owner->IsPlayerControlled() || !Cast<AAIController>(Owner->GetController()) || !bAIEnabled || !AbilitySet)
	{
		return;
	}
	if (Now < NextAIDecisionTime || !IsRoundLive() || IsOwnerDead())
	{
		return;
	}
	NextAIDecisionTime = Now + 0.4;
	if (ForcedAISlot != INDEX_NONE)
	{
		const int32 Slot = ForcedAISlot;
		ForcedAISlot = INDEX_NONE;
		LastAIUseTime = Now;
		ActivateServer(Slot);
		return;
	}
	if (Now - LastAIUseTime < AbilitySet->AIGlobalGap)
	{
		return; // anti-spam: same rules, but the AI paces itself
	}
	APawn* Opp = GetOpponentPawn();
	if (!Opp || FGOTABlueprintCombat::GetBool(Owner, TEXT("Attacking")))
	{
		return; // don't interrupt the Blueprint's own attack
	}
	const float Dist = FVector::Dist(Opp->GetActorLocation(), Owner->GetActorLocation());
	const bool bSight = HasLineOfSight(Owner, Opp);
	const FGOTACombatantVitals Own = UGOTACombatantObserver::ReadVitals(Owner);
	const float OwnFrac = Own.GetFraction();
	const bool bOppAttacking = FGOTABlueprintCombat::GetBool(Opp, TEXT("Attacking"));

	int32 Best = INDEX_NONE;
	float BestScore = 0.f;
	for (int32 i = 0; i < GetNumAbilities(); ++i)
	{
		const FGOTAAbilityDef& Def = AbilitySet->Abilities[i];
		if (Def.AIPriority <= 0.f || !IsSlotReady(i))
		{
			continue;
		}
		if (Def.MomentumCost > 0.f)
		{
			const UStatsComponent* Stats = GetStats();
			if (!Stats || Stats->GetDivineMomentum() < Def.MomentumCost)
			{
				continue;
			}
		}
		if (Dist < Def.AIMinDistance || Dist > Def.AIMaxDistance || OwnFrac > Def.AIMaxOwnHealthFraction)
		{
			continue;
		}
		if (Def.Kind != EGOTAAbilityKind::SelfGuard && !bSight)
		{
			continue;
		}
		float Score = Def.AIPriority * FMath::FRandRange(0.75f, 1.25f);
		if (Def.Kind == EGOTAAbilityKind::SelfGuard && bOppAttacking)
		{
			Score *= 1.8f; // react to a visible wind-up
		}
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = i;
		}
	}
	if (Best != INDEX_NONE && FMath::FRand() < 0.6f)
	{
		LastAIUseTime = Now;
		UE_LOG(LogGOTAAbility, Log, TEXT("AI %s chooses %s (dist %.0f, hp %.0f%%)"), *GetNameSafe(Owner), *AbilitySet->Abilities[Best].AbilityId.ToString(), Dist, OwnFrac * 100.f);
		ActivateServer(Best);
	}
}

void UGOTAHeroAbilityComponent::DebugAIUse(int32 Slot)
{
	ForcedAISlot = Slot;
	NextAIDecisionTime = 0.0;
}

// ============================================================================ presentation (all peers)

void UGOTAHeroAbilityComponent::Multicast_BeginCast_Implementation(uint8 Slot)
{
	LastCastSlot = Slot;
	if (ADuelGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr)
	{
		GS->OnDuelCombatCue.Broadcast(EGOTADuelCue::AbilityUsed, GetOwner(), nullptr);
	}
	PlayCastPresentation(Slot);
}

void UGOTAHeroAbilityComponent::Multicast_Impact_Implementation(uint8 Slot, FVector_NetQuantize Location, AActor* Target, uint8 HitIndex)
{
	PlayImpactPresentation(Slot, Location, HitIndex == 255 ? nullptr : Target);
}

void UGOTAHeroAbilityComponent::Multicast_CastCancelled_Implementation(uint8 Slot)
{
	const ACharacter* Char = Cast<ACharacter>(GetOwner());
	const UGOTAHeroBodyComponent* BodyComp = Char ? Char->FindComponentByClass<UGOTAHeroBodyComponent>() : nullptr;
	USkeletalMeshComponent* Body = BodyComp ? BodyComp->GetBodyMesh() : nullptr;
	if (UAnimInstance* Anim = Body ? Body->GetAnimInstance() : nullptr)
	{
		Anim->StopSlotAnimation(0.15f, GOTAAbilityAssets::BodySlot);
	}
}

static void SpawnTimedFX(UWorld* World, UParticleSystem* PS, const FVector& Loc, const FRotator& Rot, float Scale, float Lifetime, USceneComponent* AttachTo = nullptr)
{
	if (!World || !PS || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	UParticleSystemComponent* PSC = AttachTo
		? UGameplayStatics::SpawnEmitterAttached(PS, AttachTo, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(Scale), EAttachLocation::SnapToTarget, true)
		: UGameplayStatics::SpawnEmitterAtLocation(World, PS, Loc, Rot, FVector(Scale), true);
	if (PSC && Lifetime > 0.f)
	{
		FTimerHandle Handle;
		TWeakObjectPtr<UParticleSystemComponent> Weak = PSC;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(PSC, [Weak]()
		{
			if (Weak.IsValid())
			{
				Weak->DestroyComponent();
			}
		}), Lifetime, false);
	}
}

void UGOTAHeroAbilityComponent::PlayCastPresentation(int32 Slot)
{
	const FGOTAAbilityDef* Def = GetAbility(Slot);
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Def || !Char || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// Hero-specific animation on the visible body (montage slot on top of the retargeted pose).
	const UGOTAHeroBodyComponent* BodyComp = Char->FindComponentByClass<UGOTAHeroBodyComponent>();
	USkeletalMeshComponent* Body = BodyComp ? BodyComp->GetBodyMesh() : nullptr;
	UAnimSequenceBase* Seq = Def->Animation.Get();
	if (!Seq && !Def->Animation.IsNull())
	{
		Seq = Def->Animation.LoadSynchronous();
	}
	if (Body && Seq)
	{
		if (UAnimInstance* Anim = Body->GetAnimInstance())
		{
			UAnimMontage* M = Anim->PlaySlotAnimationAsDynamicMontage(Seq, GOTAAbilityAssets::BodySlot, 0.08f, 0.2f, FMath::Max(Def->AnimPlayRate, 0.1f), 1, -1.f, Def->AnimStartTime);
			UE_LOG(LogGOTAAbility, Verbose, TEXT("%s: body plays %s (%s, %.2fs)"), *GetNameSafe(Char), *GetNameSafe(Seq), M ? TEXT("ok") : TEXT("FAILED"), Seq->GetPlayLength());
			if (!M)
			{
				UE_LOG(LogGOTAAbility, Warning, TEXT("%s: could not play %s on the body"), *GetNameSafe(Char), *GetNameSafe(Seq));
			}
			else if (CVarGOTAAbilityAnimDebug.GetValueOnGameThread() != 0)
			{
				const double Start = GetWorld()->GetTimeSeconds();
				FOnMontageEnded Ended = FOnMontageEnded::CreateWeakLambda(this, [this, Start, Seq](UAnimMontage*, bool bInterrupted)
				{
					UE_LOG(LogGOTAAbility, Log, TEXT("body montage %s ended after %.2fs (interrupted=%d)"), *GetNameSafe(Seq), GetWorld()->GetTimeSeconds() - Start, bInterrupted ? 1 : 0);
				});
				Anim->Montage_SetEndDelegate(Ended, M);
			}
		}
	}
	SpawnTimedFX(GetWorld(), Def->CastFX.Get(), Char->GetActorLocation(), Char->GetActorRotation(), Def->FXScale, Def->FXLifetime, Char->GetRootComponent());
	if (USoundBase* S = Def->CastSound.Get())
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, Char->GetActorLocation());
	}
}

void UGOTAHeroAbilityComponent::PlayImpactPresentation(int32 Slot, const FVector& Location, AActor* Target)
{
	const FGOTAAbilityDef* Def = GetAbility(Slot);
	UWorld* World = GetWorld();
	if (!Def || !World || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	SpawnTimedFX(World, Def->ImpactFX.Get(), Location, GetOwner()->GetActorRotation(), Def->FXScale, Def->FXLifetime);
	if (USoundBase* S = Def->ImpactSound.Get())
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, Location);
	}
	if (Def->ImpactKick > 0.f && (Target || Def->Kind == EGOTAAbilityKind::AreaBurst || Def->Kind == EGOTAAbilityKind::Aura))
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (PC && PC->IsLocalController() && (PC->GetPawn() == GetOwner() || PC->GetPawn() == Target
				|| (PC->GetPawn() && FVector::Dist(PC->GetPawn()->GetActorLocation(), Location) < 1200.f)))
			{
				if (AGOTAPlayerCameraManager* Cam = Cast<AGOTAPlayerCameraManager>(PC->PlayerCameraManager))
				{
					Cam->AddImpactKick(Def->ImpactKick);
				}
			}
		}
	}
}
