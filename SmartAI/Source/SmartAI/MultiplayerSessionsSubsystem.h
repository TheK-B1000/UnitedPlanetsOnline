// White Tiger Studios 2023

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class SMARTAI_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UMultiplayerSessionsSubsystem();
	// Begin USubsystem
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
	// End USubsystem
