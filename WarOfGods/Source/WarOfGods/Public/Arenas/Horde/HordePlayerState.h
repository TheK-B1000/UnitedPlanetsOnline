// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HordePlayerState.generated.h"

UCLASS()
class WAROFGODS_API AHordePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AHordePlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	int32 Eliminations = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	int32 EliteEliminations = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	int32 PersonalScore = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	bool bIsEliminated = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Horde")
	bool bIsSpectating = false;

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void ServerAddElimination(bool bElite, int32 ScoreDelta);

	UFUNCTION(BlueprintCallable, Category = "Horde")
	void SetEliminated(bool bEliminated);
};
