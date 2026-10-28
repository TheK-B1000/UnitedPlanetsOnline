// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatTimingConfig.generated.h"

/**
 * Tunable defense windows for Implementations 04–06.
 * Create DA_GOTA_CombatTiming in Editor and assign on UCombatTimingComponent.
 */
UCLASS(BlueprintType)
class WAROFGODS_API UCombatTimingConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Total timed-parry window after press (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0.05"))
	float ParryWindowSeconds = 0.28f;

	/**
	 * Leading sub-window counted as Perfect Parry (seconds from press).
	 * Target feel: ~3–6 frames @ 60fps — scrub in PIE.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0.02"))
	float PerfectParryWindowSeconds = 0.09f;

	/** How long after a successful parry the defender stays locked in recovery (cosmetic gating). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parry", meta = (ClampMin = "0.0"))
	float ParryRecoverySeconds = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parry")
	float PerfectParryDivineMomentum = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parry")
	float NormalParryDivineMomentum = 5.f;

	/** Sustained block (hold) can still Block after the timed parry window expires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Block")
	bool bAllowSustainBlock = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge", meta = (ClampMin = "0.1"))
	float DodgeDurationSeconds = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge", meta = (ClampMin = "0.0"))
	float DodgeIFrameStartSeconds = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge", meta = (ClampMin = "0.05"))
	float DodgeIFrameEndSeconds = 0.28f;

	/** Perfect dodge if the hit lands within this many seconds after i-frames begin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge", meta = (ClampMin = "0.02"))
	float PerfectDodgeWindowSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float PerfectDodgeDivineMomentum = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float NormalDodgeDivineMomentum = 0.f;

	/** Horizontal impulse scale applied locally for feel (BP/movement can also read dodge dir). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeDistance = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Divine", meta = (ClampMin = "1.0"))
	float DivineCounterCost = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Divine", meta = (ClampMin = "0.1"))
	float DivineCounterActiveSeconds = 0.75f;

	/** Server accepts client press timestamps within this tolerance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network", meta = (ClampMin = "0.0"))
	float LatencyToleranceSeconds = 0.085f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("CombatTimingConfig"), GetFName());
	}
};
