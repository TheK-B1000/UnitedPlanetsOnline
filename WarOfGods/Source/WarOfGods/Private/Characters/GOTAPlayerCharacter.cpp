// White Tiger Studios 2024

#include "Characters/GOTAPlayerCharacter.h"

#include "Abilities/GAS/GOTAStatusEffects.h"
#include "Arenas/Duel/DuelGameState.h"
#include "GameFramework/GameStateBase.h"
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
	if (bCombatAction && GOTAGAS::IsIncapacitated(this))
	{
		UE_LOG(LogGOTANetGuard, Log, TEXT("%s: %s refused (incapacitated)"), *GetName(), *Name.ToString());
		return false; // Stunned / Frozen: no attack, spell, block or parry
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
	// On the authority (listen host, standalone, an AI-driven hero) MeleeAttack & co. call the MC_ multicast directly and
	// never pass through SR_. A Blueprint's own internal calls do not reach ProcessEvent (the player's input is locked by
	// DisableInput instead); calls into these events from C++ (AI code, test drivers) do, and are refused here.
	if (Function && HasAuthority() && GetWorld() && GetWorld()->IsGameWorld() && GOTAGAS::IsIncapacitated(this))
	{
		static const FName LockedActions[] = { TEXT("MeleeAttack"), TEXT("ShootMagicSpell"), TEXT("StartBlock"), TEXT("ParryAttack"),
			TEXT("MC_MeleeAttack"), TEXT("MC_ShootMagicSpell"), TEXT("MC_StartBlock"), TEXT("MC_ParryAttack") };
		for (const FName& A : LockedActions)
		{
			if (Function->GetFName() == A)
			{
				UE_LOG(LogGOTANetGuard, Log, TEXT("%s: %s refused (incapacitated)"), *GetName(), *A.ToString());
				return;
			}
		}
	}
	Super::ProcessEvent(Function, Parms);
}

namespace
{
	/** The pawn a W_PlayerHUD was created for (its PlayerRef), if it names one. */
	const APawn* HudPlayerRef(const UUserWidget* Widget)
	{
		const FObjectProperty* Prop = Widget ? FindFProperty<FObjectProperty>(Widget->GetClass(), TEXT("PlayerRef")) : nullptr;
		const UObject* Obj = Prop ? Prop->GetObjectPropertyValue_InContainer(Widget) : nullptr;
		if (const AController* Controller = Cast<AController>(Obj))
		{
			return Controller->GetPawn();
		}
		return Cast<APawn>(Obj);
	}

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
		// A shield is over-health in the Blueprint; the health bar shows real health and the shield has its own bar.
		const GOTAGAS::FDisplayVitals Vitals = GOTAGAS::ReadDisplayVitals(Pawn);
		if (Vitals.MaxHealth > KINDA_SMALL_NUMBER)
		{
			return Vitals.GetHealthFraction();
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
	if (GetNetMode() != NM_DedicatedServer)
	{
		// BP_Player can add its HUD after a delay, on any screen, so check for a few seconds after each pawn appears.
		HudSweepsLeft = 20;
		GetWorldTimerManager().SetTimer(HudSweepTimer, this, &AGOTAPlayerCharacter::ReconcileScreenHuds, 0.5f, true, 0.25f);
	}
}

void AGOTAPlayerCharacter::ReconcileScreenHuds()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Mine = PC && PC->IsLocalController() ? PC->GetPawn() : nullptr;
	if (Mine)
	{
		const bool bDuel = World->GetGameState() && World->GetGameState()->IsA<ADuelGameState>();
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), false);
		UUserWidget* Keep = nullptr;
		if (!bDuel)
		{
			for (UUserWidget* Widget : Widgets)
			{
				if (!Widget || !Widget->IsInViewport() || !Widget->GetClass()->GetName().StartsWith(TEXT("W_PlayerHUD")))
				{
					continue;
				}
				const APawn* Ref = HudPlayerRef(Widget);
				if (Ref == Mine || (!IsValid(Ref) && !Keep))
				{
					Keep = Widget;
				}
			}
		}
		for (UUserWidget* Widget : Widgets)
		{
			if (Widget && Widget != Keep && Widget->IsInViewport() && Widget->GetClass()->GetName().StartsWith(TEXT("W_PlayerHUD")))
			{
				UE_LOG(LogGOTANetGuard, Log, TEXT("HUD: removed extra %s (for %s) from %s's screen"), *Widget->GetName(), *GetNameSafe(HudPlayerRef(Widget)), *Mine->GetName());
				Widget->RemoveFromParent();
			}
		}
	}
	if (--HudSweepsLeft <= 0)
	{
		GetWorldTimerManager().ClearTimer(HudSweepTimer);
	}
}

void AGOTAPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindLocalHud();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RemoteHudRetry);
		World->GetTimerManager().ClearTimer(HudSweepTimer);
	}
	if (RemoteHud)
	{
		RemoteHud->RemoveFromParent();
		RemoteHud = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AGOTAPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureRemoteHud();
}

void AGOTAPlayerCharacter::EnsureRemoteHud()
{
	UWorld* World = GetWorld();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!World || HasAuthority() || RemoteHud || !PC || !PC->IsLocalController())
	{
		return;
	}
	// The mode is only known once the GameState has arrived. Duel has its own health bars.
	const AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		World->GetTimerManager().SetTimer(RemoteHudRetry, this, &AGOTAPlayerCharacter::EnsureRemoteHud, 0.25f, false);
		return;
	}
	if (GameState->IsA<ADuelGameState>())
	{
		return;
	}
	UClass* HudClass = LoadObject<UClass>(nullptr, TEXT("/Game/Widgets/W_PlayerHUD.W_PlayerHUD_C"));
	RemoteHud = HudClass ? CreateWidget<UUserWidget>(PC, HudClass) : nullptr;
	if (RemoteHud)
	{
		RemoteHud->AddToViewport();
		BindLocalHud();
	}
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
			const APawn* Ref = HudPlayerRef(Widget);
			if (!Ref || Ref == this)
			{
				Hud = Widget;
			}
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

float AGOTAPlayerCharacter::ReadHudHealthBar() const
{
	// Looked up each time: on a remote client the widget can appear after BeginPlay, so HudBar may never have been bound.
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return -1.f;
	}
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(const_cast<AGOTAPlayerCharacter*>(this), Widgets, UUserWidget::StaticClass(), false);
	UUserWidget* Hud = nullptr;
	int32 AnyHud = 0;
	for (UUserWidget* Widget : Widgets)
	{
		if (!Widget || !Widget->GetClass()->GetName().StartsWith(TEXT("W_PlayerHUD")))
		{
			continue;
		}
		++AnyHud;
		if (Widget->IsInViewport() && Widget->GetOwningPlayer() == PC)
		{
			const APawn* Ref = HudPlayerRef(Widget);
			if (IsValid(Ref) && Ref != this)
			{
				return -4.f;	// another player's HUD is on this screen
			}
			Hud = Widget;
		}
	}
	if (!Hud)
	{
		// -2: this player has no such HUD at all. -3: one exists but is not on this player's screen.
		return AnyHud > 0 ? -3.f : -2.f;
	}
	UFunction* GetPercent = Hud->FindFunction(TEXT("GetPercent"));
	const FProperty* ReturnProp = GetPercent ? GetPercent->GetReturnProperty() : nullptr;
	if (!ReturnProp)
	{
		return -1.f;
	}
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(GetPercent->ParmsSize));
	FMemory::Memzero(Params, GetPercent->ParmsSize);
	Hud->ProcessEvent(GetPercent, Params);
	const void* Value = ReturnProp->ContainerPtrToValuePtr<void>(Params);
	return ReturnProp->IsA<FDoubleProperty>() ? static_cast<float>(*static_cast<const double*>(Value)) : *static_cast<const float*>(Value);
}

void AGOTAPlayerCharacter::HandleHudHealth(float NewValue, float MaxValue)
{
	if (UProgressBar* Bar = HudBar.Get())
	{
		const GOTAGAS::FDisplayVitals Vitals = GOTAGAS::ReadDisplayVitals(this);
		if (Vitals.MaxHealth > KINDA_SMALL_NUMBER)
		{
			Bar->SetPercent(Vitals.GetHealthFraction());
			return;
		}
		Bar->SetPercent(MaxValue > KINDA_SMALL_NUMBER ? FMath::Clamp(NewValue / MaxValue, 0.f, 1.f) : 0.f);
	}
}
