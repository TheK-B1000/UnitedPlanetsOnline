// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "GOTACombatTelemetry.generated.h"

class AAIController;
class UAnimInstance;
class UAnimMontage;

/**
 * Per-enemy record for combat telemetry (server, development builds only). Added by UGOTACombatTelemetry to AI
 * combatants. No tick: montage and move-completed events, plus the subsystem's 1 Hz sample.
 */
UCLASS(NotBlueprintable)
class WAROFGODS_API UGOTAEnemyTelemetryProbe : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTAEnemyTelemetryProbe();

	FString Role = TEXT("AI");
	double SpawnTime = 0.0;
	double FirstDamagedTime = -1.0;
	float DamageTaken = 0.f;
	int32 Attacks = 0;
	int32 AttackHits = 0;
	int32 AttackBlocked = 0;
	int32 AttacksInterrupted = 0;
	float DamageDealt = 0.f;
	int32 TargetSwitches = 0;
	int32 MoveFailures = 0;
	int32 Stalls = 0;
	bool bDead = false;

	/** Bind montage and move delegates once the anim instance / controller exist (cheap, idempotent). */
	void TryBind();
	/** 1 Hz: stall detection (alive, has a brain, far from every player, not moving, not attacking). */
	void Sample(double Now);
	/** A player lost health to this enemy (or took a zero-damage hit from it). */
	void NoteHitOnPlayer(APawn* Victim, float Damage);
	void NoteDamaged(float Amount, double Now);

	UFUNCTION()
	void HandleMontageStarted(UAnimMontage* Montage);
	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TWeakObjectPtr<UAnimInstance> BoundAnim;
	TWeakObjectPtr<AAIController> BoundController;
	TWeakObjectPtr<UAnimMontage> CurrentAttack;
	TWeakObjectPtr<APawn> CurrentTarget;
	TWeakObjectPtr<APawn> LastTarget;
	double AttackStart = 0.0;
	float AttackDist = 0.f;
	float AttackDamage = 0.f;
	bool bAttackHit = false;
	bool bAttackBlocked = false;
	double StillSince = -1.0;
	bool bStallLogged = false;
	void FinishAttack(const TCHAR* Result);
};

/**
 * Development-only, server-side combat telemetry for Horde balance (`gota.Telemetry`, default 1; compiled out of
 * Shipping). Event-driven: one log line per enemy attack, hit on a player, death, respawn, wave start / end, boss attack,
 * movement failure or stall, plus a 1 Hz sample for simultaneous attackers and stalls. Never per frame.
 * Lines start with "TELEM "; Tools/GOTA_HordeTelemetry.py turns a host log into a report.
 */
UCLASS()
class WAROFGODS_API UGOTACombatTelemetry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The world's telemetry when enabled and this is the server (listen or dedicated); otherwise null. */
	static UGOTACombatTelemetry* Get(const UObject* WorldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Combat bootstrap: players start a life, AI gets a probe. */
	void RegisterCombatant(APawn* Pawn);
	void NoteEnemySpawned(APawn* Enemy, const TCHAR* Role);
	/** Server health mirror changed (UStatsComponent). Attacker is the pawn whose damage response came with it, if any. */
	void NoteHealthChange(AActor* Victim, float Before, float After, APawn* Attacker, int32 Response);
	/** A damage response that changed no health (blocked, invincible, i-frames). */
	void NoteNoDamageResponse(AActor* Victim, APawn* Attacker, int32 Response);
	void NoteBossAttack(APawn* Boss, FName AttackId, APawn* Target, float Distance);
	void NoteBossAttackEnd(APawn* Boss, FName AttackId, bool bHit);
	void NoteWaveStart(int32 Wave, bool bBoss, int32 Players, int32 Planned);
	void NoteWaveEnd(const TCHAR* Outcome);
	void NoteMatchEnd(const TCHAR* Outcome);
	/** Log the running summary (per player, per enemy class, per wave). */
	void DumpSummary(const TCHAR* Why) const;

	static FString PlayerLabel(const APawn* Pawn);
	static bool IsPlayerPawn(const AActor* Actor);
	/** Living player pawns. */
	void GetPlayers(TArray<APawn*>& Out) const;
	double Now() const;
	int32 CurrentWave() const { return Wave; }

	struct FClassStats
	{
		int32 Spawned = 0, Killed = 0, Attacks = 0, Hits = 0, Blocked = 0, Interrupted = 0, MoveFailures = 0, Stalls = 0;
		float Damage = 0.f;
		double SumTimeToKill = 0.0;
		int32 TimeToKillSamples = 0;
	};
	struct FPlayerStats
	{
		int32 HitsTaken = 0, NoDamageHits = 0, Deaths = 0, Lives = 0, MaxAttackers = 0;
		float DamageTaken = 0.f, Healed = 0.f;
		double LifeStart = -1.0, SumLife = 0.0;
		TMap<TWeakObjectPtr<APawn>, double> RecentAttackers;
	};
	FClassStats& ClassStats(const APawn* Enemy);
	FPlayerStats& PlayerStats(const APawn* Player);
	int32 CountRecentAttackers(FPlayerStats& Stats, double At) const;

private:
	void Sample();

	TMap<FString, FClassStats> Classes;
	TMap<FString, FPlayerStats> Players;
	TArray<TWeakObjectPtr<UGOTAEnemyTelemetryProbe>> Probes;
	TMap<TWeakObjectPtr<APawn>, TWeakObjectPtr<APawn>> BossTargets;
	FTimerHandle SampleTimer;

	int32 Wave = 0;
	bool bWaveBoss = false;
	bool bWaveOpen = false;
	double WaveStart = 0.0;
	int32 WavePlanned = 0, WaveKills = 0, WaveDeaths = 0, WaveHits = 0, WaveMaxEngaged = 0;
	float WaveDamageToPlayers = 0.f;
};
