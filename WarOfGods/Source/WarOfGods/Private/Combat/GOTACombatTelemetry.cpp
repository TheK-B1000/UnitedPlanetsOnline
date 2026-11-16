// White Tiger Studios 2024

#include "Combat/GOTACombatTelemetry.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Bosses/GOTABossComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTACombatTelemetry)

DEFINE_LOG_CATEGORY_STATIC(LogGOTATelemetry, Log, All);

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarGOTATelemetry(
	TEXT("gota.Telemetry"),
	1,
	TEXT("Server combat telemetry for balance (TELEM log lines: enemy attacks, hits on players, deaths, waves). 0 = off. Not in Shipping."),
	ECVF_Default);

static FAutoConsoleCommandWithWorld GOTATelemetryDumpCmd(
	TEXT("gota.Telemetry.Dump"),
	TEXT("Log the combat telemetry summary so far (server)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (const UGOTACombatTelemetry* Telemetry = UGOTACombatTelemetry::Get(World))
		{
			Telemetry->DumpSummary(TEXT("console"));
		}
	}));
#endif

namespace
{
	constexpr double AttackerWindowSeconds = 5.0;
	constexpr float EngagedRadius = 400.f;
	constexpr float StallDistance = 300.f;
	constexpr double StallSeconds = 8.0;

	FString ClassLabel(const UObject* Object)
	{
		FString Name = Object ? Object->GetClass()->GetName() : FString(TEXT("None"));
		Name.RemoveFromEnd(TEXT("_C"));
		return Name;
	}

	const TCHAR* MoveResultName(EPathFollowingResult::Type Result)
	{
		switch (Result)
		{
		case EPathFollowingResult::Success: return TEXT("Success");
		case EPathFollowingResult::Blocked: return TEXT("Blocked");
		case EPathFollowingResult::OffPath: return TEXT("OffPath");
		case EPathFollowingResult::Aborted: return TEXT("Aborted");
		case EPathFollowingResult::Invalid: return TEXT("Invalid");
		default: return TEXT("Other");
		}
	}

	APawn* NearestLivingPlayer(const UGOTACombatTelemetry& Telemetry, const FVector& From, float* OutDist = nullptr)
	{
		TArray<APawn*> Players;
		Telemetry.GetPlayers(Players);
		APawn* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (APawn* P : Players)
		{
			const float D = FVector::Dist2D(P->GetActorLocation(), From);
			if (D < BestDist)
			{
				BestDist = D;
				Best = P;
			}
		}
		if (OutDist)
		{
			*OutDist = Best ? BestDist : -1.f;
		}
		return Best;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Probe

UGOTAEnemyTelemetryProbe::UGOTAEnemyTelemetryProbe()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UGOTAEnemyTelemetryProbe::TryBind()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}
	// The enemy's hero body can swap its anim class after spawn: follow the current instance.
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (Anim != BoundAnim.Get())
	{
		if (UAnimInstance* Old = BoundAnim.Get())
		{
			Old->OnMontageStarted.RemoveDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMontageStarted);
			Old->OnMontageEnded.RemoveDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMontageEnded);
		}
		BoundAnim = Anim;
		if (Anim)
		{
			Anim->OnMontageStarted.AddUniqueDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMontageStarted);
			Anim->OnMontageEnded.AddUniqueDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMontageEnded);
		}
	}
	AAIController* AIC = Cast<AAIController>(Pawn->GetController());
	if (AIC != BoundController.Get())
	{
		if (AAIController* Old = BoundController.Get())
		{
			Old->ReceiveMoveCompleted.RemoveDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMoveCompleted);
		}
		BoundController = AIC;
		if (AIC)
		{
			AIC->ReceiveMoveCompleted.AddUniqueDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMoveCompleted);
		}
	}
}

void UGOTAEnemyTelemetryProbe::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAnimInstance* Anim = BoundAnim.Get())
	{
		Anim->OnMontageStarted.RemoveDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMontageStarted);
		Anim->OnMontageEnded.RemoveDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMontageEnded);
	}
	if (AAIController* AIC = BoundController.Get())
	{
		AIC->ReceiveMoveCompleted.RemoveDynamic(this, &UGOTAEnemyTelemetryProbe::HandleMoveCompleted);
	}
	Super::EndPlay(EndPlayReason);
}

void UGOTAEnemyTelemetryProbe::HandleMontageStarted(UAnimMontage* Montage)
{
	UGOTACombatTelemetry* Telemetry = UGOTACombatTelemetry::Get(this);
	APawn* Pawn = Cast<APawn>(GetOwner());
	// Bosses report their attacks from UGOTABossComponent; only montages with a damage notify are attacks
	// (drawing and sheathing the sword carry a notify too, but are not attacks).
	if (!Telemetry || !Pawn || bDead || !Montage || Pawn->FindComponentByClass<UGOTABossComponent>()
		|| Montage->GetName().Contains(TEXT("Sheath")) || FGOTABlueprintCombat::FindNextImpactTime(Montage, 0.f) < 0.f)
	{
		return;
	}
	if (CurrentAttack.IsValid())
	{
		FinishAttack(TEXT("chained"));
	}
	// The brain's focus is its target when it has one; otherwise the nearest player.
	APawn* Target = nullptr;
	if (const AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
	{
		Target = Cast<APawn>(AIC->GetFocusActor());
	}
	if (!Target || !UGOTACombatTelemetry::IsPlayerPawn(Target))
	{
		Target = NearestLivingPlayer(*Telemetry, Pawn->GetActorLocation());
	}
	if (LastTarget.IsValid() && Target && Target != LastTarget.Get())
	{
		++TargetSwitches;
	}
	LastTarget = Target;
	CurrentTarget = Target;
	CurrentAttack = Montage;
	AttackStart = Telemetry->Now();
	AttackDist = Target ? FVector::Dist2D(Target->GetActorLocation(), Pawn->GetActorLocation()) : -1.f;
	AttackDamage = 0.f;
	bAttackHit = false;
	bAttackBlocked = false;
	++Attacks;
	++Telemetry->ClassStats(Pawn).Attacks;
}

void UGOTAEnemyTelemetryProbe::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!Montage || Montage != CurrentAttack.Get())
	{
		return;
	}
	FinishAttack(bAttackHit ? TEXT("hit") : (bAttackBlocked ? TEXT("nodamage") : (bInterrupted ? TEXT("interrupted") : TEXT("miss"))));
}

void UGOTAEnemyTelemetryProbe::FinishAttack(const TCHAR* Result)
{
	UGOTACombatTelemetry* Telemetry = UGOTACombatTelemetry::Get(this);
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Telemetry && Pawn)
	{
		if (bAttackHit)
		{
			++AttackHits;
		}
		else if (bAttackBlocked)
		{
			++AttackBlocked;
		}
		else if (FCString::Strcmp(Result, TEXT("interrupted")) == 0)
		{
			++AttacksInterrupted;
			++Telemetry->ClassStats(Pawn).Interrupted;
		}
		UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM attack enemy=%s class=%s montage=%s target=%s dist=%.0f result=%s dmg=%.1f dur=%.2f wave=%d"),
			*GetNameSafe(Pawn), *ClassLabel(Pawn), *GetNameSafe(CurrentAttack.Get()), *UGOTACombatTelemetry::PlayerLabel(CurrentTarget.Get()),
			AttackDist, Result, AttackDamage, Telemetry->Now() - AttackStart, Telemetry->CurrentWave());
	}
	CurrentAttack.Reset();
	CurrentTarget.Reset();
}

void UGOTAEnemyTelemetryProbe::NoteHitOnPlayer(APawn* Victim, float Damage)
{
	if (Damage > 0.f)
	{
		DamageDealt += Damage;
		if (CurrentAttack.IsValid())
		{
			bAttackHit = true;
			AttackDamage += Damage;
		}
	}
	else if (CurrentAttack.IsValid())
	{
		bAttackBlocked = true;
	}
}

void UGOTAEnemyTelemetryProbe::NoteDamaged(float Amount, double Now)
{
	if (FirstDamagedTime < 0.0)
	{
		FirstDamagedTime = Now;
	}
	DamageTaken += Amount;
}

void UGOTAEnemyTelemetryProbe::HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	// Aborted is the tree replanning; Success is normal. Everything else is a movement failure.
	if (Result == EPathFollowingResult::Success || Result == EPathFollowingResult::Aborted)
	{
		return;
	}
	++MoveFailures;
	UGOTACombatTelemetry* Telemetry = UGOTACombatTelemetry::Get(this);
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Telemetry || !Pawn)
	{
		return;
	}
	++Telemetry->ClassStats(Pawn).MoveFailures;
	if (MoveFailures <= 3 || MoveFailures % 10 == 0)
	{
		float Dist = -1.f;
		NearestLivingPlayer(*Telemetry, Pawn->GetActorLocation(), &Dist);
		UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM move_fail enemy=%s class=%s result=%s count=%d nearestPlayer=%.0f wave=%d"),
			*GetNameSafe(Pawn), *ClassLabel(Pawn), MoveResultName(Result), MoveFailures, Dist, Telemetry->CurrentWave());
	}
}

void UGOTAEnemyTelemetryProbe::Sample(double Now)
{
	TryBind();
	APawn* Pawn = Cast<APawn>(GetOwner());
	UGOTACombatTelemetry* Telemetry = UGOTACombatTelemetry::Get(this);
	if (!Pawn || !Telemetry || bDead || UGOTACombatantObserver::IsCombatantDefeated(Pawn))
	{
		return;
	}
	float Dist = -1.f;
	if (!NearestLivingPlayer(*Telemetry, Pawn->GetActorLocation(), &Dist))
	{
		StillSince = -1.0;
		return;
	}
	const bool bStill = Dist > StallDistance && Pawn->GetVelocity().Size2D() < 10.f && !CurrentAttack.IsValid();
	if (!bStill)
	{
		StillSince = -1.0;
		bStallLogged = false;
		return;
	}
	if (StillSince < 0.0)
	{
		StillSince = Now;
	}
	else if (!bStallLogged && Now - StillSince >= StallSeconds)
	{
		bStallLogged = true;
		++Stalls;
		++Telemetry->ClassStats(Pawn).Stalls;
		const AAIController* AIC = Cast<AAIController>(Pawn->GetController());
		UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM stall enemy=%s class=%s nearestPlayer=%.0f for=%.0fs controller=%s focus=%s wave=%d"),
			*GetNameSafe(Pawn), *ClassLabel(Pawn), Dist, Now - StillSince, *GetNameSafe(AIC),
			*GetNameSafe(AIC ? AIC->GetFocusActor() : nullptr), Telemetry->CurrentWave());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Subsystem

UGOTACombatTelemetry* UGOTACombatTelemetry::Get(const UObject* WorldContext)
{
#if UE_BUILD_SHIPPING
	return nullptr;
#else
	if (CVarGOTATelemetry.GetValueOnGameThread() == 0 || !WorldContext || !GEngine)
	{
		return nullptr;
	}
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	return World->GetSubsystem<UGOTACombatTelemetry>();
#endif
}

bool UGOTACombatTelemetry::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
#endif
}

void UGOTACombatTelemetry::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() != NM_Client)
	{
		InWorld.GetTimerManager().SetTimer(SampleTimer, FTimerDelegate::CreateUObject(this, &UGOTACombatTelemetry::Sample), 1.f, true);
	}
}

void UGOTACombatTelemetry::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SampleTimer);
	}
	Super::Deinitialize();
}

double UGOTACombatTelemetry::Now() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

bool UGOTACombatTelemetry::IsPlayerPawn(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Cast<APlayerController>(Pawn->GetController()) != nullptr;
}

FString UGOTACombatTelemetry::PlayerLabel(const APawn* Pawn)
{
	if (!Pawn)
	{
		return TEXT("none");
	}
	if (const APlayerState* PS = Pawn->GetPlayerState())
	{
		return PS->GetPlayerName().Replace(TEXT(" "), TEXT("_"));
	}
	return Pawn->GetName();
}

void UGOTACombatTelemetry::GetPlayers(TArray<APawn*>& Out) const
{
	Out.Reset();
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
			if (IsValid(Pawn) && !UGOTACombatantObserver::IsCombatantDefeated(Pawn))
			{
				Out.Add(Pawn);
			}
		}
	}
}

UGOTACombatTelemetry::FClassStats& UGOTACombatTelemetry::ClassStats(const APawn* Enemy)
{
	return Classes.FindOrAdd(ClassLabel(Enemy));
}

UGOTACombatTelemetry::FPlayerStats& UGOTACombatTelemetry::PlayerStats(const APawn* Player)
{
	return Players.FindOrAdd(PlayerLabel(Player));
}

int32 UGOTACombatTelemetry::CountRecentAttackers(FPlayerStats& Stats, double At) const
{
	int32 Count = 0;
	for (auto It = Stats.RecentAttackers.CreateIterator(); It; ++It)
	{
		const APawn* Attacker = It.Key().Get();
		if (!Attacker || At - It.Value() > AttackerWindowSeconds || UGOTACombatantObserver::IsCombatantDefeated(Attacker))
		{
			It.RemoveCurrent();
			continue;
		}
		++Count;
	}
	return Count;
}

void UGOTACombatTelemetry::RegisterCombatant(APawn* Pawn)
{
	if (!IsValid(Pawn) || !Pawn->HasAuthority())
	{
		return;
	}
	if (IsPlayerPawn(Pawn))
	{
		// A player pawn is first seen at spawn, before possession, and was given a probe then: undo that.
		if (UGOTAEnemyTelemetryProbe* Probe = Pawn->FindComponentByClass<UGOTAEnemyTelemetryProbe>())
		{
			FClassStats& Stats = ClassStats(Pawn);
			Stats.Spawned = FMath::Max(0, Stats.Spawned - 1);
			Probes.Remove(Probe);
			Probe->DestroyComponent();
		}
		FPlayerStats& Stats = PlayerStats(Pawn);
		if (Stats.LifeStart < 0.0)
		{
			Stats.LifeStart = Now();
			++Stats.Lives;
			UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM life player=%s pawn=%s life=%d wave=%d"), *PlayerLabel(Pawn), *Pawn->GetName(), Stats.Lives, Wave);
		}
		return;
	}
	if (Pawn->FindComponentByClass<UGOTAEnemyTelemetryProbe>())
	{
		return;
	}
	UGOTAEnemyTelemetryProbe* Probe = NewObject<UGOTAEnemyTelemetryProbe>(Pawn, TEXT("GOTA_TelemetryProbe"));
	Probe->SpawnTime = Now();
	Probe->RegisterComponent();
	Probe->TryBind();
	Probes.Add(Probe);
	++ClassStats(Pawn).Spawned;
}

void UGOTACombatTelemetry::NoteEnemySpawned(APawn* Enemy, const TCHAR* Role)
{
	if (!IsValid(Enemy))
	{
		return;
	}
	RegisterCombatant(Enemy);
	if (UGOTAEnemyTelemetryProbe* Probe = Enemy->FindComponentByClass<UGOTAEnemyTelemetryProbe>())
	{
		Probe->Role = Role;
		Probe->TryBind(); // the director possesses it after the spawn event
	}
	float Dist = -1.f;
	NearestLivingPlayer(*this, Enemy->GetActorLocation(), &Dist);
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM spawn enemy=%s class=%s role=%s controller=%s nearestPlayer=%.0f wave=%d"),
		*Enemy->GetName(), *ClassLabel(Enemy), Role, *GetNameSafe(Enemy->GetController()), Dist, Wave);
}

void UGOTACombatTelemetry::NoteHealthChange(AActor* Victim, float Before, float After, APawn* Attacker, int32 Response)
{
	APawn* VictimPawn = Cast<APawn>(Victim);
	if (!VictimPawn)
	{
		return;
	}
	const double T = Now();
	const float Damage = Before - After;
	if (IsPlayerPawn(VictimPawn))
	{
		FPlayerStats& Stats = PlayerStats(VictimPawn);
		if (Damage <= 0.f)
		{
			if (Before > 0.f)
			{
				Stats.Healed += -Damage;
				UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM heal player=%s hp=%.0f->%.0f wave=%d"), *PlayerLabel(VictimPawn), Before, After, Wave);
			}
			return;
		}
		const bool bByEnemy = Attacker && !IsPlayerPawn(Attacker);
		Stats.DamageTaken += Damage;
		++Stats.HitsTaken;
		++WaveHits;
		WaveDamageToPlayers += Damage;
		if (bByEnemy)
		{
			Stats.RecentAttackers.Add(Attacker, T);
			FClassStats& Class = ClassStats(Attacker);
			++Class.Hits;
			Class.Damage += Damage;
			if (UGOTAEnemyTelemetryProbe* Probe = Attacker->FindComponentByClass<UGOTAEnemyTelemetryProbe>())
			{
				Probe->NoteHitOnPlayer(VictimPawn, Damage);
			}
		}
		const int32 Attackers = CountRecentAttackers(Stats, T);
		Stats.MaxAttackers = FMath::Max(Stats.MaxAttackers, Attackers);
		UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM hit player=%s by=%s class=%s dmg=%.1f hp=%.0f->%.0f resp=%d attackers=%d wave=%d"),
			*PlayerLabel(VictimPawn), *GetNameSafe(Attacker), Attacker ? *ClassLabel(Attacker) : TEXT("unknown"), Damage, Before, After,
			Response, Attackers, Wave);
		if (After <= 0.f && Before > 0.f)
		{
			++Stats.Deaths;
			++WaveDeaths;
			const double Life = Stats.LifeStart >= 0.0 ? T - Stats.LifeStart : -1.0;
			if (Life >= 0.0)
			{
				Stats.SumLife += Life;
			}
			Stats.LifeStart = -1.0;
			UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM death player=%s by=%s class=%s life=%.1f attackers=%d wave=%d waveTime=%.1f"),
				*PlayerLabel(VictimPawn), *GetNameSafe(Attacker), Attacker ? *ClassLabel(Attacker) : TEXT("unknown"), Life, Attackers, Wave,
				bWaveOpen ? T - WaveStart : -1.0);
		}
		return;
	}

	// An enemy.
	UGOTAEnemyTelemetryProbe* Probe = VictimPawn->FindComponentByClass<UGOTAEnemyTelemetryProbe>();
	if (!Probe || Damage <= 0.f)
	{
		return;
	}
	Probe->NoteDamaged(Damage, T);
	if (After <= 0.f && Before > 0.f && !Probe->bDead)
	{
		Probe->bDead = true;
		++WaveKills;
		FClassStats& Class = ClassStats(VictimPawn);
		++Class.Killed;
		const double Engaged = T - Probe->FirstDamagedTime;
		Class.SumTimeToKill += Engaged;
		++Class.TimeToKillSamples;
		UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM enemy_death enemy=%s class=%s role=%s ttk=%.1f alive=%.1f dmgTaken=%.0f attacks=%d hits=%d dealt=%.0f switches=%d killer=%s wave=%d"),
			*VictimPawn->GetName(), *ClassLabel(VictimPawn), *Probe->Role, Engaged, T - Probe->SpawnTime, Probe->DamageTaken, Probe->Attacks,
			Probe->AttackHits, Probe->DamageDealt, Probe->TargetSwitches, *PlayerLabel(Attacker), Wave);
	}
}

void UGOTACombatTelemetry::NoteNoDamageResponse(AActor* Victim, APawn* Attacker, int32 Response)
{
	APawn* VictimPawn = Cast<APawn>(Victim);
	if (!VictimPawn || !IsPlayerPawn(VictimPawn) || !Attacker || IsPlayerPawn(Attacker))
	{
		return;
	}
	++PlayerStats(VictimPawn).NoDamageHits;
	++ClassStats(Attacker).Blocked;
	if (UGOTAEnemyTelemetryProbe* Probe = Attacker->FindComponentByClass<UGOTAEnemyTelemetryProbe>())
	{
		Probe->NoteHitOnPlayer(VictimPawn, 0.f);
	}
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM nodamage player=%s by=%s class=%s resp=%d wave=%d"),
		*PlayerLabel(VictimPawn), *Attacker->GetName(), *ClassLabel(Attacker), Response, Wave);
}

void UGOTACombatTelemetry::NoteBossAttack(APawn* Boss, FName AttackId, APawn* Target, float Distance)
{
	if (!Boss)
	{
		return;
	}
	TWeakObjectPtr<APawn>& Last = BossTargets.FindOrAdd(Boss);
	const bool bSwitched = Last.IsValid() && Last.Get() != Target;
	Last = Target;
	++ClassStats(Boss).Attacks;
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM boss_attack boss=%s attack=%s target=%s dist=%.0f switched=%d wave=%d"),
		*Boss->GetName(), *AttackId.ToString(), *PlayerLabel(Target), Distance, bSwitched ? 1 : 0, Wave);
}

void UGOTACombatTelemetry::NoteBossAttackEnd(APawn* Boss, FName AttackId, bool bHit)
{
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM boss_attack_end boss=%s attack=%s result=%s wave=%d"),
		*GetNameSafe(Boss), *AttackId.ToString(), bHit ? TEXT("hit") : TEXT("miss"), Wave);
}

void UGOTACombatTelemetry::NoteWaveStart(int32 InWave, bool bBoss, int32 InPlayers, int32 Planned)
{
	if (bWaveOpen)
	{
		NoteWaveEnd(TEXT("superseded"));
	}
	Wave = InWave;
	bWaveBoss = bBoss;
	bWaveOpen = true;
	WaveStart = Now();
	WavePlanned = Planned;
	WaveKills = WaveDeaths = WaveHits = WaveMaxEngaged = 0;
	WaveDamageToPlayers = 0.f;
	TArray<APawn*> Living;
	GetPlayers(Living);
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM wave_start wave=%d boss=%d players=%d living=%d planned=%d"),
		Wave, bBoss ? 1 : 0, InPlayers, Living.Num(), Planned);
}

void UGOTACombatTelemetry::NoteWaveEnd(const TCHAR* Outcome)
{
	if (!bWaveOpen)
	{
		return;
	}
	bWaveOpen = false;
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM wave_end wave=%d boss=%d outcome=%s dur=%.1f planned=%d kills=%d playerDeaths=%d hitsOnPlayers=%d dmgToPlayers=%.0f maxEngaged=%d"),
		Wave, bWaveBoss ? 1 : 0, Outcome, Now() - WaveStart, WavePlanned, WaveKills, WaveDeaths, WaveHits, WaveDamageToPlayers, WaveMaxEngaged);
}

void UGOTACombatTelemetry::NoteMatchEnd(const TCHAR* Outcome)
{
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM match_end outcome=%s wave=%d"), Outcome, Wave);
	DumpSummary(Outcome);
}

void UGOTACombatTelemetry::DumpSummary(const TCHAR* Why) const
{
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM summary_begin why=%s"), Why);
	for (const TPair<FString, FClassStats>& Pair : Classes)
	{
		const FClassStats& S = Pair.Value;
		UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM class name=%s spawned=%d killed=%d attacks=%d hits=%d nodamage=%d interrupted=%d dmg=%.0f avgHit=%.1f avgTtk=%.1f moveFail=%d stalls=%d"),
			*Pair.Key, S.Spawned, S.Killed, S.Attacks, S.Hits, S.Blocked, S.Interrupted, S.Damage, S.Hits > 0 ? S.Damage / S.Hits : 0.f,
			S.TimeToKillSamples > 0 ? S.SumTimeToKill / S.TimeToKillSamples : -1.0, S.MoveFailures, S.Stalls);
	}
	for (const TPair<FString, FPlayerStats>& Pair : Players)
	{
		const FPlayerStats& S = Pair.Value;
		UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM player name=%s lives=%d deaths=%d hitsTaken=%d nodamage=%d dmgTaken=%.0f healed=%.0f avgLife=%.1f maxAttackers=%d"),
			*Pair.Key, S.Lives, S.Deaths, S.HitsTaken, S.NoDamageHits, S.DamageTaken, S.Healed, S.Deaths > 0 ? S.SumLife / S.Deaths : -1.0, S.MaxAttackers);
	}
	UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM summary_end"));
}

void UGOTACombatTelemetry::Sample()
{
#if !UE_BUILD_SHIPPING
	if (CVarGOTATelemetry.GetValueOnGameThread() == 0)
	{
		return;
	}
#endif
	const double T = Now();
	TArray<APawn*> Living;
	GetPlayers(Living);
	for (int32 Index = Probes.Num() - 1; Index >= 0; --Index)
	{
		UGOTAEnemyTelemetryProbe* Probe = Probes[Index].Get();
		if (!Probe || !IsValid(Probe->GetOwner()))
		{
			Probes.RemoveAtSwap(Index);
			continue;
		}
		Probe->Sample(T);
	}
	// Engaged = living enemies within EngagedRadius of a player; the wave keeps the worst moment.
	for (const APawn* Player : Living)
	{
		int32 Engaged = 0;
		for (const TWeakObjectPtr<UGOTAEnemyTelemetryProbe>& Weak : Probes)
		{
			const UGOTAEnemyTelemetryProbe* Probe = Weak.Get();
			const AActor* Enemy = Probe ? Probe->GetOwner() : nullptr;
			if (Enemy && !Probe->bDead && FVector::Dist2D(Enemy->GetActorLocation(), Player->GetActorLocation()) <= EngagedRadius
				&& !UGOTACombatantObserver::IsCombatantDefeated(Enemy))
			{
				++Engaged;
			}
		}
		if (bWaveOpen && Engaged > WaveMaxEngaged)
		{
			WaveMaxEngaged = Engaged;
			UE_LOG(LogGOTATelemetry, Log, TEXT("TELEM engaged player=%s enemies=%d wave=%d waveTime=%.1f"), *PlayerLabel(Player), Engaged, Wave, T - WaveStart);
		}
	}
}
