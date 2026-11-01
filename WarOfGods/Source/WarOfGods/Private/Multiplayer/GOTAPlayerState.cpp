// White Tiger Studios 2024

#include "Multiplayer/GOTAPlayerState.h"

#include "Characters/GOTAHeroRegistry.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void AGOTAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGOTAPlayerState, HeroId);
}

void AGOTAPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if (AGOTAPlayerState* Other = Cast<AGOTAPlayerState>(PlayerState))
	{
		Other->HeroId = HeroId; // keep the chosen hero across seamless travel / reconnect
	}
}

void AGOTAPlayerState::AcceptHeroFromOptions(const FString& Options, EArenaMode Mode)
{
	if (!HasAuthority())
	{
		return;
	}
	const UGOTAHeroRegistry* Registry = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	if (!Registry)
	{
		return;
	}
	const FName Requested(*UGameplayStatics::ParseOption(Options, TEXT("Hero")));
	FText Reason;
	if (!Requested.IsNone() && Registry->ValidateSelection(Requested, Mode, Reason))
	{
		HeroId = Requested;
	}
	else
	{
		HeroId = Registry->GetDefaultHero(Mode);
		if (!Requested.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("Player %s: hero '%s' rejected (%s) - using %s"), *GetPlayerName(), *Requested.ToString(), *Reason.ToString(), *HeroId.ToString());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Player %s accepted hero %s"), *GetPlayerName(), *HeroId.ToString());
	OnHeroChanged.Broadcast(HeroId);
}

void AGOTAPlayerState::OnRep_HeroId()
{
	OnHeroChanged.Broadcast(HeroId);
}
