// White Tiger Studios 2024

#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "UI/GOTAMenuContentDefinition.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "Kismet/GameplayStatics.h"

void UGOTAMenuScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	MenuSubsystem = GetMenuSubsystem();
}

void UGOTAMenuScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		MenuSubsystem = Subsystem;
		Subsystem->OnMenuScreenChanged.AddUniqueDynamic(this, &UGOTAMenuScreenWidget::HandleMenuScreenChanged);
		HandleMenuScreenChanged(Subsystem->GetCurrentScreen(), EGOTAMenuScreen::None);
	}
	else
	{
		HideScreen(false);
	}
}

void UGOTAMenuScreenWidget::NativeDestruct()
{
	if (MenuSubsystem)
	{
		MenuSubsystem->OnMenuScreenChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

UGOTAMenuSubsystem* UGOTAMenuScreenWidget::GetMenuSubsystem() const
{
	if (MenuSubsystem)
	{
		return MenuSubsystem;
	}
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UGOTAMenuSubsystem>();
	}
	return nullptr;
}

UGOTAMenuContentDefinition* UGOTAMenuScreenWidget::GetMenuContent() const
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	return Subsystem ? Subsystem->GetMenuContent() : nullptr;
}

void UGOTAMenuScreenWidget::ShowScreen(bool bAnimate)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	const bool bReduced = GetMenuSubsystem() && GetMenuSubsystem()->IsReducedMotion();
	EnterAlpha = (bAnimate && !bReduced) ? 0.f : 1.f;
	SetRenderOpacity(EnterAlpha);
	SetRenderTranslation(FVector2D((1.f - EnterAlpha) * -28.f, 0.f));
}

void UGOTAMenuScreenWidget::HideScreen(bool bAnimate)
{
	// Hide is immediate so a new screen never shares input with the old one.
	SetVisibility(ESlateVisibility::Collapsed);
	EnterAlpha = 1.f;
}

void UGOTAMenuScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (EnterAlpha >= 1.f)
	{
		return;
	}
	const UGOTAMenuContentDefinition* Content = GetMenuContent();
	const float Duration = Content ? Content->Theme.ScreenEnterSeconds : 0.28f;
	EnterAlpha = FMath::Min(1.f, EnterAlpha + InDeltaTime / FMath::Max(0.01f, Duration));
	const float Eased = 1.f - FMath::Pow(1.f - EnterAlpha, 3.f);
	SetRenderOpacity(Eased);
	SetRenderTranslation(FVector2D((1.f - Eased) * -28.f, 0.f));
}

void UGOTAMenuScreenWidget::TrackFocus(UGOTAMenuButtonWidget* Button)
{
	if (Button)
	{
		Button->OnFocused.AddUniqueDynamic(this, &UGOTAMenuScreenWidget::HandleTrackedFocus);
	}
}

void UGOTAMenuScreenWidget::HandleTrackedFocus(UGOTAMenuButtonWidget* Button)
{
	LastFocused = Button;
}

void UGOTAMenuScreenWidget::RestoreFocus()
{
	if (!bIsActive)
	{
		return;
	}
	UGOTAMenuButtonWidget* Target = LastFocused.Get();
	if (!Target || !Target->IsButtonEnabled() || !Target->IsVisible())
	{
		Target = GetDefaultFocusButton();
	}
	if (Target)
	{
		Target->FocusButton();
	}
}

void UGOTAMenuScreenWidget::HandleMenuScreenChanged(EGOTAMenuScreen NewScreen, EGOTAMenuScreen PreviousScreen)
{
	const bool bWasActive = bIsActive;
	bIsActive = (NewScreen == ScreenId);
	if (bIsActive)
	{
		ShowScreen(PreviousScreen != EGOTAMenuScreen::None);
		OnScreenActivated();
		if (!bWasActive)
		{
			// Going deeper starts at the default; coming Back restores the remembered control.
			RestoreFocus();
		}
	}
	else
	{
		HideScreen(true);
	}
	OnMenuScreenChanged(NewScreen, PreviousScreen);
}
