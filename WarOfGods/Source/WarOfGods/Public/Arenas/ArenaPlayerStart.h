// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaPlayerStart.generated.h"

UCLASS()
class WAROFGODS_API AArenaPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	AArenaPlayerStart(const FObjectInitializer& ObjectInitializer);

	/** 0-3 for 4-player arenas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	int32 PlayerSlot = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	EBiomeId PreferredBiome = EBiomeId::Coliseum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	bool bUseForPvPTeamA = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	bool bUseForPvPTeamB = true;
};
