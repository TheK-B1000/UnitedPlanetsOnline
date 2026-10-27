// White Tiger Studios 2024

#include "UI/Widgets/GOTAMenuScreenWidget.h"
#include "UI/GOTAMenuSubsystem.h"
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
		Subsystem->OnMenuScreenChanged.AddDynamic(this, &UGOTAMenuScreenWidget::HandleMenuScreenChanged);
		HandleMenuScreenChanged(Subsystem->GetCurrentScreen(), EGOTAMenuScreen::None);
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

void UGOTAMenuScreenWidget::ShowScreen(bool bAnimate)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UGOTAMenuScreenWidget::HideScreen(bool bAnimate)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGOTAMenuScreenWidget::HandleMenuScreenChanged(EGOTAMenuScreen NewScreen, EGOTAMenuScreen PreviousScreen)
{
	if (NewScreen == ScreenId)
	{
		ShowScreen(true);
	}
	else
	{
		HideScreen(true);
	}
	OnMenuScreenChanged(NewScreen, PreviousScreen);
}
