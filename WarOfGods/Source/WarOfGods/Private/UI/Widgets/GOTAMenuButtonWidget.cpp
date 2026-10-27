// White Tiger Studios 2024

#include "UI/Widgets/GOTAMenuButtonWidget.h"

void UGOTAMenuButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetLabel(Label);
	SetButtonEnabled(bStartEnabled);
	if (Button_Root)
	{
		Button_Root->OnClicked.AddDynamic(this, &UGOTAMenuButtonWidget::HandleClicked);
	}
}

void UGOTAMenuButtonWidget::SetLabel(FText InLabel)
{
	Label = MoveTemp(InLabel);
	if (Text_Label)
	{
		Text_Label->SetText(Label);
	}
}

void UGOTAMenuButtonWidget::SetButtonEnabled(bool bEnabled)
{
	bStartEnabled = bEnabled;
	if (Button_Root)
	{
		Button_Root->SetIsEnabled(bEnabled);
	}
	SetIsEnabled(bEnabled);
}

void UGOTAMenuButtonWidget::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
}

void UGOTAMenuButtonWidget::HandleClicked()
{
	if (GetIsEnabled())
	{
		OnActivated.Broadcast();
	}
}
