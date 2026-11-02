// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"

class APawn;
class UGOTADuelFighterComponent;
class UGOTAHeroAbilityComponent;

/**
 * Mode-independent "is combat active" query.
 * Duel, Story, and Horde each publish their own live window; combat components call this instead of
 * hard-coding ADuelGameState. Death, respawn, waves, and scoring stay in each GameMode.
 */
namespace GOTACombatLive
{
	/** True while players may attack, defend, spend momentum, and use abilities. */
	WAROFGODS_API bool IsCombatLive(const UWorld* World);

	/** Nearest living pawn that owns a UGOTABossComponent (server or client). */
	WAROFGODS_API APawn* FindNearestBoss(APawn* From);

	/**
	 * Server: ensure the player has a duel fighter + hero abilities for the shared combat path.
	 * Opponent is refreshed to the nearest boss when missing or defeated.
	 */
	WAROFGODS_API void EnsurePlayerCombat(APawn* PlayerPawn, FName HeroId);
}
