// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Arenas/ArenaTypes.h"
#include "MenuTypes.generated.h"

/** Frontend screen / camera shot identifiers for the shared cinematic menu level. */
UENUM(BlueprintType)
enum class EGOTAMenuScreen : uint8
{
	None UMETA(DisplayName = "None"),
	MainMenu UMETA(DisplayName = "Main Menu"),
	StoryMode UMETA(DisplayName = "Story Mode"),
	Settings UMETA(DisplayName = "Settings"),
	Multiplayer UMETA(DisplayName = "Multiplayer"),
	Lobby UMETA(DisplayName = "Lobby"),
	ArenaSelection UMETA(DisplayName = "Arena Selection"),
	CharacterSelection UMETA(DisplayName = "Character Selection"),
	Extras UMETA(DisplayName = "Extras"),
	Loading UMETA(DisplayName = "Loading")
};

UENUM(BlueprintType)
enum class EGOTALobbySlotState : uint8
{
	Empty,
	Connecting,
	Occupied,
	Ready,
	Disconnected
};

UENUM(BlueprintType)
enum class EGOTASessionPrivacy : uint8
{
	Public,
	FriendsOnly,
	Private
};

/** Snapshot of one lobby presentation slot (host + up to 3 friends). */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTALobbyPlayerSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FText PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	EGOTALobbySlotState State = EGOTALobbySlotState::Empty;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsHost = false;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsLocalPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAArenaCard
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	EArenaId ArenaId = EArenaId::None;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	FText Tagline;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	TArray<EBiomeId> Biomes;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	TArray<EArenaMode> SupportedModes;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	bool bIsSelectable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	bool bComingSoon = true;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(BlueprintReadOnly, Category = "Arena")
	FSoftObjectPath ArenaMap;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGOTAMenuScreenChanged, EGOTAMenuScreen, NewScreen, EGOTAMenuScreen, PreviousScreen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGOTAMenuTransitionFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGOTALobbyViewUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGOTASettingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGOTASettingsApplyResult, bool, bSuccess);
