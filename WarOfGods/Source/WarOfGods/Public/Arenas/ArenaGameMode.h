// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Arenas/ArenaTypes.h"
#include "ArenaGameMode.generated.h"

class AArenaGrayboxBuilder;
class AArenaEncounterDirector;
class AArenaPlayerStart;
class APawn;

/**
 * 4-player arena mode. Builds graybox on BeginPlay.
 * Soft-loads BP_Player / BP_Enemy_Melee / WarOfGods PlayerController when present.
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

	/** Host + 3 friends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	int32 SoftMaxPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Classes")
	TSoftClassPtr<APawn> SoftPlayerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Classes")
	TSoftClassPtr<APlayerController> SoftPlayerControllerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Classes")
	TSoftClassPtr<APawn> SoftEnemyClass;

	UFUNCTION(BlueprintCallable, Category = "Arena")
	void SetGrayboxLayout(EArenaGrayboxLayout NewLayout);

	/** Actor tag placed by Tools/GOTA_BuildArenaLevels.py on every authored arena level. */
	static const FName AuthoredArenaTag;

	/** True when the loaded level is an authored arena (its own floors, starts and baked navmesh). */
	static bool IsAuthoredArenaWorld(const UWorld* World);

	/** Authored level for an arena (/Game/Arenas/Maps/L_Arena_<Name>), or empty if none exists. */
	static FString GetAuthoredArenaMap(EArenaId Arena);

protected:
	void ResolveSoftClasses();
	void EnsureGraybox();
	void EnsureEncounterDirector();
	AArenaPlayerStart* FindStartForSlot(int32 Slot) const;

	UPROPERTY()
	TObjectPtr<AArenaGrayboxBuilder> SpawnedGraybox;

	UPROPERTY()
	TObjectPtr<AArenaEncounterDirector> EncounterDirector;

	int32 NextPlayerSlot = 0;
};
