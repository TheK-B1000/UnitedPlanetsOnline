// White Tiger Studios 2024

#include "Combat/GOTACombatLive.h"

#include "Abilities/GOTAHeroAbilityComponent.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/DuelTypes.h"
#include "Arenas/Duel/GOTADuelCombat.h"
#include "Arenas/Horde/HordeGameState.h"
#include "Arenas/Horde/HordeTypes.h"
#include "Bosses/GOTABossComponent.h"
#include "Combat/GOTACombatantObserver.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Story/GOTAStoryGameState.h"
#include "Story/StoryTypes.h"

bool GOTACombatLive::IsCombatLive(const UWorld* World)
{
	if (!World)
	{
		return false;
	}
	if (const ADuelGameState* Duel = World->GetGameState<ADuelGameState>())
	{
		return Duel->DuelPhase == EDuelPhase::InRound && Duel->bCombatEnabled;
	}
	if (const AHordeGameState* Horde = World->GetGameState<AHordeGameState>())
	{
		const EHordeMatchPhase Phase = Horde->HordePhase;
		return Phase == EHordeMatchPhase::WaveActive
			|| Phase == EHordeMatchPhase::WaveStarting
			|| Phase == EHordeMatchPhase::BossStarting
			|| Phase == EHordeMatchPhase::BossActive;
	}
	if (const AGOTAStoryGameState* Story = World->GetGameState<AGOTAStoryGameState>())
	{
		const EStoryMissionPhase Phase = Story->MissionPhase;
		return Phase == EStoryMissionPhase::Active || Phase == EStoryMissionPhase::CheckpointReached;
	}
	return false;
}

APawn* GOTACombatLive::FindNearestBoss(APawn* From)
{
	UWorld* World = From ? From->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	APawn* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector Origin = From->GetActorLocation();
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == From || !Candidate->FindComponentByClass<UGOTABossComponent>())
		{
			continue;
		}
		if (UGOTACombatantObserver::IsCombatantDefeated(Candidate))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), Origin);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}
	return Best;
}

void GOTACombatLive::EnsurePlayerCombat(APawn* PlayerPawn, FName HeroId)
{
	if (!IsValid(PlayerPawn) || !PlayerPawn->HasAuthority())
	{
		return;
	}
	UGOTADuelFighterComponent* Fighter = PlayerPawn->FindComponentByClass<UGOTADuelFighterComponent>();
	if (!Fighter)
	{
		Fighter = NewObject<UGOTADuelFighterComponent>(PlayerPawn, TEXT("GOTA_DuelFighter"));
		Fighter->SetIsReplicated(true);
		Fighter->RegisterComponent();
	}
	// Prefer a living boss. Never wipe a valid non-boss opponent (Duel PvP / AI champion).
	APawn* Boss = FindNearestBoss(PlayerPawn);
	APawn* Current = Fighter->GetOpponent();
	if (Boss && (!IsValid(Current) || UGOTACombatantObserver::IsCombatantDefeated(Current) || Current != Boss))
	{
		Fighter->InitFighter(false, Boss);
	}
	else if (!IsValid(Current))
	{
		Fighter->InitFighter(false, nullptr); // bind OnBlocked before the first encounter
	}
	if (!HeroId.IsNone())
	{
		UGOTAHeroAbilityComponent::ApplyToPawn(PlayerPawn, HeroId);
	}
}
