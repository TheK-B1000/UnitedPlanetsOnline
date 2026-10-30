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
	Loading UMETA(DisplayName = "Loading"),
	HeroSelect UMETA(DisplayName = "Hero Select")
	// Append only — values are serialized in camera shots.
};

/**
 * Single routing vocabulary for the frontend. Nav rows and feature cards both carry an
 * action and call UGOTAMenuSubsystem::ExecuteMenuAction — no duplicated click logic.
 */
UENUM(BlueprintType)
enum class EGOTAMenuAction : uint8
{
	None,
	Continue,
	Story,
	Arena,
	Multiplayer,
	Settings,
	Extras,
	Quit
};

/** Modal layer shown above every screen; blocks input to the screen underneath. */
UENUM(BlueprintType)
enum class EGOTAMenuModal : uint8
{
	None,
	ConfirmQuit,
	Error,
	Busy,
	ConfirmDisplay
};

/** Arena Mode rulesets the menu can launch. */
UENUM(BlueprintType)
enum class EGOTAArenaRuleset : uint8
{
	FightAI UMETA(DisplayName = "Trial of Champions — Fight AI"),
	Horde UMETA(DisplayName = "Wrath of the Gods — Horde")
};

USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAModalRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	EGOTAMenuModal Modal = EGOTAMenuModal::None;

	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	FText Body;

	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	FText ConfirmLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	FText CancelLabel;
};

/** Local profile projection. Never fabricated: fields without a backing system stay unset. */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTAProfileView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Profile")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Profile")
	FText RankTitle;

	/** False until a persistent progression system exists — UI hides level + XP bar. */
	UPROPERTY(BlueprintReadOnly, Category = "Profile")
	bool bHasProgression = false;

	UPROPERTY(BlueprintReadOnly, Category = "Profile")
	int32 Level = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Profile")
	float LevelProgress = 0.f;
};

/** Read-only view of one LAN search result for the session browser. */
USTRUCT(BlueprintType)
struct WAROFGODS_API FGOTASessionResultView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 ResultIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 OpenSlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bIsFull = false;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGOTAMenuModalChanged, const FGOTAModalRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGOTALobbyViewUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGOTASettingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGOTASettingsApplyResult, bool, bSuccess);
