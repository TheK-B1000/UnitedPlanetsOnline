// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Abilities/GOTAAbilityTypes.h"
#include "GOTACombatFeedbackSubsystem.generated.h"

class UBorder;
class UTextBlock;
class UHorizontalBox;
class UProgressBar;
class UVerticalBox;
class UGOTAAbilitySystemComponent;

/** One status chip: short name with seconds left, over a bar that drains. */
USTRUCT()
struct FGOTAStatusChip
{
	GENERATED_BODY()

	/** Dark backing, so the chip reads on bright sand and over bodies. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> Box;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> Time;
};

/**
 * Combat feedback shared by Duel, Story and Horde: a fading message, the hard-status line, status chips with time left
 * for the local player and their target, and the local shield bar. Built in C++. The readout refreshes at 10 Hz from
 * replicated state and sends nothing over the network.
 */
UCLASS()
class WAROFGODS_API UGOTACombatFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Show(const FText& Text, const FLinearColor& Color, float Seconds);
	/** A status line that stays until told otherwise (no timer, no tick). Empty text hides it. */
	void SetStatus(const FText& Text, const FLinearColor& Color);

protected:
	/** The tree must exist before Slate builds the widget; adding a root afterwards never shows. */
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void EnsureLayout();
	void BuildChips(UHorizontalBox* Row, TArray<FGOTAStatusChip>& Pool);
	/** Show ASC's statuses in a chip row; appends a stable description to OutKey and a readable one to OutLog. */
	void FillChips(const UGOTAAbilitySystemComponent* ASC, TArray<FGOTAStatusChip>& Pool, FString& OutKey, FString& OutLog);
	void UpdateReadout();
	/** Lock-on target, else the Duel opponent, else a living boss. */
	APawn* ResolveTarget(const APawn* Local) const;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Line;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusLine;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> LocalRow;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TargetRow;
	UPROPERTY(Transient)
	TArray<FGOTAStatusChip> LocalChips;
	UPROPERTY(Transient)
	TArray<FGOTAStatusChip> TargetChips;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ShieldBox;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ShieldBar;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ShieldText;
	TWeakObjectPtr<APawn> ReadoutTarget;
	float Remaining = 0.f;
	float ReadoutWait = 0.f;
	float TargetWait = 0.f;
	FString LastReadoutKey;
};

/**
 * Local, mode-independent combat feedback (Duel, Horde and Story have different HUDs). Event-driven: it listens to
 * GOTAGAS::OnStatusResisted and tells the player who applied a status when the target resisted it, and to
 * GOTAGAS::OnStatusCue to show "BURNING" while the local player's own pawn burns. Nothing here polls.
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

	/**
	 * F4: a perfect dodge avoided an attack. Duel's HUD already says so; in Horde and Story this screen's player is told
	 * here. Fed by the fighter component's existing dodge cue, so nothing new crosses the network.
	 */
	static void NotifyPerfectDodge(const AActor* Dodger);

protected:
	void HandleStatusResisted(AActor* Target, EGOTAStatus Status, int32 Aspects, AActor* Instigator);
	void HandleStatusCue(AActor* Target, EGOTAStatus Status, bool bActive);
	/** F4: this screen's own shield broke ("SHIELD BROKEN"); everyone near already hears and sees the break. */
	void HandleShieldBroken(AActor* Target);
	/** F4: sounds for this screen's own pawn: hard control lands, hard control ends, shield fades. */
	void PlayOwnStatusFeedback(AActor* Pawn, EGOTAStatus Status, bool bActive);
	bool IsLocalPawn(const AActor* Actor) const;
	/** A pawn destroyed while a status is up (respawn) never gets the cue's removal; this clears the indicators. */
	UFUNCTION()
	void HandleStatusPawnDestroyed(AActor* Destroyed);
	/** The local pawn gained or lost an indicated status (FROZEN, STUNNED, BURNING). */
	void SetLocalStatus(AActor* Pawn, EGOTAStatus Status, bool bOn);
	/** Show every indicated status the local pawn has, most important first ("FROZEN  |  BURNING"). */
	void RefreshStatusLine();
	UGOTACombatFeedbackWidget* EnsureWidget();

	UPROPERTY(Transient)
	TObjectPtr<UGOTACombatFeedbackWidget> Widget;
	FDelegateHandle ResistedHandle;
	FDelegateHandle StatusCueHandle;
	FDelegateHandle ShieldBrokenHandle;
	/** F4: which of the own pawn's statuses have already sounded (cues can report the same status twice). */
	TWeakObjectPtr<AActor> SoundPawn;
	TArray<EGOTAStatus> SoundedStatuses;
	double LastOwnShieldBreak = -100.0;
	/** The local pawn the indicators are showing for, and which statuses it has. */
	TWeakObjectPtr<AActor> StatusPawn;
	TArray<EGOTAStatus> LocalStatuses;
};
