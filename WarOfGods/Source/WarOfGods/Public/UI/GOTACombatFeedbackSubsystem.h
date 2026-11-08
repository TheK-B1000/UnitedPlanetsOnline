// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "GOTACombatFeedbackSubsystem.generated.h"

class UTextBlock;

/** One centered line of combat feedback above the crosshair. Built in C++; fades out on its own. */
UCLASS()
class WAROFGODS_API UGOTACombatFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Show(const FText& Text, const FLinearColor& Color, float Seconds);

protected:
	/** The tree must exist before Slate builds the widget; adding a root afterwards never shows. */
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void EnsureLayout();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Line;
	float Remaining = 0.f;
};

/**
 * Local, mode-independent combat feedback (Duel, Horde and Story have different HUDs). Event-driven: it listens to
 * GOTAGAS::OnStatusResisted and tells the player who applied a status when the target resisted it.
 */
UCLASS()
class WAROFGODS_API UGOTACombatFeedbackSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void ShowMessage(const FText& Text, const FLinearColor& Color, float Seconds = 1.8f);

	/** "QUARRY BEHEMOTH RESISTS SLOW" and friends. */
	static FText DescribeResist(const AActor* Target, EGOTAStatus Status, int32 Aspects);

protected:
	void HandleStatusResisted(AActor* Target, EGOTAStatus Status, int32 Aspects, AActor* Instigator);

	UPROPERTY(Transient)
	TObjectPtr<UGOTACombatFeedbackWidget> Widget;
	FDelegateHandle ResistedHandle;
};
