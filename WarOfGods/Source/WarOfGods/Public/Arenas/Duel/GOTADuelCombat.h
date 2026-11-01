// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Arenas/Duel/DuelTypes.h"
#include "GOTADuelCombat.generated.h"

class ADuelGameState;
class UAnimMontage;
class UStatsComponent;

/**
 * Trial of Champions — combat layer for the Fight AI duel (Combat Milestone 01).
 *
 * Nothing here deals damage on its own or keeps health. The original Blueprint combat
 * (BP_Player / BP_Enemy_Melee / BPC_Attacks / BPC_DamageSystem) still swings, traces, blocks,
 * parries and applies every point of damage. These components:
 *   - observe the Blueprint outcomes (OnBlocked, health deltas, dodge i-frames, montage notifies),
 *   - award Divine Momentum (C++-owned in UStatsComponent),
 *   - drive the existing enemy Blueprint API (StartBlock, HitResponse, GroundSmashAttack) from
 *     visible, replicated state with a reaction delay — never from hidden player input,
 *   - emit cosmetic cues (sparks, sound, camera impulse, actor-local hit-stop, HUD callouts).
 */

/** Per-difficulty tuning of the Sovereign Champion. See Docs/GOTA_COMBAT_TUNING.md. */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAChampionProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") FText DisplayName;
	/** Spawn-time MaxHealth written into each fighter's own BPC_DamageSystem (before any damage). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float ChallengerMaxHealth = 130.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float ChampionMaxHealth = 260.f;
	/** Delay between a visible player wind-up (replicated Attacking) and the champion's answer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float ReactionSeconds = 0.36f;
	/** Chance the champion answers a visible wind-up at all (guard or sidestep). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float DefendChance = 0.38f;
	/** Share of answers that are sidesteps instead of guards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float SidestepShare = 0.35f;
	/** Written into BP_Enemy_Base::BlockChance (its own reactive block-after-hit roll). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float ReactiveBlockChance = 0.25f;
	/** Shield overload: blocked hits within the window that break the champion's guard. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") int32 GuardOverloadHits = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float GuardOverloadWindowSeconds = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float GuardBreakSeconds = 2.5f;
	/** After a blocked hit the champion tends to keep its guard up against the follow-up swing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float GuardChainChance = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float GuardChainSeconds = 1.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float PhaseTwoHealthFraction = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float PhaseTwoDefendBonus = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float PhaseTwoReactionScale = 0.8f;
	/** Phase II signature attack: telegraphed, unblockable (the Blueprint GroundSmash), dodgeable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float SurgeIntervalSeconds = 14.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel|AI") float SurgeTelegraphSeconds = 1.1f;

	static FGOTAChampionProfile ForDifficulty(EGOTADuelDifficulty Difficulty);
	static EGOTADuelDifficulty ParseDifficulty(const FString& Value, EGOTADuelDifficulty Fallback = EGOTADuelDifficulty::Veteran);
	static FText GetDifficultyName(EGOTADuelDifficulty Difficulty);
};

/** Reflection helpers onto the Blueprint combat (names resolved at runtime; no BP edits). */
struct WAROFGODS_API FGOTABlueprintCombat
{
	static bool GetBool(const UObject* Obj, FName Name, bool bDefault = false);
	static bool SetBool(UObject* Obj, FName Name, bool bValue);
	static bool SetNumber(UObject* Obj, FName Name, double Value);
	/**
	 * Calls a Blueprint function. Object params receive Target; enum params receive the entry whose
	 * display name is EnumEntry (when given); numeric params receive Number; bools stay false.
	 */
	static bool Call(UObject* Obj, FName Function, UObject* Target = nullptr, const FString& EnumEntry = FString(), double Number = 0.0);
	/** Value of a Blueprint user-defined enum entry by display name (e.g. E_DamageResponse "Stagger"). */
	static int64 FindEnumValue(const UEnum* Enum, const FString& DisplayName, int64 Fallback = 0);
	/** Damage through the victim's own BPC_DamageSystem::TakeDamage — the only damage path. */
	static bool ApplyDamage(AActor* Victim, AActor* Causer, float Amount, bool bCanBeBlocked, bool bCanBeParried, const FString& Response);
	/** Spawn-time tuning of the Blueprint health authority's own MaxHealth/Health (before combat). */
	static bool ConfigureHealth(AActor* Fighter, float MaxHealth);
	static UAnimMontage* GetActiveMontage(const AActor* Actor, float* OutPosition = nullptr);
	/** Trigger time of the next PlayMontageNotify (the Blueprint's damage moment) at/after Position. */
	static float FindNextImpactTime(const UAnimMontage* Montage, float Position);
	/** Binds a UFUNCTION to a Blueprint multicast delegate when the parameter layouts match. */
	static bool BindDelegate(UObject* Source, FName DelegateName, UObject* Listener, FName ListenerFunction);
	static void UnbindDelegate(UObject* Source, FName DelegateName, UObject* Listener, FName ListenerFunction);
};

/**
 * On every duel fighter (added by ADuelGameMode on the server; replicated).
 * Challenger: Divine Momentum, perfect parry / perfect dodge detection, Divine Counter.
 * Both: cosmetic impact cues.
 */
UCLASS(ClassGroup = (GOTA), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API UGOTADuelFighterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTADuelFighterComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: called by the GameMode once both fighters exist. */
	void InitFighter(bool bInChampion, APawn* InOpponent);

	UFUNCTION(BlueprintPure, Category = "Duel|Combat")
	float GetMomentum() const { return Momentum; }

	UFUNCTION(BlueprintPure, Category = "Duel|Combat")
	float GetMomentumFraction() const { return MaxMomentum > 0.f ? FMath::Clamp(Momentum / MaxMomentum, 0.f, 1.f) : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Duel|Combat")
	bool CanDivineCounter() const { return Momentum >= DivineCounterCost; }

	UFUNCTION(BlueprintPure, Category = "Duel|Combat")
	bool IsChampion() const { return bIsChampion; }

	/**
	 * Server: a perfect dodge against a boss attack is honoured to the impact it was timed against (the Blueprint
	 * dodge's own i-frames last 0.2 s, shorter than the perfect window). Bosses check this before dealing damage.
	 */
	bool IsDodgeProtected() const;

	UFUNCTION(BlueprintPure, Category = "Duel|Combat")
	APawn* GetOpponent() const { return Opponent.Get(); }

	/** Local input (Q / gamepad Y) → server. */
	UFUNCTION(BlueprintCallable, Category = "Duel|Combat")
	void RequestDivineCounter();

	/** Server: award momentum (stored in UStatsComponent, mirrored here for every peer's HUD). */
	void GrantMomentum(float Amount, const TCHAR* Reason);

	/** Server: broadcast a cosmetic cue to every peer. */
	void EmitCue(EGOTADuelCue Cue, AActor* Target);

	UFUNCTION()
	void HandleBlocked(bool bCanBeParried, AActor* DamageCauser);

	// --- tuning (Docs/GOTA_COMBAT_TUNING.md) ---
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float BlockMomentum = 4.f;
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float PerfectParryMomentum = 20.f;
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float PerfectDodgeMomentum = 15.f;
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float HitLandedMomentum = 3.f;
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float GuardBreakMomentum = 15.f;
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float DivineCounterCost = 50.f;
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float DivineCounterDamage = 25.f;
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float DivineCounterRange = 380.f;
	/** Seconds from the counter's start to its blow (the hero's strike animation connects). Invulnerable throughout. */
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float DivineCounterImpactDelay = 0.3f;
	/** Dodge counts as perfect when started at most this long before the attack's impact notify. */
	UPROPERTY(EditAnywhere, Category = "Duel|Momentum") float PerfectDodgeLeadSeconds = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Duel|Feel") float HitStopBlockSeconds = 0.05f;
	UPROPERTY(EditAnywhere, Category = "Duel|Feel") float HitStopPerfectSeconds = 0.09f;

protected:
	UFUNCTION(Server, Reliable)
	void Server_RequestDivineCounter();

	UFUNCTION(Client, Reliable)
	void Client_CounterDenied(uint8 Reason);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Cue(EGOTADuelCue Cue, AActor* Target, FVector_NetQuantize Location);

	void PlayCueLocally(EGOTADuelCue Cue, AActor* Target, const FVector& Location);
	void ApplyHitStop(AActor* Actor, float Seconds);
	void TryBindLocalInput();
	void TickChallengerServer();
	ADuelGameState* GetDuelGS() const;
	UStatsComponent* FindOrAddStats() const;
	bool IsRoundLive() const;

	UPROPERTY(Replicated)
	bool bIsChampion = false;

	UPROPERTY(Replicated)
	TObjectPtr<APawn> Opponent;

	/** Mirror of the server's UStatsComponent::DivineMomentum for every peer's HUD. */
	UPROPERTY(Replicated)
	float Momentum = 0.f;

	UPROPERTY(Replicated)
	float MaxMomentum = 100.f;

	TWeakObjectPtr<UActorComponent> DamageSystem;
	bool bDelegatesBound = false;
	bool bInputBound = false;
	float LastOwnHealth = -1.f;
	float LastOpponentHealth = -1.f;
	bool bWasInvincible = false;
	bool bSuppressDodgeDetect = false;
	// Perfect-dodge candidate (server)
	bool bDodgeArmed = false;
	double DodgeResolveTime = 0.0;
	float HealthAtDodge = 0.f;
	double CounterInvulnEndTime = 0.0;
	bool bCounterPending = false;
	double DodgeProtectedUntil = 0.0;

	/** Server: the Divine Counter's blow, DivineCounterImpactDelay after it starts. */
	void LandDivineCounter(float Damage);
};

/** Server-only brain layer on the AI champion, on top of AIC_Enemy_Base / BT_Enemy_Melee. */
UCLASS(ClassGroup = (GOTA), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API UGOTAChampionDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTAChampionDirectorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	void InitDirector(APawn* InChallenger, EGOTADuelDifficulty InDifficulty);

	UFUNCTION()
	void HandleChampionBlocked(bool bCanBeParried, AActor* DamageCauser);

	void NotifyDivineCounterReceived();

	int32 GetPhase() const { return Phase; }
	bool IsGuardBroken() const;
	FString DescribeState() const;
	const FGOTAChampionProfile& GetProfile() const { return Profile; }

	/** Test hook: force the next decision (block|sidestep|surge|guardbreak|phase2). */
	void DebugForce(const FString& What);

protected:
	void TickTelegraphs(APawn* Champion);
	void TickReactions(APawn* Champion, APawn* Challenger, double Now);
	void TickPhase(APawn* Champion, double Now);
	void TickSurge(APawn* Champion, APawn* Challenger, double Now);
	void ExecuteDefence(APawn* Champion, APawn* Challenger, bool bSidestep);
	void BreakGuard(APawn* Champion, APawn* Challenger);
	void EnterPhaseTwo(APawn* Champion, APawn* Challenger);
	void BeginSurge(APawn* Champion, APawn* Challenger, double Now);
	void ReleaseSurge();
	void SetBrainPaused(bool bPaused, const TCHAR* Reason);
	void FaceChallenger(APawn* Champion, APawn* Challenger) const;
	bool IsChampionAttacking(const APawn* Champion) const;
	bool IsRoundLive() const;
	void Cue(EGOTADuelCue Cue, AActor* Target);
	float EffectiveDefendChance() const;
	float EffectiveReaction() const;

	FGOTAChampionProfile Profile;
	EGOTADuelDifficulty Difficulty = EGOTADuelDifficulty::Veteran;
	TWeakObjectPtr<APawn> Challenger;
	int32 Phase = 1;
	bool bChallengerWasAttacking = false;
	FName LastFoeMontageKey;
	double GuardChainUntil = 0.0;
	double PendingReactionTime = -1.0;
	bool bPendingSidestep = false;
	TArray<double> BlockTimes;
	double GuardBrokenUntil = 0.0;
	double NextSurgeTime = 0.0;
	double SurgeReleaseTime = -1.0;
	double PhaseTwoReleaseTime = -1.0;
	bool bPhaseTwoSlamPending = false;
	FName LastMontageName;
	FString LastDecision;
	int32 Defences = 0;
	int32 Sidesteps = 0;
	int32 GuardBreaks = 0;
	int32 Surges = 0;
	bool bBound = false;
	FString ForcedDecision;
};
