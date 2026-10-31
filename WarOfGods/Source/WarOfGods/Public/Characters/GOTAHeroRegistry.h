// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Arenas/ArenaTypes.h"
#include "GOTAHeroRegistry.generated.h"

class UGOTAHeroDefinition;

/**
 * Which heroes exist, and whether a choice is legal. Loads hero DEFINITIONS (small) through the
 * AssetManager ("GOTAHero" primary assets); art stays soft until a widget asks for it.
 * Validation is the single rule used by both the menu (to grey out) and the server (to accept).
 * Also remembers the local player's *intent* for the next match (not authoritative).
 */
UCLASS()
class WAROFGODS_API UGOTAHeroRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** All registered heroes, sorted by SortOrder. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Heroes")
	TArray<UGOTAHeroDefinition*> GetHeroes() const;

	UFUNCTION(BlueprintPure, Category = "GOTA|Heroes")
	UGOTAHeroDefinition* FindHero(FName HeroId) const;

	/** Server-grade validation: exists, playable, mode allowed, pawn class loadable. */
	UFUNCTION(BlueprintPure, Category = "GOTA|Heroes")
	bool ValidateSelection(FName HeroId, EArenaMode Mode, FText& OutReason) const;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Heroes")
	void SetLocalSelection(FName HeroId) { LocalSelection = HeroId; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Heroes")
	FName GetLocalSelection() const { return LocalSelection; }

	/** First playable hero for the mode (used as the default selection). */
	UFUNCTION(BlueprintPure, Category = "GOTA|Heroes")
	FName GetDefaultHero(EArenaMode Mode) const;

protected:
	void EnsureLoaded() const;

	UPROPERTY()
	mutable TArray<TObjectPtr<UGOTAHeroDefinition>> Heroes;

	mutable bool bLoaded = false;
	FName LocalSelection;
};
