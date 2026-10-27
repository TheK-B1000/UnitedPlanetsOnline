// White Tiger Studios 2024

#include "UI/Widgets/GOTASettingsWidget.h"
#include "UI/GOTAMenuSettingsAdapter.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"

UGOTASettingsWidget::UGOTASettingsWidget()
{
	ScreenId = EGOTAMenuScreen::Settings;
}

void UGOTASettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Btn_Apply)
	{
		Btn_Apply->SetLabel(NSLOCTEXT("GOTA", "Apply", "APPLY"));
		Btn_Apply->OnActivated.AddDynamic(this, &UGOTASettingsWidget::HandleApply);
	}
	if (Btn_Cancel)
	{
		Btn_Cancel->SetLabel(NSLOCTEXT("GOTA", "Cancel", "CANCEL"));
		Btn_Cancel->OnActivated.AddDynamic(this, &UGOTASettingsWidget::HandleCancel);
	}
	if (Btn_Defaults)
	{
		Btn_Defaults->SetLabel(NSLOCTEXT("GOTA", "Defaults", "RESTORE DEFAULTS"));
		Btn_Defaults->OnActivated.AddDynamic(this, &UGOTASettingsWidget::HandleDefaults);
	}
	if (Btn_Back)
	{
		Btn_Back->SetLabel(NSLOCTEXT("GOTA", "Back", "BACK"));
		Btn_Back->OnActivated.AddDynamic(this, &UGOTASettingsWidget::HandleBack);
	}
}

void UGOTASettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGOTAMenuSettingsAdapter* Adapter = GetAdapter())
	{
		Adapter->CaptureBaseline();
	}
	OnSettingsModelRefreshed();
}

UGOTAMenuSettingsAdapter* UGOTASettingsWidget::GetAdapter() const
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		return Subsystem->GetSettingsAdapter();
	}
	return nullptr;
}

void UGOTASettingsWidget::ApplySettings()
{
	if (UGOTAMenuSettingsAdapter* Adapter = GetAdapter())
	{
		Adapter->Apply(false);
	}
	OnSettingsModelRefreshed();
}

void UGOTASettingsWidget::CancelSettings()
{
	if (UGOTAMenuSettingsAdapter* Adapter = GetAdapter())
	{
		Adapter->Cancel();
	}
	OnSettingsModelRefreshed();
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->NavigateTo(EGOTAMenuScreen::MainMenu, false);
	}
}

void UGOTASettingsWidget::RestoreDefaults()
{
	if (UGOTAMenuSettingsAdapter* Adapter = GetAdapter())
	{
		Adapter->RestoreDefaults();
	}
	OnSettingsModelRefreshed();
}

void UGOTASettingsWidget::HandleApply() { ApplySettings(); }
void UGOTASettingsWidget::HandleCancel() { CancelSettings(); }
void UGOTASettingsWidget::HandleDefaults() { RestoreDefaults(); }

void UGOTASettingsWidget::HandleBack()
{
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->GoBack(false);
	}
}
