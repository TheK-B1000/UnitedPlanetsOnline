// White Tiger Studios 2024

#include "Arenas/Duel/GOTADuelCombat.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Arenas/Duel/DuelGameMode.h"
#include "Arenas/Duel/DuelGameState.h"
#include "BrainComponent.h"
#include "Characters/StatsComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/GOTAPlayerCameraManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTADuelAI, Log, All);

static TAutoConsoleVariable<int32> CVarDuelDebugOverlay(
	TEXT("gota.Duel.DebugOverlay"), 0,
	TEXT("1 = on-screen Sovereign Champion / momentum debug overlay."));

static TAutoConsoleVariable<int32> CVarDuelHitStop(
	TEXT("gota.Duel.HitStop"), 1,
	TEXT("1 = actor-local hit-stop on impact cues (mesh animation only, both fighters equally)."));

// ============================================================================ profiles

FGOTAChampionProfile FGOTAChampionProfile::ForDifficulty(EGOTADuelDifficulty Difficulty)
{
	FGOTAChampionProfile P;
	P.DisplayName = GetDifficultyName(Difficulty);
	switch (Difficulty)
	{
	case EGOTADuelDifficulty::Recruit:
		P.ChallengerMaxHealth = 160.f; P.ChampionMaxHealth = 190.f; P.GuardChainChance = 0.45f;
		P.ReactionSeconds = 0.55f; P.DefendChance = 0.2f; P.SidestepShare = 0.3f; P.ReactiveBlockChance = 0.15f;
		P.GuardOverloadHits = 2; P.GuardOverloadWindowSeconds = 6.f; P.GuardBreakSeconds = 3.f;
		P.PhaseTwoDefendBonus = 0.1f; P.PhaseTwoReactionScale = 0.9f;
		P.SurgeIntervalSeconds = 18.f; P.SurgeTelegraphSeconds = 1.4f;
		break;
	case EGOTADuelDifficulty::Ascendant:
		P.ChallengerMaxHealth = 110.f; P.ChampionMaxHealth = 340.f; P.GuardChainChance = 0.8f;
		P.ReactionSeconds = 0.24f; P.DefendChance = 0.55f; P.SidestepShare = 0.4f; P.ReactiveBlockChance = 0.35f;
		P.GuardOverloadHits = 3; P.GuardOverloadWindowSeconds = 4.5f; P.GuardBreakSeconds = 2.f;
		P.PhaseTwoDefendBonus = 0.15f; P.PhaseTwoReactionScale = 0.85f;
		P.SurgeIntervalSeconds = 11.f; P.SurgeTelegraphSeconds = 0.9f;
		break;
	case EGOTADuelDifficulty::Veteran:
	default:
		break; // struct defaults are the Veteran profile
	}
	return P;
}

EGOTADuelDifficulty FGOTAChampionProfile::ParseDifficulty(const FString& Value, EGOTADuelDifficulty Fallback)
{
	if (Value.Equals(TEXT("Recruit"), ESearchCase::IgnoreCase)) { return EGOTADuelDifficulty::Recruit; }
	if (Value.Equals(TEXT("Veteran"), ESearchCase::IgnoreCase)) { return EGOTADuelDifficulty::Veteran; }
	if (Value.Equals(TEXT("Ascendant"), ESearchCase::IgnoreCase)) { return EGOTADuelDifficulty::Ascendant; }
	return Fallback;
}

FText FGOTAChampionProfile::GetDifficultyName(EGOTADuelDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case EGOTADuelDifficulty::Recruit: return NSLOCTEXT("GOTA", "DiffRecruit", "Recruit");
	case EGOTADuelDifficulty::Ascendant: return NSLOCTEXT("GOTA", "DiffAscendant", "Ascendant");
	default: return NSLOCTEXT("GOTA", "DiffVeteran", "Veteran");
	}
}

// ============================================================================ reflection helpers

bool FGOTABlueprintCombat::GetBool(const UObject* Obj, FName Name, bool bDefault)
{
	const FBoolProperty* P = Obj ? FindFProperty<FBoolProperty>(Obj->GetClass(), Name) : nullptr;
	return P ? P->GetPropertyValue_InContainer(Obj) : bDefault;
}

bool FGOTABlueprintCombat::SetBool(UObject* Obj, FName Name, bool bValue)
{
	const FBoolProperty* P = Obj ? FindFProperty<FBoolProperty>(Obj->GetClass(), Name) : nullptr;
	if (!P)
	{
		return false;
	}
	P->SetPropertyValue_InContainer(Obj, bValue);
	return true;
}

bool FGOTABlueprintCombat::SetNumber(UObject* Obj, FName Name, double Value)
{
	FProperty* P = Obj ? FindFProperty<FProperty>(Obj->GetClass(), Name) : nullptr;
	if (FDoubleProperty* D = CastField<FDoubleProperty>(P)) { D->SetPropertyValue_InContainer(Obj, Value); return true; }
	if (FFloatProperty* F = CastField<FFloatProperty>(P)) { F->SetPropertyValue_InContainer(Obj, static_cast<float>(Value)); return true; }
	if (FIntProperty* I = CastField<FIntProperty>(P)) { I->SetPropertyValue_InContainer(Obj, FMath::RoundToInt(Value)); return true; }
	return false;
}

int64 FGOTABlueprintCombat::FindEnumValue(const UEnum* Enum, const FString& DisplayName, int64 Fallback)
{
	if (!Enum)
	{
		return Fallback;
	}
	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
	{
		if (Enum->GetDisplayNameTextByIndex(i).ToString().Equals(DisplayName, ESearchCase::IgnoreCase)
			|| Enum->GetAuthoredNameStringByIndex(i).Equals(DisplayName, ESearchCase::IgnoreCase))
		{
			return Enum->GetValueByIndex(i);
		}
	}
	return Fallback;
}

/** Writes an enum entry (by display name) into a byte/enum property. */
static void SetEnumProp(FProperty* Prop, void* Container, const FString& Entry)
{
	if (FByteProperty* B = CastField<FByteProperty>(Prop))
	{
		B->SetPropertyValue_InContainer(Container, static_cast<uint8>(FGOTABlueprintCombat::FindEnumValue(B->Enum, Entry, 0)));
	}
	else if (FEnumProperty* E = CastField<FEnumProperty>(Prop))
	{
		E->GetUnderlyingProperty()->SetIntPropertyValue(E->ContainerPtrToValuePtr<void>(Container),
			FGOTABlueprintCombat::FindEnumValue(E->GetEnum(), Entry, 0));
	}
}

bool FGOTABlueprintCombat::Call(UObject* Obj, FName Function, UObject* Target, const FString& EnumEntry, double Number)
{
	UFunction* Fn = Obj ? Obj->FindFunction(Function) : nullptr;
	if (!Fn)
	{
		UE_LOG(LogGOTADuelAI, Verbose, TEXT("Call: %s has no %s"), *GetNameSafe(Obj), *Function.ToString());
		return false;
	}
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(Fn->ParmsSize, 1)));
	FMemory::Memzero(Params, Fn->ParmsSize);
	Fn->InitializeStruct(Params);
	for (TFieldIterator<FProperty> P(Fn); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
	{
		if (P->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm) && !P->HasAnyPropertyFlags(CPF_ReferenceParm))
		{
			continue;
		}
		if (FObjectPropertyBase* O = CastField<FObjectPropertyBase>(*P))
		{
			if (Target && Target->IsA(O->PropertyClass))
			{
				O->SetObjectPropertyValue_InContainer(Params, Target);
			}
		}
		else if (!EnumEntry.IsEmpty() && (P->IsA<FByteProperty>() || P->IsA<FEnumProperty>()))
		{
			SetEnumProp(*P, Params, EnumEntry);
		}
		else if (FDoubleProperty* D = CastField<FDoubleProperty>(*P))
		{
			D->SetPropertyValue_InContainer(Params, Number);
		}
		else if (FFloatProperty* F = CastField<FFloatProperty>(*P))
		{
			F->SetPropertyValue_InContainer(Params, static_cast<float>(Number));
		}
	}
	Obj->ProcessEvent(Fn, Params);
	Fn->DestroyStruct(Params);
	return true;
}

bool FGOTABlueprintCombat::ApplyDamage(AActor* Victim, AActor* Causer, float Amount, bool bCanBeBlocked, bool bCanBeParried, const FString& Response)
{
	UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Victim);
	UFunction* Fn = nullptr;
	for (TFieldIterator<UFunction> It(DS ? DS->GetClass() : nullptr, EFieldIteratorFlags::ExcludeSuper); DS && It; ++It)
	{
		if (It->GetName().Equals(TEXT("TakeDamage")))
		{
			Fn = *It;
			break;
		}
	}
	if (!Fn)
	{
		UE_LOG(LogGOTADuelAI, Warning, TEXT("ApplyDamage: no BPC_DamageSystem::TakeDamage on %s"), *GetNameSafe(Victim));
		return false;
	}
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Fn->ParmsSize));
	FMemory::Memzero(Params, Fn->ParmsSize);
	Fn->InitializeStruct(Params);
	FBoolProperty* WasDamagedProp = nullptr;
	for (TFieldIterator<FProperty> P(Fn); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
	{
		if (FStructProperty* SP = CastField<FStructProperty>(*P))
		{
			void* Info = SP->ContainerPtrToValuePtr<void>(Params);
			for (TFieldIterator<FProperty> In(SP->Struct); In; ++In)
			{
				const FString N = In->GetName();
				if (N.StartsWith(TEXT("Amount")))
				{
					if (FDoubleProperty* D = CastField<FDoubleProperty>(*In)) { D->SetPropertyValue_InContainer(Info, Amount); }
					if (FFloatProperty* F = CastField<FFloatProperty>(*In)) { F->SetPropertyValue_InContainer(Info, Amount); }
				}
				else if (FBoolProperty* B = CastField<FBoolProperty>(*In))
				{
					if (N.StartsWith(TEXT("CanBeBlocked"))) { B->SetPropertyValue_InContainer(Info, bCanBeBlocked); }
					else if (N.StartsWith(TEXT("CanBeParried"))) { B->SetPropertyValue_InContainer(Info, bCanBeParried); }
					else if (N.StartsWith(TEXT("ShouldForceInterrupt"))) { B->SetPropertyValue_InContainer(Info, true); }
					else { B->SetPropertyValue_InContainer(Info, false); }
				}
				else if (N.StartsWith(TEXT("DamageResponse")))
				{
					SetEnumProp(*In, Info, Response);
				}
				else if (N.StartsWith(TEXT("DamageType")))
				{
					SetEnumProp(*In, Info, TEXT("Melee"));
				}
			}
		}
		else if (FObjectPropertyBase* O = CastField<FObjectPropertyBase>(*P))
		{
			O->SetObjectPropertyValue_InContainer(Params, Causer);
		}
		else if (FBoolProperty* B = CastField<FBoolProperty>(*P))
		{
			WasDamagedProp = B;
		}
	}
	DS->ProcessEvent(Fn, Params);
	const bool bDamaged = WasDamagedProp ? WasDamagedProp->GetPropertyValue_InContainer(Params) : true;
	Fn->DestroyStruct(Params);
	return bDamaged;
}

bool FGOTABlueprintCombat::ConfigureHealth(AActor* Fighter, float MaxHealth)
{
	UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Fighter);
	if (!DS || MaxHealth <= 0.f)
	{
		return false;
	}
	const bool bMax = SetNumber(DS, TEXT("MaxHealth"), MaxHealth);
	const bool bCur = SetNumber(DS, TEXT("Health"), MaxHealth);
	return bMax && bCur;
}

UAnimMontage* FGOTABlueprintCombat::GetActiveMontage(const AActor* Actor, float* OutPosition)
{
	const ACharacter* Char = Cast<ACharacter>(Actor);
	UAnimInstance* Anim = (Char && Char->GetMesh()) ? Char->GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* M = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
	if (M && OutPosition)
	{
		*OutPosition = Anim->Montage_GetPosition(M);
	}
	return M;
}

float FGOTABlueprintCombat::FindNextImpactTime(const UAnimMontage* Montage, float Position)
{
	if (!Montage)
	{
		return -1.f;
	}
	float Best = -1.f;
	for (const FAnimNotifyEvent& N : Montage->Notifies)
	{
		// UAnimNotify_PlayMontageNotify (AnimGraphRuntime): matched by class name to avoid the module dependency.
		if (!N.Notify || !N.Notify->GetClass()->GetName().Equals(TEXT("AnimNotify_PlayMontageNotify")))
		{
			continue;
		}
		const float T = N.GetTriggerTime();
		if (T >= Position - 0.05f && (Best < 0.f || T < Best))
		{
			Best = T;
		}
	}
	return Best;
}

/** True when two functions have identical parameter layouts (safe to bind across C++/BP). */
static bool LayoutsMatch(const UFunction* A, const UFunction* B)
{
	if (!A || !B || A->ParmsSize != B->ParmsSize || A->NumParms != B->NumParms)
	{
		return false;
	}
	TFieldIterator<FProperty> IA(A);
	TFieldIterator<FProperty> IB(B);
	for (; IA && IB; ++IA, ++IB)
	{
		if (IA->GetOffset_ForUFunction() != IB->GetOffset_ForUFunction() || IA->GetSize() != IB->GetSize())
		{
			return false;
		}
	}
	return true;
}

bool FGOTABlueprintCombat::BindDelegate(UObject* Source, FName DelegateName, UObject* Listener, FName ListenerFunction)
{
	FMulticastDelegateProperty* P = Source ? FindFProperty<FMulticastDelegateProperty>(Source->GetClass(), DelegateName) : nullptr;
	UFunction* Mine = Listener ? Listener->FindFunction(ListenerFunction) : nullptr;
	if (!P || !Mine || !LayoutsMatch(P->SignatureFunction, Mine))
	{
		UE_LOG(LogGOTADuelAI, Warning, TEXT("BindDelegate: cannot bind %s.%s -> %s (missing or layout mismatch)"),
			*GetNameSafe(Source), *DelegateName.ToString(), *ListenerFunction.ToString());
		return false;
	}
	FScriptDelegate D;
	D.BindUFunction(Listener, ListenerFunction);
	P->AddDelegate(MoveTemp(D), Source);
	return true;
}

void FGOTABlueprintCombat::UnbindDelegate(UObject* Source, FName DelegateName, UObject* Listener, FName ListenerFunction)
{
	FMulticastDelegateProperty* P = (IsValid(Source) && Listener) ? FindFProperty<FMulticastDelegateProperty>(Source->GetClass(), DelegateName) : nullptr;
	if (P)
	{
		FScriptDelegate D;
		D.BindUFunction(Listener, ListenerFunction);
		P->RemoveDelegate(D, Source);
	}
}

// ============================================================================ cosmetic assets

namespace GOTADuelAssets
{
	static UParticleSystem* Particle(const TCHAR* Path)
	{
		return LoadObject<UParticleSystem>(nullptr, Path, nullptr, LOAD_NoWarn);
	}
	static USoundBase* Sound(const TCHAR* Path)
	{
		return LoadObject<USoundBase>(nullptr, Path, nullptr, LOAD_NoWarn);
	}
	static const TCHAR* SparksSmall = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Sparks/P_Sparks_C.P_Sparks_C");
	static const TCHAR* SparksBig = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Sparks/P_Sparks_C.P_Sparks_C");
	static const TCHAR* SparksBurst = TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Sparks/P_Sparks_C.P_Sparks_C");
	static const TCHAR* Clash = TEXT("/Game/Audio/axe-block.axe-block");
	static const TCHAR* Charge = TEXT("/Game/Audio/energy-charge-up.energy-charge-up");
	static const TCHAR* Divine = TEXT("/Game/Audio/sci-fi-explosion.sci-fi-explosion");
	static const TCHAR* Chime = TEXT("/Game/Audio/UI/S_GOTA_UI_Confirm.S_GOTA_UI_Confirm");
	static const TCHAR* Error = TEXT("/Game/Audio/UI/S_GOTA_UI_Error.S_GOTA_UI_Error");
}

// ============================================================================ fighter component

UGOTADuelFighterComponent::UGOTADuelFighterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.f / 30.f;
	SetIsReplicatedByDefault(true);
}

void UGOTADuelFighterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGOTADuelFighterComponent, bIsChampion);
	DOREPLIFETIME(UGOTADuelFighterComponent, Opponent);
	DOREPLIFETIME(UGOTADuelFighterComponent, Momentum);
	DOREPLIFETIME(UGOTADuelFighterComponent, MaxMomentum);
}

void UGOTADuelFighterComponent::BeginPlay()
{
	Super::BeginPlay();
	TryBindLocalInput();
}

void UGOTADuelFighterComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (bDelegatesBound)
	{
		FGOTABlueprintCombat::UnbindDelegate(DamageSystem.Get(), TEXT("OnBlocked"), this, GET_FUNCTION_NAME_CHECKED(UGOTADuelFighterComponent, HandleBlocked));
		bDelegatesBound = false;
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

void UGOTADuelFighterComponent::InitFighter(bool bInChampion, APawn* InOpponent)
{
	bIsChampion = bInChampion;
	Opponent = InOpponent;
	DamageSystem = UGOTACombatantObserver::FindDamageSystem(GetOwner());
	if (!bIsChampion && !bDelegatesBound && DamageSystem.IsValid())
	{
		bDelegatesBound = FGOTABlueprintCombat::BindDelegate(DamageSystem.Get(), TEXT("OnBlocked"), this,
			GET_FUNCTION_NAME_CHECKED(UGOTADuelFighterComponent, HandleBlocked));
	}
	if (UStatsComponent* Stats = FindOrAddStats())
	{
		MaxMomentum = Stats->GetMaxDivineMomentum();
		Momentum = Stats->GetDivineMomentum();
	}
	LastOwnHealth = LastOpponentHealth = -1.f;
	UE_LOG(LogGOTADuelAI, Log, TEXT("Fighter %s ready (%s, opponent %s, OnBlocked bound=%d)"), *GetNameSafe(GetOwner()),
		bIsChampion ? TEXT("champion") : TEXT("challenger"), *GetNameSafe(InOpponent), bDelegatesBound ? 1 : 0);
}

ADuelGameState* UGOTADuelFighterComponent::GetDuelGS() const
{
	return GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr;
}

bool UGOTADuelFighterComponent::IsRoundLive() const
{
	const ADuelGameState* GS = GetDuelGS();
	return GS && GS->DuelPhase == EDuelPhase::InRound && GS->bCombatEnabled;
}

UStatsComponent* UGOTADuelFighterComponent::FindOrAddStats() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	UStatsComponent* Stats = Owner->FindComponentByClass<UStatsComponent>();
	if (!Stats && Owner->HasAuthority())
	{
		Stats = NewObject<UStatsComponent>(Owner, TEXT("GOTA_DuelStats"));
		Stats->RegisterComponent();
	}
	return Stats;
}

void UGOTADuelFighterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bInputBound)
	{
		TryBindLocalInput();
	}
	if (GetOwner() && GetOwner()->HasAuthority() && !bIsChampion)
	{
		TickChallengerServer();
	}
}

void UGOTADuelFighterComponent::TryBindLocalInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController() || !PC->InputComponent)
	{
		return;
	}
	PC->InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &UGOTADuelFighterComponent::RequestDivineCounter);
	PC->InputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &UGOTADuelFighterComponent::RequestDivineCounter);
	bInputBound = true;
}

void UGOTADuelFighterComponent::TickChallengerServer()
{
	UStatsComponent* Stats = FindOrAddStats();
	if (Stats)
	{
		Momentum = Stats->GetDivineMomentum();
		MaxMomentum = Stats->GetMaxDivineMomentum();
	}
	UActorComponent* DS = DamageSystem.Get();
	APawn* Opp = Opponent.Get();
	if (!IsRoundLive() || !DS || !IsValid(Opp))
	{
		bDodgeArmed = false;
		LastOwnHealth = LastOpponentHealth = -1.f;
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const float OwnHealth = UGOTACombatantObserver::ReadVitals(GetOwner()).Health;
	const float OppHealth = UGOTACombatantObserver::ReadVitals(Opp).Health;

	// Landed hits build momentum (the Blueprint already applied the damage).
	if (LastOpponentHealth >= 0.f && OppHealth < LastOpponentHealth - 0.5f)
	{
		GrantMomentum(HitLandedMomentum, TEXT("hit landed"));
	}

	// Perfect dodge: BP_Player's dodge raises IsInvincible for its i-frames. If that happens while the
	// opponent's attack montage is within PerfectDodgeLeadSeconds of its impact notify, and the attack
	// then fails to damage us, it was a perfect dodge.
	const bool bInvincible = FGOTABlueprintCombat::GetBool(DS, TEXT("IsInvincible"));
	if (bInvincible && !bWasInvincible && Now > CounterInvulnEndTime)
	{
		float Pos = 0.f;
		UAnimMontage* M = FGOTABlueprintCombat::GetActiveMontage(Opp, &Pos);
		const bool bOppAttacking = FGOTABlueprintCombat::GetBool(Opp, TEXT("Attacking"));
		const float Dist = FVector::Dist(Opp->GetActorLocation(), GetOwner()->GetActorLocation());
		const float Impact = FGOTABlueprintCombat::FindNextImpactTime(M, Pos);
		const float Lead = Impact - Pos;
		UE_LOG(LogGOTADuelAI, Log, TEXT("Dodge i-frames: oppAttacking=%d montage=%s pos=%.2f impact=%.2f lead=%.2f dist=%.0f"),
			bOppAttacking ? 1 : 0, *GetNameSafe(M), Pos, Impact, Lead, Dist);
		if (bOppAttacking && M && Impact >= 0.f && Lead <= PerfectDodgeLeadSeconds && Dist < 650.f)
		{
			bDodgeArmed = true;
			HealthAtDodge = OwnHealth;
			DodgeResolveTime = Now + FMath::Max(Lead, 0.f) / FMath::Max(M->RateScale, 0.1f) + 0.2;
		}
	}
	bWasInvincible = bInvincible;
	if (bDodgeArmed && OwnHealth < HealthAtDodge - 0.5f)
	{
		bDodgeArmed = false; // hit anyway
	}
	else if (bDodgeArmed && Now >= DodgeResolveTime)
	{
		bDodgeArmed = false;
		GrantMomentum(PerfectDodgeMomentum, TEXT("perfect dodge"));
		EmitCue(EGOTADuelCue::PerfectDodge, Opp);
	}

	LastOwnHealth = OwnHealth;
	LastOpponentHealth = OppHealth;
}

void UGOTADuelFighterComponent::HandleBlocked(bool bCanBeParried, AActor* DamageCauser)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !IsRoundLive())
	{
		return;
	}
	// The Blueprint parries automatically when a parryable hit lands inside the block montage's
	// ParryWindow notify (BP_Player::MC_OnBlocked → ParryAttack). That is the Perfect Parry.
	const bool bPerfect = bCanBeParried && FGOTABlueprintCombat::GetBool(GetOwner(), TEXT("IsWithinParryWindow"));
	if (bPerfect)
	{
		GrantMomentum(PerfectParryMomentum, TEXT("perfect parry"));
		EmitCue(EGOTADuelCue::PerfectParry, DamageCauser);
	}
	else
	{
		GrantMomentum(BlockMomentum, TEXT("block"));
		EmitCue(EGOTADuelCue::Blocked, DamageCauser);
	}
}

void UGOTADuelFighterComponent::GrantMomentum(float Amount, const TCHAR* Reason)
{
	UStatsComponent* Stats = FindOrAddStats();
	if (!Stats || Amount <= 0.f)
	{
		return;
	}
	Stats->AddDivineMomentum(Amount);
	Momentum = Stats->GetDivineMomentum();
	UE_LOG(LogGOTADuelAI, Log, TEXT("Momentum %s +%.0f (%s) -> %.0f"), *GetNameSafe(GetOwner()), Amount, Reason, Momentum);
}

void UGOTADuelFighterComponent::RequestDivineCounter()
{
	if (bIsChampion)
	{
		return;
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Server_RequestDivineCounter_Implementation();
	}
	else
	{
		Server_RequestDivineCounter();
	}
}

void UGOTADuelFighterComponent::Server_RequestDivineCounter_Implementation()
{
	AActor* Owner = GetOwner();
	APawn* Opp = Opponent.Get();
	UStatsComponent* Stats = FindOrAddStats();
	if (!Owner || !IsRoundLive() || !Stats || bIsChampion)
	{
		return;
	}
	if (Stats->GetDivineMomentum() < DivineCounterCost)
	{
		Client_CounterDenied(0);
		return;
	}
	if (!IsValid(Opp) || UGOTACombatantObserver::IsCombatantDefeated(Opp)
		|| FVector::Dist(Opp->GetActorLocation(), Owner->GetActorLocation()) > DivineCounterRange)
	{
		Client_CounterDenied(1);
		return;
	}
	if (!Stats->ConsumeDivineMomentum(DivineCounterCost))
	{
		Client_CounterDenied(0);
		return;
	}
	Momentum = Stats->GetDivineMomentum();

	// Face the champion, brief invulnerability through the Blueprint's own flag, then the counter blow.
	FRotator Face = (Opp->GetActorLocation() - Owner->GetActorLocation()).Rotation();
	Face.Pitch = 0.f;
	Owner->SetActorRotation(Face);
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		if (AController* C = Pawn->GetController())
		{
			C->SetControlRotation(Face);
		}
	}
	UActorComponent* DS = DamageSystem.Get();
	const double Now = GetWorld()->GetTimeSeconds();
	CounterInvulnEndTime = Now + 0.6;
	if (DS)
	{
		FGOTABlueprintCombat::SetBool(DS, TEXT("IsInvincible"), true);
		FTimerHandle Handle;
		TWeakObjectPtr<UActorComponent> WeakDS = DS;
		TWeakObjectPtr<UGOTADuelFighterComponent> WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [WeakDS, WeakThis]()
		{
			if (WeakDS.IsValid() && WeakThis.IsValid() && WeakThis->GetWorld()->GetTimeSeconds() >= WeakThis->CounterInvulnEndTime - 0.01)
			{
				FGOTABlueprintCombat::SetBool(WeakDS.Get(), TEXT("IsInvincible"), false);
			}
		}), 0.6f, false);
	}
	FGOTABlueprintCombat::Call(Owner, TEXT("MeleeAttack"));

	UGOTAChampionDirectorComponent* Director = Opp->FindComponentByClass<UGOTAChampionDirectorComponent>();
	const float Damage = DivineCounterDamage * ((Director && Director->IsGuardBroken()) ? 1.4f : 1.f);
	const bool bLanded = FGOTABlueprintCombat::ApplyDamage(Opp, Owner, Damage, false, false, TEXT("Stagger"));
	UE_LOG(LogGOTADuelAI, Log, TEXT("DIVINE COUNTER by %s on %s: %.0f damage (landed=%d, momentum left %.0f)"),
		*GetNameSafe(Owner), *GetNameSafe(Opp), Damage, bLanded ? 1 : 0, Momentum);
	EmitCue(EGOTADuelCue::DivineCounter, Opp);
	if (Director)
	{
		Director->NotifyDivineCounterReceived();
	}
}

void UGOTADuelFighterComponent::Client_CounterDenied_Implementation(uint8 Reason)
{
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->OnDuelCombatCue.Broadcast(EGOTADuelCue::CounterDenied, GetOwner(), Reason == 1 ? Opponent.Get() : nullptr);
	}
	if (USoundBase* S = GOTADuelAssets::Sound(GOTADuelAssets::Error))
	{
		UGameplayStatics::PlaySound2D(this, S, 0.6f);
	}
}

void UGOTADuelFighterComponent::EmitCue(EGOTADuelCue Cue, AActor* Target)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	FVector Loc = Owner->GetActorLocation();
	if (Target)
	{
		Loc = (Loc + Target->GetActorLocation()) * 0.5f;
	}
	Loc.Z += 50.f;
	Multicast_Cue(Cue, Target, Loc);
}

void UGOTADuelFighterComponent::Multicast_Cue_Implementation(EGOTADuelCue Cue, AActor* Target, FVector_NetQuantize Location)
{
	PlayCueLocally(Cue, Target, Location);
}

void UGOTADuelFighterComponent::ApplyHitStop(AActor* Actor, float Seconds)
{
	ACharacter* Char = Cast<ACharacter>(Actor);
	USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;
	if (!Mesh || Seconds <= 0.f || CVarDuelHitStop.GetValueOnGameThread() == 0)
	{
		return;
	}
	Mesh->GlobalAnimRateScale = 0.02f;
	FTimerHandle Handle;
	TWeakObjectPtr<USkeletalMeshComponent> WeakMesh = Mesh;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Mesh, [WeakMesh]()
	{
		if (WeakMesh.IsValid())
		{
			WeakMesh->GlobalAnimRateScale = 1.f;
		}
	}), Seconds, false);
}

void UGOTADuelFighterComponent::PlayCueLocally(EGOTADuelCue Cue, AActor* Target, const FVector& Location)
{
	if (ADuelGameState* GS = GetDuelGS())
	{
		GS->OnDuelCombatCue.Broadcast(Cue, GetOwner(), Target);
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const TCHAR* Fx = nullptr;
	const TCHAR* Sfx = nullptr;
	float HitStop = 0.f;
	float Kick = 0.f;
	switch (Cue)
	{
	case EGOTADuelCue::Blocked: Fx = GOTADuelAssets::SparksSmall; Sfx = GOTADuelAssets::Clash; HitStop = HitStopBlockSeconds; Kick = 0.6f; break;
	case EGOTADuelCue::PerfectParry: Fx = GOTADuelAssets::SparksBig; Sfx = GOTADuelAssets::Clash; HitStop = HitStopPerfectSeconds; Kick = 1.2f; break;
	case EGOTADuelCue::PerfectDodge: Sfx = GOTADuelAssets::Chime; Kick = 0.35f; break;
	case EGOTADuelCue::GuardBreak: Fx = GOTADuelAssets::SparksBurst; Sfx = GOTADuelAssets::Clash; HitStop = HitStopPerfectSeconds; Kick = 1.4f; break;
	case EGOTADuelCue::UnblockableTelegraph: Sfx = GOTADuelAssets::Charge; break;
	case EGOTADuelCue::PhaseTwo: Sfx = GOTADuelAssets::Charge; Kick = 1.f; break;
	case EGOTADuelCue::SurgeTelegraph: Sfx = GOTADuelAssets::Charge; Kick = 0.5f; break;
	case EGOTADuelCue::DivineCounter: Fx = GOTADuelAssets::SparksBig; Sfx = GOTADuelAssets::Divine; HitStop = 0.11f; Kick = 1.6f; break;
	default: break;
	}
	UWorld* World = GetWorld();
	if (Fx)
	{
		if (UParticleSystem* PS = GOTADuelAssets::Particle(Fx))
		{
			// Small, brief impact sparks: removed after half a second so they never linger over the fight.
			const float Scale = (Cue == EGOTADuelCue::DivineCounter || Cue == EGOTADuelCue::GuardBreak) ? 0.35f : 0.22f;
			if (UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(World, PS, Location, FRotator::ZeroRotator, FVector(Scale), true))
			{
				FTimerHandle Handle;
				TWeakObjectPtr<UParticleSystemComponent> WeakPSC = PSC;
				World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(PSC, [WeakPSC]()
				{
					if (WeakPSC.IsValid())
					{
						WeakPSC->DestroyComponent();
					}
				}), 0.5f, false);
			}
		}
	}
	if (Sfx)
	{
		if (USoundBase* S = GOTADuelAssets::Sound(Sfx))
		{
			UGameplayStatics::PlaySoundAtLocation(this, S, Location, Cue == EGOTADuelCue::Blocked ? 0.7f : 1.f);
		}
	}
	// Hit-stop: mesh animation only, applied to both combatants equally after the Blueprint has
	// already resolved the hit. Never touches global time dilation or movement simulation.
	if (HitStop > 0.f)
	{
		ApplyHitStop(GetOwner(), HitStop);
		ApplyHitStop(Target, HitStop);
	}
	if (Kick > 0.f)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (PC && PC->IsLocalController() && (PC->GetPawn() == GetOwner() || PC->GetPawn() == Target))
			{
				if (AGOTAPlayerCameraManager* Cam = Cast<AGOTAPlayerCameraManager>(PC->PlayerCameraManager))
				{
					Cam->AddImpactKick(Kick);
				}
			}
		}
	}
}

// ============================================================================ champion director

UGOTAChampionDirectorComponent::UGOTAChampionDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
	SetIsReplicatedByDefault(false);
}

void UGOTAChampionDirectorComponent::InitDirector(APawn* InChallenger, EGOTADuelDifficulty InDifficulty)
{
	Challenger = InChallenger;
	Difficulty = InDifficulty;
	Profile = FGOTAChampionProfile::ForDifficulty(InDifficulty);
	Phase = 1;
	GuardBrokenUntil = 0.0;
	NextSurgeTime = 0.0;
	SurgeReleaseTime = PhaseTwoReleaseTime = PendingReactionTime = -1.0;
	BlockTimes.Reset();
	APawn* Champion = Cast<APawn>(GetOwner());
	FGOTABlueprintCombat::SetNumber(Champion, TEXT("BlockChance"), Profile.ReactiveBlockChance);
	if (!bBound)
	{
		bBound = FGOTABlueprintCombat::BindDelegate(UGOTACombatantObserver::FindDamageSystem(Champion), TEXT("OnBlocked"), this,
			GET_FUNCTION_NAME_CHECKED(UGOTAChampionDirectorComponent, HandleChampionBlocked));
	}
	if (ADuelGameState* GS = GetWorld()->GetGameState<ADuelGameState>())
	{
		GS->ChampionPhase = 1;
		GS->bChampionGuardBroken = false;
	}
	static const TCHAR* NetModes[] = { TEXT("Standalone"), TEXT("DedicatedServer"), TEXT("ListenServer"), TEXT("Client") };
	UE_LOG(LogGOTADuelAI, Log, TEXT("Sovereign Champion director: %s (react %.2fs defend %.0f%% guard-overload %d, hp %.0f vs %.0f) vs %s [net %s]"),
		*Profile.DisplayName.ToString(), Profile.ReactionSeconds, Profile.DefendChance * 100.f, Profile.GuardOverloadHits,
		Profile.ChampionMaxHealth, Profile.ChallengerMaxHealth, *GetNameSafe(InChallenger), NetModes[FMath::Clamp<int32>(GetNetMode(), 0, 3)]);
}

void UGOTAChampionDirectorComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (bBound)
	{
		FGOTABlueprintCombat::UnbindDelegate(UGOTACombatantObserver::FindDamageSystem(GetOwner()), TEXT("OnBlocked"), this,
			GET_FUNCTION_NAME_CHECKED(UGOTAChampionDirectorComponent, HandleChampionBlocked));
		bBound = false;
	}
	Super::EndPlay(Reason);
}

bool UGOTAChampionDirectorComponent::IsRoundLive() const
{
	const ADuelGameState* GS = GetWorld() ? GetWorld()->GetGameState<ADuelGameState>() : nullptr;
	return GS && GS->DuelPhase == EDuelPhase::InRound && GS->bCombatEnabled;
}

bool UGOTAChampionDirectorComponent::IsGuardBroken() const
{
	return GetWorld() && GuardBrokenUntil > GetWorld()->GetTimeSeconds();
}

bool UGOTAChampionDirectorComponent::IsChampionAttacking(const APawn* Champion) const
{
	return FGOTABlueprintCombat::GetBool(Champion, TEXT("Attacking"));
}

float UGOTAChampionDirectorComponent::EffectiveDefendChance() const
{
	return FMath::Clamp(Profile.DefendChance + (Phase >= 2 ? Profile.PhaseTwoDefendBonus : 0.f), 0.f, 0.9f);
}

float UGOTAChampionDirectorComponent::EffectiveReaction() const
{
	// Human-like floor: never answers faster than 0.2s after the wind-up becomes visible.
	return FMath::Max(0.2f, Profile.ReactionSeconds * (Phase >= 2 ? Profile.PhaseTwoReactionScale : 1.f));
}

void UGOTAChampionDirectorComponent::Cue(EGOTADuelCue InCue, AActor* Target)
{
	if (UGOTADuelFighterComponent* Fighter = GetOwner()->FindComponentByClass<UGOTADuelFighterComponent>())
	{
		Fighter->EmitCue(InCue, Target);
	}
}

void UGOTAChampionDirectorComponent::SetBrainPaused(bool bPaused, const TCHAR* Reason)
{
	const APawn* Champion = Cast<APawn>(GetOwner());
	AAIController* AIC = Champion ? Cast<AAIController>(Champion->GetController()) : nullptr;
	UBrainComponent* Brain = AIC ? AIC->GetBrainComponent() : nullptr;
	if (!Brain)
	{
		return;
	}
	if (bPaused)
	{
		Brain->PauseLogic(Reason);
		AIC->StopMovement();
	}
	else if (IsRoundLive())
	{
		Brain->ResumeLogic(Reason);
	}
}

void UGOTAChampionDirectorComponent::FaceChallenger(APawn* Champion, APawn* InChallenger) const
{
	if (!Champion || !InChallenger)
	{
		return;
	}
	FRotator Rot = (InChallenger->GetActorLocation() - Champion->GetActorLocation()).Rotation();
	Rot.Pitch = 0.f;
	Champion->SetActorRotation(Rot);
}

void UGOTAChampionDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APawn* Champion = Cast<APawn>(GetOwner());
	APawn* Foe = Challenger.Get();
	if (!Champion || !Champion->HasAuthority())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (!IsRoundLive() || !IsValid(Foe) || UGOTACombatantObserver::IsCombatantDefeated(Champion))
	{
		PendingReactionTime = -1.0;
		SurgeReleaseTime = -1.0;
		return;
	}

	if (GuardBrokenUntil > 0.0 && Now >= GuardBrokenUntil)
	{
		GuardBrokenUntil = 0.0;
		FGOTABlueprintCombat::SetNumber(Champion, TEXT("BlockChance"), Profile.ReactiveBlockChance + (Phase >= 2 ? Profile.PhaseTwoDefendBonus : 0.f));
		if (ADuelGameState* GS = GetWorld()->GetGameState<ADuelGameState>())
		{
			GS->bChampionGuardBroken = false;
		}
		LastDecision = TEXT("guard restored");
	}

	if (!ForcedDecision.IsEmpty())
	{
		const FString What = ForcedDecision;
		ForcedDecision.Reset();
		if (What == TEXT("block")) { ExecuteDefence(Champion, Foe, false); }
		else if (What == TEXT("sidestep")) { ExecuteDefence(Champion, Foe, true); }
		else if (What == TEXT("guardbreak")) { BreakGuard(Champion, Foe); }
		else if (What == TEXT("phase2") && Phase < 2) { EnterPhaseTwo(Champion, Foe); }
		else if (What == TEXT("surge")) { BeginSurge(Champion, Foe, Now); }
	}

	TickTelegraphs(Champion);
	TickPhase(Champion, Now);
	TickSurge(Champion, Foe, Now);
	TickReactions(Champion, Foe, Now);

#if !UE_BUILD_SHIPPING
	if (CVarDuelDebugOverlay.GetValueOnGameThread() != 0 && GEngine)
	{
		const UGOTADuelFighterComponent* FoeFighter = Foe->FindComponentByClass<UGOTADuelFighterComponent>();
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.2f, FColor(255, 210, 120),
			FString::Printf(TEXT("%s\nchallenger momentum %.0f  attacking=%d blocking=%d parryWin=%d"), *DescribeState(),
				FoeFighter ? FoeFighter->GetMomentum() : -1.f,
				FGOTABlueprintCombat::GetBool(Foe, TEXT("Attacking")) ? 1 : 0,
				FGOTABlueprintCombat::GetBool(UGOTACombatantObserver::FindDamageSystem(Foe), TEXT("IsBlocking")) ? 1 : 0,
				FGOTABlueprintCombat::GetBool(Foe, TEXT("IsWithinParryWindow")) ? 1 : 0));
	}
#endif
}

void UGOTAChampionDirectorComponent::TickTelegraphs(APawn* Champion)
{
	UAnimMontage* M = FGOTABlueprintCombat::GetActiveMontage(Champion);
	const FName Name = M ? M->GetFName() : NAME_None;
	if (Name != LastMontageName)
	{
		LastMontageName = Name;
		// The Blueprint GroundSmash deals CanBeBlocked=false damage: warn the challenger to evade.
		// (Surge / phase-two slams already announced themselves.)
		const bool bScripted = SurgeReleaseTime < 0.0 && PhaseTwoReleaseTime < 0.0 && LastDecision.StartsWith(TEXT("surge"));
		if (Name.ToString().Contains(TEXT("GroundSmash")) && !bScripted)
		{
			Cue(EGOTADuelCue::UnblockableTelegraph, Challenger.Get());
		}
	}
}

void UGOTAChampionDirectorComponent::TickReactions(APawn* Champion, APawn* Foe, double Now)
{
	// Reads only what the challenger visibly does: the replicated BP_Player::Attacking flag that
	// starts its swing montage. No input, no prediction of buttons.
	const bool bFoeAttacking = FGOTABlueprintCombat::GetBool(Foe, TEXT("Attacking"));
	const bool bBusy = IsGuardBroken() || SurgeReleaseTime > 0.0 || PhaseTwoReleaseTime > 0.0;
	// A visible wind-up: the swing starts, or a new combo section/montage begins while swinging.
	FName FoeKey = NAME_None;
	if (bFoeAttacking)
	{
		const ACharacter* FoeChar = Cast<ACharacter>(Foe);
		UAnimInstance* Anim = (FoeChar && FoeChar->GetMesh()) ? FoeChar->GetMesh()->GetAnimInstance() : nullptr;
		if (UAnimMontage* FoeM = Anim ? Anim->GetCurrentActiveMontage() : nullptr)
		{
			FoeKey = FName(*(FoeM->GetName() + TEXT(":") + Anim->Montage_GetCurrentSection(FoeM).ToString()));
		}
	}
	const bool bWindUp = bFoeAttacking && (!bChallengerWasAttacking || (FoeKey != LastFoeMontageKey && !FoeKey.IsNone()));
	LastFoeMontageKey = FoeKey;
	if (bWindUp && !bBusy && PendingReactionTime < 0.0)
	{
		const bool bChain = Now < GuardChainUntil;
		const float Dist = FVector::Dist(Foe->GetActorLocation(), Champion->GetActorLocation());
		if (IsChampionAttacking(Champion))
		{
			LastDecision = TEXT("committed to own attack");
		}
		else if (Dist > 340.f)
		{
			LastDecision = TEXT("out of reach");
		}
		else if (bChain && FMath::FRand() < Profile.GuardChainChance)
		{
			// Still braced from the last blocked hit: the guard comes back up faster (shield pressure builds).
			PendingReactionTime = Now + FMath::Max(0.2f, EffectiveReaction() * 0.6f);
			bPendingSidestep = false;
			LastDecision = TEXT("braced -> guard chain");
		}
		else if (FMath::FRand() < EffectiveDefendChance())
		{
			PendingReactionTime = Now + EffectiveReaction();
			bPendingSidestep = FMath::FRand() < Profile.SidestepShare;
			LastDecision = FString::Printf(TEXT("read wind-up -> %s in %.2fs"), bPendingSidestep ? TEXT("sidestep") : TEXT("guard"), EffectiveReaction());
		}
		else
		{
			LastDecision = TEXT("let it through");
		}
		UE_LOG(LogGOTADuelAI, Log, TEXT("Champion read wind-up [%s] dist=%.0f chain=%d -> %s"), *FoeKey.ToString(),
			FVector::Dist(Foe->GetActorLocation(), Champion->GetActorLocation()), bChain ? 1 : 0, *LastDecision);
	}
	bChallengerWasAttacking = bFoeAttacking;

	if (PendingReactionTime > 0.0 && Now >= PendingReactionTime)
	{
		PendingReactionTime = -1.0;
		if (!IsChampionAttacking(Champion) && !bBusy)
		{
			ExecuteDefence(Champion, Foe, bPendingSidestep);
		}
	}
}

void UGOTAChampionDirectorComponent::ExecuteDefence(APawn* Champion, APawn* Foe, bool bSidestep)
{
	FaceChallenger(Champion, Foe);
	if (bSidestep)
	{
		if (ACharacter* Char = Cast<ACharacter>(Champion))
		{
			const FVector ToFoe = (Foe->GetActorLocation() - Champion->GetActorLocation()).GetSafeNormal2D();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, ToFoe) * (FMath::RandBool() ? 1.f : -1.f);
			Char->LaunchCharacter(Side * 720.f - ToFoe * 260.f + FVector(0.f, 0.f, 180.f), true, true);
		}
		++Sidesteps;
		LastDecision = TEXT("sidestep");
	}
	else
	{
		FGOTABlueprintCombat::Call(Champion, TEXT("StartBlock"));
		++Defences;
		LastDecision = TEXT("guard raised");
	}
	Cue(EGOTADuelCue::ChampionDefended, Foe);
	UE_LOG(LogGOTADuelAI, Log, TEXT("Champion defends: %s"), *LastDecision);
}

void UGOTAChampionDirectorComponent::HandleChampionBlocked(bool bCanBeParried, AActor* DamageCauser)
{
	if (!IsRoundLive())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	BlockTimes.Add(Now);
	GuardChainUntil = Now + Profile.GuardChainSeconds;
	BlockTimes.RemoveAll([this, Now](double T) { return Now - T > Profile.GuardOverloadWindowSeconds; });
	Cue(EGOTADuelCue::Blocked, DamageCauser);
	UE_LOG(LogGOTADuelAI, Log, TEXT("Champion blocked a hit (shield pressure %d/%d)"), BlockTimes.Num(), Profile.GuardOverloadHits);
	if (BlockTimes.Num() >= Profile.GuardOverloadHits)
	{
		BlockTimes.Reset();
		BreakGuard(Cast<APawn>(GetOwner()), Challenger.Get());
	}
}

void UGOTAChampionDirectorComponent::BreakGuard(APawn* Champion, APawn* Foe)
{
	if (!Champion)
	{
		return;
	}
	// Shield overload: the guard shatters. The Blueprint's own EndBlock + HitResponse(Stagger).
	FGOTABlueprintCombat::Call(Champion, TEXT("EndBlock"));
	FGOTABlueprintCombat::SetNumber(Champion, TEXT("BlockChance"), 0.0);
	FGOTABlueprintCombat::Call(Champion, TEXT("HitResponse"), Foe, TEXT("Stagger"));
	GuardBrokenUntil = GetWorld()->GetTimeSeconds() + Profile.GuardBreakSeconds;
	GuardChainUntil = 0.0;
	PendingReactionTime = -1.0;
	++GuardBreaks;
	LastDecision = TEXT("GUARD BROKEN");
	if (ADuelGameState* GS = GetWorld()->GetGameState<ADuelGameState>())
	{
		GS->bChampionGuardBroken = true;
	}
	if (UGOTADuelFighterComponent* FoeFighter = Foe ? Foe->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr)
	{
		FoeFighter->GrantMomentum(FoeFighter->GuardBreakMomentum, TEXT("guard break"));
	}
	Cue(EGOTADuelCue::GuardBreak, Foe);
	UE_LOG(LogGOTADuelAI, Log, TEXT("Champion guard broken for %.1fs"), Profile.GuardBreakSeconds);
}

void UGOTAChampionDirectorComponent::TickPhase(APawn* Champion, double Now)
{
	if (PhaseTwoReleaseTime > 0.0)
	{
		FaceChallenger(Champion, Challenger.Get());
		if (Now >= PhaseTwoReleaseTime)
		{
			PhaseTwoReleaseTime = -1.0;
			FGOTABlueprintCombat::SetBool(UGOTACombatantObserver::FindDamageSystem(Champion), TEXT("IsInvincible"), false);
			SetBrainPaused(false, TEXT("Ascension"));
			if (bPhaseTwoSlamPending)
			{
				bPhaseTwoSlamPending = false;
				LastDecision = TEXT("surge: ascension slam");
				UE_LOG(LogGOTADuelAI, Log, TEXT("Ascension slam released"));
				FGOTABlueprintCombat::Call(Champion, TEXT("GroundSmashAttack"), Challenger.Get());
			}
		}
		return;
	}
	if (Phase < 2)
	{
		const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Champion);
		if (V.bHasDamageSystem && !V.bIsDead && V.Health > 0.f && V.GetFraction() <= Profile.PhaseTwoHealthFraction)
		{
			EnterPhaseTwo(Champion, Challenger.Get());
		}
	}
}

void UGOTAChampionDirectorComponent::EnterPhaseTwo(APawn* Champion, APawn* Foe)
{
	Phase = 2;
	const double Now = GetWorld()->GetTimeSeconds();
	// Ascension: a short, announced invulnerable beat (the Blueprint's own IsInvincible flag),
	// then a telegraphed unblockable slam. Faster reads and a firmer guard from here on.
	FGOTABlueprintCombat::SetBool(UGOTACombatantObserver::FindDamageSystem(Champion), TEXT("IsInvincible"), true);
	SetBrainPaused(true, TEXT("Ascension"));
	FaceChallenger(Champion, Foe);
	PhaseTwoReleaseTime = Now + 1.4;
	bPhaseTwoSlamPending = true;
	NextSurgeTime = Now + 1.4 + Profile.SurgeIntervalSeconds * 0.4f;
	PendingReactionTime = -1.0;
	FGOTABlueprintCombat::SetNumber(Champion, TEXT("BlockChance"), Profile.ReactiveBlockChance + Profile.PhaseTwoDefendBonus);
	if (ACharacter* Char = Cast<ACharacter>(Champion))
	{
		Char->GetCharacterMovement()->MaxWalkSpeed *= 1.1f;
	}
	if (ADuelGameState* GS = GetWorld()->GetGameState<ADuelGameState>())
	{
		GS->ChampionPhase = 2;
	}
	LastDecision = TEXT("PHASE II");
	Cue(EGOTADuelCue::PhaseTwo, Foe);
	UE_LOG(LogGOTADuelAI, Log, TEXT("Sovereign Champion enters PHASE II"));
}

void UGOTAChampionDirectorComponent::TickSurge(APawn* Champion, APawn* Foe, double Now)
{
	if (SurgeReleaseTime > 0.0)
	{
		FaceChallenger(Champion, Foe);
		if (Now >= SurgeReleaseTime)
		{
			ReleaseSurge();
		}
		return;
	}
	if (Phase < 2 || PhaseTwoReleaseTime > 0.0 || Now < NextSurgeTime || IsGuardBroken() || IsChampionAttacking(Champion))
	{
		return;
	}
	if (FVector::Dist(Foe->GetActorLocation(), Champion->GetActorLocation()) <= 700.f)
	{
		BeginSurge(Champion, Foe, Now);
	}
}

void UGOTAChampionDirectorComponent::BeginSurge(APawn* Champion, APawn* Foe, double Now)
{
	SetBrainPaused(true, TEXT("SovereignSurge"));
	FaceChallenger(Champion, Foe);
	SurgeReleaseTime = Now + Profile.SurgeTelegraphSeconds;
	PendingReactionTime = -1.0;
	++Surges;
	LastDecision = TEXT("surge: telegraph");
	Cue(EGOTADuelCue::SurgeTelegraph, Foe);
	UE_LOG(LogGOTADuelAI, Log, TEXT("Sovereign Surge telegraphed (%.1fs)"), Profile.SurgeTelegraphSeconds);
}

void UGOTAChampionDirectorComponent::ReleaseSurge()
{
	APawn* Champion = Cast<APawn>(GetOwner());
	SurgeReleaseTime = -1.0;
	NextSurgeTime = GetWorld()->GetTimeSeconds() + Profile.SurgeIntervalSeconds;
	SetBrainPaused(false, TEXT("SovereignSurge"));
	LastDecision = TEXT("surge: release");
	UE_LOG(LogGOTADuelAI, Log, TEXT("Sovereign Surge released (GroundSmashAttack)"));
	FGOTABlueprintCombat::Call(Champion, TEXT("GroundSmashAttack"), Challenger.Get());
}

void UGOTAChampionDirectorComponent::NotifyDivineCounterReceived()
{
	// A Divine Counter staggers the champion; it also resets shield pressure and any pending read.
	PendingReactionTime = -1.0;
	BlockTimes.Reset();
	LastDecision = TEXT("staggered by Divine Counter");
}

void UGOTAChampionDirectorComponent::DebugForce(const FString& What)
{
	ForcedDecision = What.ToLower();
}

FString UGOTAChampionDirectorComponent::DescribeState() const
{
	const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(GetOwner());
	return FString::Printf(TEXT("SOVEREIGN CHAMPION [%s] phase=%d hp=%.0f/%.0f guard=%s pressure=%d/%d defences=%d sidesteps=%d breaks=%d surges=%d | %s"),
		*Profile.DisplayName.ToString(), Phase, V.Health, V.MaxHealth, IsGuardBroken() ? TEXT("BROKEN") : TEXT("ok"),
		BlockTimes.Num(), Profile.GuardOverloadHits, Defences, Sidesteps, GuardBreaks, Surges, *LastDecision);
}
