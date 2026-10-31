// White Tiger Studios 2024

#include "Combat/CombatBootstrapSubsystem.h"

#include "Characters/StatsComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/CombatTimingComponent.h"
#include "Combat/CombatTimingTypes.h"
#include "Combat/LockonComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTACombatBootstrap, Log, All);

void UCombatBootstrapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCombatBootstrapSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	LoggingBound.Reset();
	TimingLoggingBound.Reset();
	Super::Deinitialize();
}

void UCombatBootstrapSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &UCombatBootstrapSubsystem::HandleActorSpawned));

	for (FConstPlayerControllerIterator It = InWorld.GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			BindPlayerController(PC);
		}
	}
}

bool UCombatBootstrapSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

void UCombatBootstrapSubsystem::HandleActorSpawned(AActor* Spawned)
{
	if (APlayerController* PC = Cast<APlayerController>(Spawned))
	{
		BindPlayerController(PC);
	}
	else if (APawn* Pawn = Cast<APawn>(Spawned))
	{
		if (Pawn->IsPlayerControlled())
		{
			EnsureComponentsOnPawn(Pawn);
		}
	}
}

void UCombatBootstrapSubsystem::BindPlayerController(APlayerController* PC)
{
	if (!IsValid(PC))
	{
		return;
	}

	PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UCombatBootstrapSubsystem::HandlePawnPossessed);
	if (APawn* Pawn = PC->GetPawn())
	{
		EnsureComponentsOnPawn(Pawn);
	}
}

void UCombatBootstrapSubsystem::HandlePawnPossessed(APawn* OldPawn, APawn* NewPawn)
{
	EnsureComponentsOnPawn(NewPawn);
}

void UCombatBootstrapSubsystem::EnsureComponentsOnPawn(APawn* Pawn)
{
	if (!bAutoAttachComponents || !IsValid(Pawn) || !Pawn->IsPlayerControlled())
	{
		return;
	}

	UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>();
	if (!Combat)
	{
		Combat = NewObject<UCombatComponent>(Pawn, TEXT("GOTA_CombatComponent"));
		Combat->RegisterComponent();
		UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Attached UCombatComponent to %s"), *GetNameSafe(Pawn));
	}

	UCombatTimingComponent* Timing = Pawn->FindComponentByClass<UCombatTimingComponent>();
	if (!Timing)
	{
		Timing = NewObject<UCombatTimingComponent>(Pawn, TEXT("GOTA_CombatTimingComponent"));
		Timing->RegisterComponent();
		UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Attached UCombatTimingComponent to %s"), *GetNameSafe(Pawn));
	}

	if (!Pawn->FindComponentByClass<ULockonComponent>())
	{
		ULockonComponent* Lockon = NewObject<ULockonComponent>(Pawn, TEXT("GOTA_LockonComponent"));
		Lockon->RegisterComponent();
		UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Attached ULockonComponent to %s"), *GetNameSafe(Pawn));
	}

	UStatsComponent* Stats = Pawn->FindComponentByClass<UStatsComponent>();
	if (!Stats)
	{
		Stats = NewObject<UStatsComponent>(Pawn, TEXT("GOTA_StatsComponent"));
		Stats->RegisterComponent();
		UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Attached UStatsComponent to %s"), *GetNameSafe(Pawn));
	}

	if (Stats)
	{
		Stats->SetAuthoritativeMutationsEnabled(bEnableAuthoritativeStats);
	}

	if (bLogCombatEvents && Combat)
	{
		BindCombatLogging(Combat);
	}
	if (bLogCombatEvents && Timing)
	{
		BindTimingLogging(Timing);
	}
}

void UCombatBootstrapSubsystem::BindCombatLogging(UCombatComponent* Combat)
{
	if (!Combat || LoggingBound.Contains(Combat))
	{
		return;
	}
	LoggingBound.Add(Combat);

	Combat->OnAttackStarted.AddDynamic(this, &UCombatBootstrapSubsystem::HandleAttackStarted);
	Combat->OnHitConfirmed.AddDynamic(this, &UCombatBootstrapSubsystem::HandleHitConfirmed);
	Combat->OnBlockSucceeded.AddDynamic(this, &UCombatBootstrapSubsystem::HandleBlockSucceeded);
	Combat->OnParrySucceeded.AddDynamic(this, &UCombatBootstrapSubsystem::HandleParrySucceeded);
	Combat->OnPerfectParry.AddDynamic(this, &UCombatBootstrapSubsystem::HandlePerfectParry);
	Combat->OnPerfectDodge.AddDynamic(this, &UCombatBootstrapSubsystem::HandlePerfectDodge);
}

void UCombatBootstrapSubsystem::BindTimingLogging(UCombatTimingComponent* Timing)
{
	if (!Timing || TimingLoggingBound.Contains(Timing))
	{
		return;
	}
	TimingLoggingBound.Add(Timing);
	Timing->OnDefenseResolved.AddDynamic(this, &UCombatBootstrapSubsystem::HandleDefenseResolved);
}

void UCombatBootstrapSubsystem::HandleAttackStarted(const FCombatActorEvent& Event)
{
	UE_LOG(LogGOTACombatBootstrap, Verbose, TEXT("Event AttackStarted Instigator=%s Target=%s"),
		*GetNameSafe(Event.Instigator.Get()), *GetNameSafe(Event.Target.Get()));
}

void UCombatBootstrapSubsystem::HandleHitConfirmed(const FCombatHitEvent& Event)
{
	UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Event HitConfirmed Attacker=%s Victim=%s Damage=%.1f AttackId=%s"),
		*GetNameSafe(Event.Attacker.Get()), *GetNameSafe(Event.Victim.Get()), Event.Damage, *Event.AttackId.ToString());
}

void UCombatBootstrapSubsystem::HandleBlockSucceeded(const FCombatDefenseEvent& Event)
{
	UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Event BlockSucceeded Defender=%s Attacker=%s"),
		*GetNameSafe(Event.Defender.Get()), *GetNameSafe(Event.Attacker.Get()));
}

void UCombatBootstrapSubsystem::HandleParrySucceeded(const FCombatDefenseEvent& Event)
{
	UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Event ParrySucceeded Defender=%s Attacker=%s Perfect=%d"),
		*GetNameSafe(Event.Defender.Get()), *GetNameSafe(Event.Attacker.Get()), Event.bPerfect ? 1 : 0);
}

void UCombatBootstrapSubsystem::HandlePerfectParry(const FCombatDefenseEvent& Event)
{
	UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Event PerfectParry Defender=%s Attacker=%s"),
		*GetNameSafe(Event.Defender.Get()), *GetNameSafe(Event.Attacker.Get()));
}

void UCombatBootstrapSubsystem::HandlePerfectDodge(const FCombatDefenseEvent& Event)
{
	UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Event PerfectDodge Defender=%s Attacker=%s"),
		*GetNameSafe(Event.Defender.Get()), *GetNameSafe(Event.Attacker.Get()));
}

void UCombatBootstrapSubsystem::HandleDefenseResolved(const FCombatDefenseResolveResult& Result)
{
	UE_LOG(LogGOTACombatBootstrap, Log, TEXT("Event DefenseResolved Outcome=%d CancelDamage=%d Age=%.3f Momentum=+%.1f"),
		static_cast<int32>(Result.Outcome), Result.bCancelDamage ? 1 : 0, Result.DefenseAgeSeconds, Result.DivineMomentumGranted);
}
