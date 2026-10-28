// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Arenas/ArenaTypes.h"
#include "DuelMatchDefinition.generated.h"

/**
 * Data-driven Trial of Champions rules.
 * Conservative defaults: best-of-3, 90s rounds, no mid-round respawn.
 */
UCLASS(BlueprintType)
class WAROFGODS_API UDuelMatchDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel")
	FName ModeId = TEXT("TrialOfChampions");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel")
	FText DisplayName = NSLOCTEXT("GOTA", "TrialOfChampions", "Trial of Champions");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel")
	EArenaId ArenaId = EArenaId::TitansQuarry;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel", meta = (ClampMin = "2", ClampMax = "2"))
	int32 MaxParticipants = 2;

	/** Best-of-N (odd). RoundsToWin = (BestOf/2)+1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel", meta = (ClampMin = "1"))
	int32 BestOfRounds = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel", meta = (ClampMin = "0.0"))
	float CountdownSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel", meta = (ClampMin = "10.0"))
	float RoundDurationSeconds = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel", meta = (ClampMin = "0.0"))
	float RoundEndDelaySeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel", meta = (ClampMin = "0.0"))
	float IntermissionSeconds = 2.f;

	/** On timeout: true = higher observed health wins; false = draw (no score). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel")
	bool bTimeoutAwardHigherHealth = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel")
	bool bFriendlyFire = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel")
	bool bAllowMidRoundRespawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duel")
	float NormalizedMaxHealth = 100.f;

	int32 GetRoundsToWin() const { return (BestOfRounds / 2) + 1; }
};
