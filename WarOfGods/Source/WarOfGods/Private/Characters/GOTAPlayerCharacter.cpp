// White Tiger Studios 2024

#include "Characters/GOTAPlayerCharacter.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/StatsComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/ProgressBar.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/Stack.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTANetGuard, Log, All);

namespace GOTANetGuard
{
	static const FName ParryAttack(TEXT("SR_ParryAttack"));
	static const FName OnBlocked(TEXT("SR_OnBlocked"));

	static bool ReadBool(const UObject* Obj, FName Name)
	{
		const FBoolProperty* P = Obj ? FindFProperty<FBoolProperty>(Obj->GetClass(), Name) : nullptr;
		return P && P->GetPropertyValue_InContainer(Obj);
	}
}

AGOTAPlayerCharacter::AGOTAPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGOTAPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Server: remember when this pawn was last defending, so a riposte request that arrives one round trip
	// after the block is still honoured.
	if (HasAuthority() && IsDefendingNow())
	{
		LastDefendingTime = GetWorld()->GetTimeSeconds();
	}
	NeutralizeUnsafeGlobalSlowMo();
}

void AGOTAPlayerCharacter::NeutralizeUnsafeGlobalSlowMo()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Standalone)
	{
		return; // solo: keep BP_Player's authored feel
	}
	const float Dil = UGameplayStatics::GetGlobalTimeDilation(World);
	if (FMath::IsNearlyEqual(Dil, 1.f))
	{
		return;
	}
	// The pawn keeps simulating at full speed. Feedback is a local camera pulse.
	CustomTimeDilation = 1.f;
	if (IsLocallyControlled() && Dil < 1.f)
	{
		ApplyLocalParrySlow(Dil);
	}
	UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	if (!bLoggedGlobalSlowMoClamp)
	{
		bLoggedGlobalSlowMoClamp = true;
		UE_LOG(LogGOTANetGuard, Warning,
			TEXT("%s: clamped SetGlobalTimeDilation in multiplayer (pawn CustomTimeDilation=%.2f)"),
			*GetName(), CustomTimeDilation);
	}
}

bool AGOTAPlayerCharacter::IsDefendingNow() const
{
	const UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(this);
	return GOTANetGuard::ReadBool(DS, TEXT("IsBlocking")) || GOTANetGuard::ReadBool(this, TEXT("IsWithinParryWindow"));
}

AActor* AGOTAPlayerCharacter::ReadActorParam(const UFunction* Function, void* Parms)
{
	for (TFieldIterator<FProperty> P(Function); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
	{
		if (const FObjectPropertyBase* O = CastField<FObjectPropertyBase>(*P))
		{
			return Cast<AActor>(O->GetObjectPropertyValue_InContainer(Parms));
		}
	}
	return nullptr;
}

bool AGOTAPlayerCharacter::ValidateServerRPC(const UFunction* Function, void* Parms)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const FName Name = Function->GetFName();
	static const FName CombatActions[] = { TEXT("SR_MeleeAttack"), TEXT("SR_ShootMagicSpell"), TEXT("SR_StartBlock"), GOTANetGuard::ParryAttack, GOTANetGuard::OnBlocked };
	bool bCombatAction = false;
	for (const FName& A : CombatActions)
	{
		bCombatAction |= Name == A;
	}
	if (bCombatAction && UGOTACombatantObserver::IsCombatantDefeated(this))
	{
		return false; // the dead cannot attack, block or parry
	}
	const bool bDefending = IsDefendingNow() || Now - LastDefendingTime <= ParryGraceSeconds;
	if (Name == GOTANetGuard::ParryAttack || Name == GOTANetGuard::OnBlocked)
	{
		AActor* Target = ReadActorParam(Function, Parms);
		const float Range = Name == GOTANetGuard::ParryAttack ? ParryRiposteRange : BlockedCauserRange;
		if (!IsValid(Target) || Target == this || UGOTACombatantObserver::IsCombatantDefeated(Target)
			|| FVector::Dist(Target->GetActorLocation(), GetActorLocation()) > Range || !bDefending)
		{
			return false;
		}
		if (Name == GOTANetGuard::ParryAttack)
		{
			if (Now - LastRiposteTime < ParryRiposteCooldown)
			{
				return false;
			}
			LastRiposteTime = Now;
		}
	}
	return true;
}

void AGOTAPlayerCharacter::ProcessEvent(UFunction* Function, void* Parms)
{
	if (Function && Function->HasAnyFunctionFlags(FUNC_NetServer) && HasAuthority() && GetWorld() && GetWorld()->IsGameWorld()
		&& Function->GetName().StartsWith(TEXT("SR_")) && !ValidateServerRPC(Function, Parms))
	{
		++RejectedRPCs;
		UE_LOG(LogGOTANetGuard, Warning, TEXT("%s: rejected %s (failed server validation, %d rejected so far)"), *GetName(), *Function->GetName(), RejectedRPCs);
		return;
	}
	Super::ProcessEvent(Function, Parms);
}

namespace
{
	float SafeHealthFraction(const UUserWidget* Widget)
	{
		if (!Widget)
		{
			return 0.f;
		}
		const APawn* Pawn = nullptr;
		if (const FObjectProperty* Prop = FindFProperty<FObjectProperty>(Widget->GetClass(), TEXT("PlayerRef")))
		{
			if (UObject* Obj = Prop->GetObjectPropertyValue_InContainer(Widget))
			{
				if (const APawn* AsPawn = Cast<APawn>(Obj))
				{
					Pawn = AsPawn;
				}
				else if (const AController* Controller = Cast<AController>(Obj))
				{
					Pawn = Controller->GetPawn();
				}
			}
		}
		if (!IsValid(Pawn))
		{
			if (const APlayerController* PC = Widget->GetOwningPlayer())
			{
				Pawn = PC->GetPawn();
			}
		}
		if (!IsValid(Pawn))
		{
			return 0.f;
		}
		const UStatsComponent* Stats = Pawn->FindComponentByClass<UStatsComponent>();
		const float Max = Stats ? Stats->GetMaxHealth() : 0.f;
		if (Max <= KINDA_SMALL_NUMBER)
		{
			return 0.f;
		}
		return FMath::Clamp(Stats->GetHealth() / Max, 0.f, 1.f);
	}

	FNativeFuncPtr OriginalSetDilation = nullptr;
	bool bInDilationGuard = false;
	bool bPercentIsDouble = false;

	void ExecSafeGetPercent(UObject* Context, FFrame& Stack, void* Result)
	{
		P_FINISH;
		if (Result)
		{
			const double Fraction = SafeHealthFraction(Cast<UUserWidget>(Context));
			if (bPercentIsDouble)
			{
				*static_cast<double*>(Result) = Fraction;
			}
			else
			{
				*static_cast<float*>(Result) = static_cast<float>(Fraction);
			}
		}
	}

	void ExecGuardedDilation(UObject* Context, FFrame& Stack, void* Result)
	{
		if (OriginalSetDilation)
		{
			OriginalSetDilation(Context, Stack, Result);
		}
		else
		{
			P_FINISH;
		}
		if (bInDilationGuard)
		{
			return;
		}
		bInDilationGuard = true;
		for (TObjectIterator<UWorld> It; It; ++It)
		{
			UWorld* World = *It;
			if (!World->IsGameWorld() || World->GetNetMode() == NM_Standalone)
			{
				continue;
			}
			const float Dil = UGameplayStatics::GetGlobalTimeDilation(World);
			if (Dil >= 1.f - KINDA_SMALL_NUMBER)
			{
				continue;
			}
			if (AWorldSettings* Settings = World->GetWorldSettings())
			{
				Settings->SetTimeDilation(1.f);
			}
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (AGOTAPlayerCharacter* Hero = Cast<AGOTAPlayerCharacter>(PC->GetPawn()))
				{
					Hero->ApplyLocalParrySlow(Dil);
				}
			}
		}
		bInDilationGuard = false;
	}
}

void AGOTAPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	static bool bHudPatched = false;
	if (!bHudPatched)
	{
		if (UClass* HudClass = LoadObject<UClass>(nullptr, TEXT("/Game/Widgets/W_PlayerHUD.W_PlayerHUD_C")))
		{
			if (UFunction* GetPercent = HudClass->FindFunctionByName(TEXT("GetPercent")))
			{
				if (const FProperty* ReturnProp = GetPercent->GetReturnProperty())
				{
					bPercentIsDouble = ReturnProp->IsA<FDoubleProperty>();
				}
				GetPercent->SetNativeFunc(&ExecSafeGetPercent);
				GetPercent->FunctionFlags |= FUNC_Native;
				bHudPatched = true;
				UE_LOG(LogGOTANetGuard, Log, TEXT("W_PlayerHUD GetPercent now reads replicated stats and returns 0 when the pawn is missing"));
			}
		}
	}
	if (IsLocallyControlled())
	{
		BindLocalHud();
	}
}

void AGOTAPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindLocalHud();
	Super::EndPlay(EndPlayReason);
}

void AGOTAPlayerCharacter::RestoreParryCamera()
{
	bParryCameraActive = false;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (APlayerCameraManager* Cam = PC->PlayerCameraManager)
		{
			Cam->SetFOV(SavedFOV);
		}
	}
}

void AGOTAPlayerCharacter::ApplyLocalParrySlow(float Dilation)
{
	// Dilation is the value the Blueprint asked the world to use. It is not applied to this pawn.
	(void)Dilation;
	CustomTimeDilation = 1.f;
	if (!IsLocallyControlled() || !GetWorld())
	{
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (APlayerCameraManager* Cam = PC->PlayerCameraManager)
		{
			if (!bParryCameraActive)
			{
				SavedFOV = Cam->GetFOVAngle();
				bParryCameraActive = true;
			}
			Cam->SetFOV(FMath::Max(SavedFOV * 0.9f, 50.f));
			TWeakObjectPtr<AGOTAPlayerCharacter> WeakThis(this);
			GetWorld()->GetTimerManager().SetTimer(LocalSlowMoHandle, FTimerDelegate::CreateLambda([WeakThis]()
			{
				if (AGOTAPlayerCharacter* Self = WeakThis.Get())
				{
					Self->RestoreParryCamera();
				}
			}), LocalParrySlowMoSeconds, false);
		}
	}
	if (!bLoggedGlobalSlowMoClamp)
	{
		bLoggedGlobalSlowMoClamp = true;
		UE_LOG(LogGOTANetGuard, Warning, TEXT("%s: parry feedback is local camera only (CustomTimeDilation=%.2f)"), *GetName(), CustomTimeDilation);
	}
}

void AGOTAPlayerCharacter::InstallGlobalDilationGuard()
{
	static bool bInstalled = false;
	if (bInstalled)
	{
		return;
	}
	UClass* Statics = FindObject<UClass>(nullptr, TEXT("/Script/Engine.GameplayStatics"));
	UFunction* Fn = Statics ? Statics->FindFunctionByName(TEXT("SetGlobalTimeDilation")) : nullptr;
	if (!Fn)
	{
		return;
	}
	OriginalSetDilation = Fn->GetNativeFunc();
	Fn->SetNativeFunc(&ExecGuardedDilation);
	bInstalled = true;
}

void AGOTAPlayerCharacter::BindLocalHud()
{
	UnbindLocalHud();
	if (!IsLocallyControlled())
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), false);
	UUserWidget* Hud = nullptr;
	for (UUserWidget* Widget : Widgets)
	{
		if (Widget && Widget->GetClass()->GetName().StartsWith(TEXT("W_PlayerHUD")) && Widget->GetOwningPlayer() == PC)
		{
			Hud = Widget;
		}
	}
	if (!Hud || !Hud->WidgetTree)
	{
		return;
	}
	HudBar = Cast<UProgressBar>(Hud->WidgetTree->FindWidget(TEXT("ProgressBar_0")));
	if (UStatsComponent* Stats = FindComponentByClass<UStatsComponent>())
	{
		HudStats = Stats;
		Stats->OnHealthChanged.AddDynamic(this, &AGOTAPlayerCharacter::HandleHudHealth);
		HandleHudHealth(Stats->GetHealth(), Stats->GetMaxHealth());
	}
}

void AGOTAPlayerCharacter::UnbindLocalHud()
{
	if (UStatsComponent* Stats = HudStats.Get())
	{
		Stats->OnHealthChanged.RemoveDynamic(this, &AGOTAPlayerCharacter::HandleHudHealth);
	}
	HudStats = nullptr;
	HudBar = nullptr;
}

void AGOTAPlayerCharacter::HandleHudHealth(float NewValue, float MaxValue)
{
	if (UProgressBar* Bar = HudBar.Get())
	{
		Bar->SetPercent(MaxValue > KINDA_SMALL_NUMBER ? FMath::Clamp(NewValue / MaxValue, 0.f, 1.f) : 0.f);
	}
}
