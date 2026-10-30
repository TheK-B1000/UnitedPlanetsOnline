// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GOTAFrontendGameMode.generated.h"

/**
 * Menu-only PlayerController: cursor on, no pawn input. UI input mode is owned by
 * UGOTAMenuSubsystem::PresentFrontendUI.
 */
UCLASS()
class WAROFGODS_API AGOTAFrontendPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGOTAFrontendPlayerController();
	virtual void BeginPlay() override;
};

/**
 * World Settings override for L_GOTA_MainMenu ONLY (never a global default, so no gameplay
 * map can load with it). No pawn; presents the frontend; keeps the LAN lobby projection fresh
 * when the host is listening in the menu map.
 */
UCLASS()
class WAROFGODS_API AGOTAFrontendGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGOTAFrontendGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Host + 3 allies. */
	UPROPERTY(EditDefaultsOnly, Category = "GOTA|Frontend")
	int32 MaxLobbyPlayers = 4;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

protected:
	void RefreshLobby() const;
};
