// White Tiger Studios 2024

#include "UI/Widgets/GOTAMenuModalWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTAMenuSubsystem* UGOTAMenuModalWidget::GetMenuSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UGOTAMenuSubsystem>();
	}
	return nullptr;
}

void UGOTAMenuModalWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Btn_Confirm)
	{
		Btn_Confirm->Style = EGOTAButtonStyle::Action;
		Btn_Confirm->OnActivated.AddUniqueDynamic(this, &UGOTAMenuModalWidget::HandleConfirm);
	}
	if (Btn_Cancel)
	{
		Btn_Cancel->Style = EGOTAButtonStyle::Action;
		Btn_Cancel->OnActivated.AddUniqueDynamic(this, &UGOTAMenuModalWidget::HandleCancel);
	}
	// Trap navigation inside the dialog: only Cancel <-> Confirm, never the screen beneath.
	if (Btn_Cancel && Btn_Confirm)
	{
		for (UGOTAMenuButtonWidget* Btn : { Btn_Cancel.Get(), Btn_Confirm.Get() })
		{
			Btn->SetNavigationRuleBase(EUINavigation::Up, EUINavigationRule::Stop);
			Btn->SetNavigationRuleBase(EUINavigation::Down, EUINavigationRule::Stop);
			Btn->SetNavigationRuleBase(EUINavigation::Next, EUINavigationRule::Stop);
			Btn->SetNavigationRuleBase(EUINavigation::Previous, EUINavigationRule::Stop);
		}
		Btn_Cancel->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
		Btn_Cancel->SetNavigationRuleExplicit(EUINavigation::Right, Btn_Confirm);
		Btn_Confirm->SetNavigationRuleExplicit(EUINavigation::Left, Btn_Cancel);
		Btn_Confirm->SetNavigationRuleBase(EUINavigation::Right, EUINavigationRule::Stop);
	}
}

void UGOTAMenuModalWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->OnModalChanged.AddUniqueDynamic(this, &UGOTAMenuModalWidget::HandleModalChanged);
		ApplyRequest(Subsystem->GetActiveModal());
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGOTAMenuModalWidget::NativeDestruct()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->OnModalChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UGOTAMenuModalWidget::HandleModalChanged(const FGOTAModalRequest& Request)
{
	ApplyRequest(Request);
}

void UGOTAMenuModalWidget::ApplyRequest(const FGOTAModalRequest& Request)
{
	ActiveModal = Request.Modal;
	if (ActiveModal == EGOTAMenuModal::None)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// Visible (not SelfHitTestInvisible): the dim layer swallows clicks meant for the screen below.
	SetVisibility(ESlateVisibility::Visible);
	BaseBody = Request.Body;
	if (Text_Title) { Text_Title->SetText(Request.Title); }
	if (Text_Body) { Text_Body->SetText(Request.Body); }

	const bool bBusy = ActiveModal == EGOTAMenuModal::Busy;
	const bool bSingleButton = ActiveModal == EGOTAMenuModal::Error;
	if (Panel_Buttons) { Panel_Buttons->SetVisibility(bBusy ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible); }
	if (Image_Spinner) { Image_Spinner->SetVisibility(bBusy ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (Btn_Confirm)
	{
		Btn_Confirm->SetLabel(Request.ConfirmLabel);
		Btn_Confirm->SetButtonEnabled(!bBusy);
	}
	if (Btn_Cancel)
	{
		Btn_Cancel->SetLabel(Request.CancelLabel);
		Btn_Cancel->SetVisibility(bSingleButton ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		Btn_Cancel->SetButtonEnabled(!bBusy && !bSingleButton);
	}

	CountdownRemaining = ActiveModal == EGOTAMenuModal::ConfirmDisplay ? DisplayConfirmSeconds : 0.f;
	FocusDefault();
}

void UGOTAMenuModalWidget::FocusDefault()
{
	if (ActiveModal == EGOTAMenuModal::None || ActiveModal == EGOTAMenuModal::Busy)
	{
		SetFocus();
		return;
	}
	// Destructive / risky choices default to the safe option.
	const bool bSafeIsCancel = ActiveModal == EGOTAMenuModal::ConfirmQuit || ActiveModal == EGOTAMenuModal::ConfirmDisplay;
	UGOTAMenuButtonWidget* Target = (bSafeIsCancel && Btn_Cancel) ? Btn_Cancel.Get() : Btn_Confirm.Get();
	if (Target)
	{
		Target->FocusButton();
	}
}

void UGOTAMenuModalWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ActiveModal == EGOTAMenuModal::Busy && Image_Spinner)
	{
		SpinnerAngle = FMath::Fmod(SpinnerAngle + InDeltaTime * 140.f, 360.f);
		Image_Spinner->SetRenderTransformAngle(SpinnerAngle);
	}
	if (ActiveModal == EGOTAMenuModal::ConfirmDisplay && CountdownRemaining > 0.f)
	{
		CountdownRemaining -= InDeltaTime;
		if (Text_Body)
		{
			Text_Body->SetText(FText::Format(NSLOCTEXT("GOTA", "DisplayRevertFmt", "{0}\n\nReverting in {1} seconds."),
				BaseBody, FText::AsNumber(FMath::Max(0, FMath::CeilToInt(CountdownRemaining)))));
		}
		if (CountdownRemaining <= 0.f)
		{
			HandleCancel();
		}
	}
}

void UGOTAMenuModalWidget::HandleConfirm()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->PlayUISound(EGOTAMenuSound::Confirm);
		Subsystem->ResolveModal(true);
	}
}

void UGOTAMenuModalWidget::HandleCancel()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->PlayUISound(EGOTAMenuSound::Back);
		Subsystem->ResolveModal(false);
	}
}
