// White Tiger Studios 2024

#include "UI/Widgets/GOTAFrontendRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuModalWidget.h"
#include "UI/Widgets/GOTAMenuScreenWidget.h"

namespace GOTARootInput
{
	static bool IsBackKey(const FKey& Key)
	{
		return Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::BackSpace;
	}

	static bool IsNavigationKey(const FKey& Key)
	{
		return Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Left || Key == EKeys::Right
			|| Key == EKeys::Tab
			|| Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down
			|| Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right
			|| Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Gamepad_LeftStick_Down
			|| Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Gamepad_LeftStick_Right
			|| Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter;
	}
}

UGOTAMenuSubsystem* UGOTAFrontendRootWidget::GetMenuSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UGOTAMenuSubsystem>();
	}
	return nullptr;
}

void UGOTAFrontendRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->SetRootMenuWidget(this);
		Subsystem->OnModalChanged.AddUniqueDynamic(this, &UGOTAFrontendRootWidget::HandleModalChanged);
	}
	if (Image_Backdrop)
	{
		Image_Backdrop->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}
	bRestoreFocusNextTick = true;
}

void UGOTAFrontendRootWidget::NativeDestruct()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->OnModalChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UGOTAFrontendRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bRestoreFocusNextTick)
	{
		bRestoreFocusNextTick = false;
		RestoreActiveFocus();
	}

	if (!Image_Backdrop)
	{
		return;
	}
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	if (Subsystem && Subsystem->IsReducedMotion())
	{
		Image_Backdrop->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.03f, 1.03f), FVector2D::ZeroVector, 0.f));
		return;
	}
	// Slow, low-amplitude Lissajous drift: reads as a living vista without competing with UI.
	DriftTime += InDeltaTime;
	const float Phase = (2.f * PI / FMath::Max(1.f, BackdropDriftPeriod)) * DriftTime;
	const FVector2D Offset(FMath::Sin(Phase) * BackdropDriftPixels, FMath::Sin(Phase * 0.5f + 1.1f) * BackdropDriftPixels * 0.4f);
	const float Scale = 1.035f + 0.008f * FMath::Sin(Phase * 0.33f);
	Image_Backdrop->SetRenderTransform(FWidgetTransform(Offset, FVector2D(Scale, Scale), FVector2D::ZeroVector, 0.f));
}

FReply UGOTAFrontendRootWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (GOTARootInput::IsBackKey(Key))
	{
		if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
		{
			if (Subsystem->HandleBackInput())
			{
				return FReply::Handled();
			}
		}
	}
	else if (GOTARootInput::IsNavigationKey(Key))
	{
		// Reached the root unhandled: nothing interactive has focus. Re-seat it.
		RestoreActiveFocus();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UGOTAFrontendRootWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	// The viewport hands focus to the root (startup, alt-tab back): push it to a real control.
	bRestoreFocusNextTick = true;
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

void UGOTAFrontendRootWidget::HandleModalChanged(const FGOTAModalRequest& Request)
{
	if (Request.Modal == EGOTAMenuModal::None)
	{
		// Return focus to the control that opened the modal (screens remember it).
		bRestoreFocusNextTick = true;
	}
}

void UGOTAFrontendRootWidget::RestoreActiveFocus()
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	if (!WidgetTree)
	{
		return;
	}
	if (Subsystem && Subsystem->IsModalOpen())
	{
		WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			if (UGOTAMenuModalWidget* Modal = Cast<UGOTAMenuModalWidget>(Widget))
			{
				Modal->FocusDefault();
			}
		});
		return;
	}
	WidgetTree->ForEachWidget([](UWidget* Widget)
	{
		if (UGOTAMenuScreenWidget* Screen = Cast<UGOTAMenuScreenWidget>(Widget))
		{
			if (Screen->IsActiveScreen())
			{
				Screen->RestoreFocus();
			}
		}
	});
}
