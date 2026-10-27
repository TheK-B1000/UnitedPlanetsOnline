// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "ArenaTypes.generated.h"

UENUM(BlueprintType)
enum class EArenaId : uint8
{
	None UMETA(DisplayName = "None"),
	TitansQuarry UMETA(DisplayName = "Titan's Quarry"),
	AshfallCrucible UMETA(DisplayName = "Ashfall Crucible"),
	FrostboundCitadel UMETA(DisplayName = "Frostbound Citadel"),
	VerdantSanctuary UMETA(DisplayName = "Verdant Sanctuary"),
	SunscarExpanse UMETA(DisplayName = "Sunscar Expanse"),
	StormbreakCoast UMETA(DisplayName = "Stormbreak Coast"),
	ShatteredDominion UMETA(DisplayName = "The Shattered Dominion")
};

UENUM(BlueprintType)
enum class EBiomeId : uint8
{
	None,
	StoneRuins,
	Volcanic,
	Forest,
	Frozen,
	Desert,
	Coast,
	Coliseum
};

UENUM(BlueprintType)
enum class EArenaMode : uint8
{
	Skirmish UMETA(ToolTip = "Free combat / graybox test"),
	PvEHorde,
	PvEConquest,
	PvPDuel,
	PvPDominion,
	Story
};

UENUM(BlueprintType)
enum class EArenaPhase : uint8
{
	WaitingForPlayers,
	Countdown,
	Active,
	WaveBreak,
	Boss,
	Completed,
	Failed
};

UENUM(BlueprintType)
enum class EArenaGrayboxLayout : uint8
{
	TitansQuarry,
	ShatteredDominionThreeBiome
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBiomeChanged, EBiomeId, NewBiome, EBiomeId, PreviousBiome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArenaPhaseChanged, EArenaPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnArenaWaveChanged, int32, WaveIndex, int32, RemainingEnemies);
