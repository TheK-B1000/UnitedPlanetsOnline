// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaGameMode.generated.h"

class AArenaGrayboxBuilder;
class AArenaEncounterDirector;
class AArenaPlayerStart;

/**
 * 4-player arena mode. Builds graybox layout on BeginPlay when AutoBuildGraybox is set.
 * Preserves DefaultPawnClass from project/BP when assigned in editor.
 */
UCLASS()
class WAROFGODS_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	EArenaId ArenaId = EArenaId::TitansQuarry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	EArenaMode ArenaMode = EArenaMode::Skirmish;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	EArenaGrayboxLayout GrayboxLayout = EArenaGrayboxLayout::TitansQuarry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	bool bAutoBuildGraybox = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	bool bAutoStartEncounterDirector = true;

	/** Story / PvE / skirmish: host + 3 friends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	int32 SoftMaxPlayers = 4;

protected:
	void EnsureGraybox();
	void EnsureEncounterDirector();
	AArenaPlayerStart* FindStartForSlot(int32 Slot) const;

	UPROPERTY()
	TObjectPtr<AArenaGrayboxBuilder> SpawnedGraybox;

	UPROPERTY()
	TObjectPtr<AArenaEncounterDirector> EncounterDirector;

	int32 NextPlayerSlot = 0;
};
