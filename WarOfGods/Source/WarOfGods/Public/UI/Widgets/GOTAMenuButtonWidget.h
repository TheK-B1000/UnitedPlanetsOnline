// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MenuTypes.h"
#include "GOTAMenuButtonWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UBorder;
class UTexture2D;
class UGOTAMenuButtonWidget;
class UGOTAMenuSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGOTAMenuButtonActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGOTAMenuButtonFocused, UGOTAMenuButtonWidget*, Button);

/** Visual family — one class drives every interactive element in the frontend. */
UENUM(BlueprintType)
enum class EGOTAButtonStyle : uint8
{
	NavRow,      // left navigation: icon + label + subtitle, gold frame + sweep when focused
	FeatureCard, // bottom illustrated card: art lifts and frame brightens
	Action,      // framed action button (Apply, Host, Rematch…)
	Tab,         // settings / extras tabs
	ListEntry    // dynamic list rows (arenas, sessions, lore)
};

/**
 * Shared frontend button.
 * Bind (all optional): Button_Root, Text_Label, Text_Subtitle, Image_Icon, Image_Art,
 * Image_Selection, Border_Frame.
 * If MenuAction is set, activation routes through UGOTAMenuSubsystem::ExecuteMenuAction,
 * so a nav row and its feature card share exactly one code path.
 */
UCLASS(Blueprintable)
class WAROFGODS_API UGOTAMenuButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetLabel(FText InLabel);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetSubtitle(FText InSubtitle);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetIcon(UTexture2D* InIcon);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetArt(UTexture2D* InArt);

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetButtonEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool IsButtonEnabled() const { return bButtonEnabled; }

	/** Persistent selection (chosen tab, chosen arena) — distinct from focus and hover. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool IsSelected() const { return bIsSelected; }

	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void FocusButton();

	UFUNCTION(BlueprintPure, Category = "GOTA|Menu")
	bool HasFocusedButton() const { return bHasFocus; }

	/** Pulls label / subtitle / icon / art for MenuAction from DA_GOTA_MenuContent. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|Menu")
	void ApplyDestinationContent();

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Menu")
	FOnGOTAMenuButtonActivated OnActivated;

	UPROPERTY(BlueprintAssignable, Category = "GOTA|Menu")
	FOnGOTAMenuButtonFocused OnFocused;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	EGOTAButtonStyle Style = EGOTAButtonStyle::Action;

	/** When set, activation runs the shared menu action (nav rows + feature cards). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	EGOTAMenuAction MenuAction = EGOTAMenuAction::None;

	/** Feature cards read CardSubtitle + CardArt instead of the nav subtitle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	bool bUseCardContent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	FText Label = NSLOCTEXT("GOTA", "MenuButton", "BUTTON");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	bool bStartEnabled = true;

	/** Per-instance icon (e.g. the Friends button). Content-driven icons override this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOTA|Menu")
	TObjectPtr<UTexture2D> IconOverride;

	/** Free payload for list rows (arena id, session index, lore index…). */
	UPROPERTY(BlueprintReadWrite, Category = "GOTA|Menu")
	int32 Payload = INDEX_NONE;

protected:
	UFUNCTION()
	void HandleClicked();
	UFUNCTION()
	void HandlePressed();
	UFUNCTION()
	void HandleHovered();
	UFUNCTION()
	void HandleUnhovered();

	void ApplyVisualState(float DeltaTime, bool bSnap);
	UGOTAMenuSubsystem* GetMenuSubsystem() const;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UButton> Button_Root;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_Label;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UTextBlock> Text_Subtitle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UImage> Image_Art;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UImage> Image_Selection;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GOTA|Menu")
	TObjectPtr<UBorder> Border_Frame;

	UPROPERTY(BlueprintReadOnly, Category = "GOTA|Menu")
	bool bIsSelected = false;

	bool bButtonEnabled = true;
	bool bHasFocus = false;
	bool bHovered = false;
	float HighlightAlpha = 0.f;
	float PressAlpha = 0.f;
	float LastFocusSoundTime = -1.f;
};
