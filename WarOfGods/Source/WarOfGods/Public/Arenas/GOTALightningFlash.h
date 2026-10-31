// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GOTALightningFlash.generated.h"

class UDirectionalLightComponent;
class USoundBase;

/**
 * Cosmetic storm lightning for arena atmosphere (Stormbreak Coast, Dominion coast).
 * Purely local: not replicated, never affects gameplay. Each peer flashes on its own random
 * schedule. Intensity and duration are capped so flashes never wash out combat telegraphs,
 * and Reduced Motion disables them.
 */
UCLASS()
class WAROFGODS_API AGOTALightningFlash : public AActor
{
	GENERATED_BODY()

public:
	AGOTALightningFlash();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Lightning")
	TObjectPtr<UDirectionalLightComponent> Flash;

	/** Peak flash illuminance (lux). Kept well below the sun so silhouettes stay readable. */
	UPROPERTY(EditAnywhere, Category = "Lightning")
	float PeakIntensity = 6.f;

	UPROPERTY(EditAnywhere, Category = "Lightning")
	FVector2D IntervalSeconds = FVector2D(9.f, 22.f);

	UPROPERTY(EditAnywhere, Category = "Lightning")
	TObjectPtr<USoundBase> Thunder;

	UPROPERTY(EditAnywhere, Category = "Lightning")
	float ThunderVolume = 0.6f;

protected:
	void ScheduleNext();
	double NextStrikeTime = 0.0;
	double StrikeStart = -1.0;
	float StrikeScale = 1.f;
	bool bThunderPending = false;
	double ThunderTime = 0.0;
};
