// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Arenas/ArenaTypes.h"
#include "GOTAPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGOTAHeroChanged, FName, HeroId);

/**
 * Shared player state for the co-op modes (Horde, Story): the player's server-validated hero.
 * Clients request a hero with the join URL option ?Hero=<Id>; the server validates it against the hero
 * registry for the mode and replicates the accepted id (never trusted from the client as-is).
 * The Duel keeps ADuelPlayerState (its own HeroId, slots and round scores).
 */
UCLASS()
class WAROFGODS_API AGOTAPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	/** Server: validate the URL-requested hero for Mode; falls back to the mode's default hero. */
	void AcceptHeroFromOptions(const FString& Options, EArenaMode Mode);

	UFUNCTION(BlueprintPure, Category = "GOTA|Player")
	FName GetHeroId() const { return HeroId; }

	/** Every machine: fired when the accepted hero is set (server) or arrives (clients). */
	UPROPERTY(BlueprintAssignable, Category = "GOTA|Player")
	FOnGOTAHeroChanged OnHeroChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_HeroId, BlueprintReadOnly, Category = "GOTA|Player")
	FName HeroId;

	UFUNCTION()
	void OnRep_HeroId();
};
