// White Tiger Studios 2024

#include "UI/Widgets/GOTASettingsWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "UI/GOTAGameUserSettings.h"
#include "UI/GOTAMenuSettingsAdapter.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UI/Widgets/GOTAMenuButtonWidget.h"
#include "UI/Widgets/GOTASettingRowWidget.h"

namespace GOTASettingsText
{
	static TArray<FText> QualityLevels()
	{
		return {
			NSLOCTEXT("GOTA", "QLow", "LOW"), NSLOCTEXT("GOTA", "QMed", "MEDIUM"), NSLOCTEXT("GOTA", "QHigh", "HIGH"),
			NSLOCTEXT("GOTA", "QEpic", "EPIC"), NSLOCTEXT("GOTA", "QCine", "CINEMATIC") };
	}
	static TArray<FText> OffOn()
	{
		return { NSLOCTEXT("GOTA", "Off", "OFF"), NSLOCTEXT("GOTA", "On", "ON") };
	}
	static FText Percent(float V)
	{
		return FText::Format(NSLOCTEXT("GOTA", "PctFmt", "{0}%"), FText::AsNumber(FMath::RoundToInt(V * 100.f)));
	}
	static const int32 FrameRates[] = { 30, 60, 120, 144, 0 };
}

UGOTASettingsWidget::UGOTASettingsWidget()
{
	ScreenId = EGOTAMenuScreen::Settings;
}

UGOTAMenuSettingsAdapter* UGOTASettingsWidget::GetAdapter() const
{
	const UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	return Subsystem ? Subsystem->GetSettingsAdapter() : nullptr;
}

void UGOTASettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	auto BindTab = [this](UGOTAMenuButtonWidget* Tab, FText Label)
	{
		if (Tab)
		{
			Tab->Style = EGOTAButtonStyle::Tab;
			Tab->SetLabel(Label);
			TrackFocus(Tab);
		}
	};
	BindTab(Tab_Video, NSLOCTEXT("GOTA", "TabVideo", "VIDEO"));
	BindTab(Tab_Audio, NSLOCTEXT("GOTA", "TabAudio", "AUDIO"));
	BindTab(Tab_Controls, NSLOCTEXT("GOTA", "TabControls", "CONTROLS"));
	BindTab(Tab_Accessibility, NSLOCTEXT("GOTA", "TabAccess", "ACCESSIBILITY"));
	if (Tab_Video) { Tab_Video->OnActivated.AddUniqueDynamic(this, &UGOTASettingsWidget::HandleTabVideo); }
	if (Tab_Audio) { Tab_Audio->OnActivated.AddUniqueDynamic(this, &UGOTASettingsWidget::HandleTabAudio); }
	if (Tab_Controls) { Tab_Controls->OnActivated.AddUniqueDynamic(this, &UGOTASettingsWidget::HandleTabControls); }
	if (Tab_Accessibility) { Tab_Accessibility->OnActivated.AddUniqueDynamic(this, &UGOTASettingsWidget::HandleTabAccessibility); }

	auto BindAction = [this](UGOTAMenuButtonWidget* Btn, FText Label)
	{
		if (Btn)
		{
			Btn->SetLabel(Label);
			TrackFocus(Btn);
		}
	};
	BindAction(Btn_Apply, NSLOCTEXT("GOTA", "Apply", "APPLY"));
	BindAction(Btn_Cancel, NSLOCTEXT("GOTA", "Revert", "REVERT"));
	BindAction(Btn_Defaults, NSLOCTEXT("GOTA", "Defaults", "DEFAULTS"));
	BindAction(Btn_Back, NSLOCTEXT("GOTA", "Back", "BACK"));
	if (Btn_Apply) { Btn_Apply->OnActivated.AddUniqueDynamic(this, &UGOTASettingsWidget::HandleApply); }
	if (Btn_Cancel) { Btn_Cancel->OnActivated.AddUniqueDynamic(this, &UGOTASettingsWidget::HandleCancel); }
	if (Btn_Defaults) { Btn_Defaults->OnActivated.AddUniqueDynamic(this, &UGOTASettingsWidget::HandleDefaults); }
	if (Btn_Back) { Btn_Back->OnActivated.AddUniqueDynamic(this, &UGOTASettingsWidget::HandleBack); }
}

void UGOTASettingsWidget::OnScreenActivated()
{
	if (UGOTAMenuSettingsAdapter* Adapter = GetAdapter())
	{
		Adapter->CaptureBaseline();
		CachedResolutions = Adapter->GetSupportedResolutions();
	}
	MarkPending(false);
	SelectTab(ActiveTab);
}

UGOTAMenuButtonWidget* UGOTASettingsWidget::GetDefaultFocusButton() const
{
	switch (ActiveTab)
	{
	case EGOTASettingsTab::Audio: return Tab_Audio;
	case EGOTASettingsTab::Controls: return Tab_Controls;
	case EGOTASettingsTab::Accessibility: return Tab_Accessibility;
	default: return Tab_Video;
	}
}

FReply UGOTASettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const int32 Count = 4;
	int32 Delta = 0;
	if (Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Q) { Delta = -1; }
	if (Key == EKeys::Gamepad_RightShoulder || Key == EKeys::E) { Delta = 1; }
	if (Delta != 0)
	{
		SelectTab(static_cast<EGOTASettingsTab>((static_cast<int32>(ActiveTab) + Delta + Count) % Count));
		if (!Rows.IsEmpty() && Rows[0])
		{
			Rows[0]->FocusRow();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGOTASettingsWidget::SelectTab(EGOTASettingsTab Tab)
{
	ActiveTab = Tab;
	if (Tab_Video) { Tab_Video->SetSelected(Tab == EGOTASettingsTab::Video); }
	if (Tab_Audio) { Tab_Audio->SetSelected(Tab == EGOTASettingsTab::Audio); }
	if (Tab_Controls) { Tab_Controls->SetSelected(Tab == EGOTASettingsTab::Controls); }
	if (Tab_Accessibility) { Tab_Accessibility->SetSelected(Tab == EGOTASettingsTab::Accessibility); }
	RebuildRows();
}

void UGOTASettingsWidget::HandleTabVideo() { SelectTab(EGOTASettingsTab::Video); }
void UGOTASettingsWidget::HandleTabAudio() { SelectTab(EGOTASettingsTab::Audio); }
void UGOTASettingsWidget::HandleTabControls() { SelectTab(EGOTASettingsTab::Controls); }
void UGOTASettingsWidget::HandleTabAccessibility() { SelectTab(EGOTASettingsTab::Accessibility); }

UGOTASettingRowWidget* UGOTASettingsWidget::AddRow()
{
	if (!Box_Rows || !RowClass)
	{
		return nullptr;
	}
	UGOTASettingRowWidget* Row = CreateWidget<UGOTASettingRowWidget>(this, RowClass);
	if (Row)
	{
		Row->OnEdited = [this]() { MarkPending(true); };
		Box_Rows->AddChild(Row);
		Rows.Add(Row);
	}
	return Row;
}

void UGOTASettingsWidget::RebuildRows()
{
	UGOTAMenuSettingsAdapter* Adapter = GetAdapter();
	UGOTAGameUserSettings* S = Adapter ? Adapter->GetSettingsForDisplay() : nullptr;
	if (!Box_Rows || !Adapter || !S)
	{
		return;
	}
	Box_Rows->ClearChildren();
	Rows.Reset();
	TWeakObjectPtr<UGOTAMenuSettingsAdapter> WeakAdapter(Adapter);
	TWeakObjectPtr<UGOTAGameUserSettings> WeakS(S);

	auto Slider01 = [&](FText Label, TFunction<float()> Get, TFunction<void(float)> Set)
	{
		if (UGOTASettingRowWidget* Row = AddRow())
		{
			Row->SetupSlider(Label, 0.f, 1.f, 0.05f, MoveTemp(Get), MoveTemp(Set), &GOTASettingsText::Percent);
		}
	};
	auto Toggle = [&](FText Label, TFunction<bool()> Get, TFunction<void(bool)> Set)
	{
		if (UGOTASettingRowWidget* Row = AddRow())
		{
			Row->SetupChoice(Label, GOTASettingsText::OffOn(),
				[Get]() { return Get() ? 1 : 0; },
				[Set](int32 I) { Set(I == 1); });
		}
	};

	FText Note;
	switch (ActiveTab)
	{
	case EGOTASettingsTab::Video:
	{
		if (UGOTASettingRowWidget* Row = AddRow())
		{
			Row->SetupChoice(NSLOCTEXT("GOTA", "WindowMode", "DISPLAY MODE"),
				{ NSLOCTEXT("GOTA", "WMFull", "FULLSCREEN"), NSLOCTEXT("GOTA", "WMBorderless", "BORDERLESS"), NSLOCTEXT("GOTA", "WMWindowed", "WINDOWED") },
				[WeakS]() { return WeakS.IsValid() ? static_cast<int32>(WeakS->GetFullscreenMode()) : 0; },
				[WeakAdapter](int32 I) { if (WeakAdapter.IsValid()) { WeakAdapter->SetFullscreenMode(static_cast<EWindowMode::Type>(I)); } });
		}
		if (CachedResolutions.Num() > 0)
		{
			TArray<FText> ResOptions;
			for (const FIntPoint& R : CachedResolutions)
			{
				ResOptions.Add(FText::FromString(FString::Printf(TEXT("%d × %d"), R.X, R.Y)));
			}
			TArray<FIntPoint> Resolutions = CachedResolutions;
			if (UGOTASettingRowWidget* Row = AddRow())
			{
				Row->SetupChoice(NSLOCTEXT("GOTA", "Resolution", "RESOLUTION"), ResOptions,
					[WeakS, Resolutions]() { return WeakS.IsValid() ? Resolutions.IndexOfByKey(WeakS->GetScreenResolution()) : 0; },
					[WeakAdapter, Resolutions](int32 I) { if (WeakAdapter.IsValid() && Resolutions.IsValidIndex(I)) { WeakAdapter->SetResolution(Resolutions[I]); } });
			}
		}
		if (UGOTASettingRowWidget* Row = AddRow())
		{
			Row->SetupChoice(NSLOCTEXT("GOTA", "Preset", "GRAPHICS PRESET"), GOTASettingsText::QualityLevels(),
				[WeakS]() { return WeakS.IsValid() ? WeakS->GetOverallScalabilityLevel() : 0; },
				[this, WeakAdapter](int32 I) { if (WeakAdapter.IsValid()) { WeakAdapter->SetOverallQualityPreset(I); RefreshRows(); } });
		}
		const TPair<int32, FText> Groups[] = {
			{ 2, NSLOCTEXT("GOTA", "QShadows", "SHADOWS") },
			{ 3, NSLOCTEXT("GOTA", "QGI", "GLOBAL ILLUMINATION") },
			{ 4, NSLOCTEXT("GOTA", "QReflections", "REFLECTIONS") },
			{ 7, NSLOCTEXT("GOTA", "QEffects", "EFFECTS") },
			{ 6, NSLOCTEXT("GOTA", "QTextures", "TEXTURES") },
			{ 0, NSLOCTEXT("GOTA", "QViewDist", "VIEW DISTANCE") },
			{ 1, NSLOCTEXT("GOTA", "QAA", "ANTI-ALIASING") },
			{ 5, NSLOCTEXT("GOTA", "QPost", "POST PROCESSING") },
			{ 8, NSLOCTEXT("GOTA", "QFoliage", "FOLIAGE") },
		};
		for (const TPair<int32, FText>& Group : Groups)
		{
			const int32 GroupIndex = Group.Key;
			if (UGOTASettingRowWidget* Row = AddRow())
			{
				Row->SetupChoice(Group.Value, GOTASettingsText::QualityLevels(),
					[WeakAdapter, GroupIndex]() { return WeakAdapter.IsValid() ? WeakAdapter->GetScalabilityGroup(GroupIndex) : 0; },
					[this, WeakAdapter, GroupIndex](int32 I) { if (WeakAdapter.IsValid()) { WeakAdapter->SetScalabilityGroup(GroupIndex, I); RefreshRows(); } });
			}
		}
		Toggle(NSLOCTEXT("GOTA", "VSync", "VSYNC"),
			[WeakS]() { return WeakS.IsValid() && WeakS->bVSyncEnabled; },
			[WeakAdapter](bool b) { if (WeakAdapter.IsValid()) { WeakAdapter->SetVSyncEnabled(b); } });
		if (UGOTASettingRowWidget* Row = AddRow())
		{
			Row->SetupChoice(NSLOCTEXT("GOTA", "FrameLimit", "FRAME RATE LIMIT"),
				{ FText::AsNumber(30), FText::AsNumber(60), FText::AsNumber(120), FText::AsNumber(144), NSLOCTEXT("GOTA", "Unlimited", "UNLIMITED") },
				[WeakS]()
				{
					const int32 Current = WeakS.IsValid() ? WeakS->FrameRateLimitSetting : 0;
					for (int32 I = 0; I < UE_ARRAY_COUNT(GOTASettingsText::FrameRates); ++I)
					{
						if (GOTASettingsText::FrameRates[I] == Current) { return I; }
					}
					return 4;
				},
				[WeakAdapter](int32 I) { if (WeakAdapter.IsValid()) { WeakAdapter->SetFrameRateLimit(GOTASettingsText::FrameRates[FMath::Clamp(I, 0, 4)]); } });
		}
		Note = NSLOCTEXT("GOTA", "VideoNote", "Display mode and resolution changes ask for confirmation and revert automatically.");
		break;
	}
	case EGOTASettingsTab::Audio:
		Slider01(NSLOCTEXT("GOTA", "Master", "MASTER VOLUME"),
			[WeakS]() { return WeakS.IsValid() ? WeakS->MasterVolume : 1.f; },
			[WeakAdapter](float V) { if (WeakAdapter.IsValid()) { WeakAdapter->SetMasterVolume(V); } });
		Slider01(NSLOCTEXT("GOTA", "Music", "MUSIC"),
			[WeakS]() { return WeakS.IsValid() ? WeakS->MusicVolume : 1.f; },
			[WeakAdapter](float V) { if (WeakAdapter.IsValid()) { WeakAdapter->SetMusicVolume(V); } });
		Slider01(NSLOCTEXT("GOTA", "SFX", "SOUND EFFECTS"),
			[WeakS]() { return WeakS.IsValid() ? WeakS->SFXVolume : 1.f; },
			[WeakAdapter](float V) { if (WeakAdapter.IsValid()) { WeakAdapter->SetSFXVolume(V); } });
		Slider01(NSLOCTEXT("GOTA", "UIVol", "INTERFACE"),
			[WeakS]() { return WeakS.IsValid() ? WeakS->UIVolume : 1.f; },
			[WeakAdapter](float V) { if (WeakAdapter.IsValid()) { WeakAdapter->SetUIVolume(V); } });
		Note = NSLOCTEXT("GOTA", "AudioNote", "Dialogue volume will appear once voiced dialogue is routed to its own sound class.");
		break;
	case EGOTASettingsTab::Controls:
		if (UGOTASettingRowWidget* Row = AddRow())
		{
			Row->SetupSlider(NSLOCTEXT("GOTA", "LookSens", "LOOK SENSITIVITY"), 0.2f, 3.f, 0.1f,
				[WeakS]() { return WeakS.IsValid() ? WeakS->MouseSensitivity : 1.f; },
				[WeakAdapter](float V) { if (WeakAdapter.IsValid()) { WeakAdapter->SetMouseSensitivity(V); } },
				[](float V) { return FText::AsNumber(FMath::RoundToFloat(V * 10.f) / 10.f); });
		}
		Toggle(NSLOCTEXT("GOTA", "InvertY", "INVERT VERTICAL LOOK"),
			[WeakS]() { return WeakS.IsValid() && WeakS->bInvertCameraY; },
			[WeakAdapter](bool b) { if (WeakAdapter.IsValid()) { WeakAdapter->SetInvertCameraY(b); } });
		Note = NSLOCTEXT("GOTA", "ControlsNote", "Key and button remapping is not available yet. See Extras › Controls for the current layout.");
		break;
	case EGOTASettingsTab::Accessibility:
		if (UGOTASettingRowWidget* Row = AddRow())
		{
			Row->SetupSlider(NSLOCTEXT("GOTA", "Shake", "CAMERA SHAKE"), 0.f, 1.f, 0.1f,
				[WeakS]() { return WeakS.IsValid() ? WeakS->CameraShakeStrength : 1.f; },
				[WeakAdapter](float V) { if (WeakAdapter.IsValid()) { WeakAdapter->SetCameraShakeStrength(V); } },
				&GOTASettingsText::Percent);
		}
		Toggle(NSLOCTEXT("GOTA", "ReducedMotion", "REDUCED MOTION"),
			[WeakS]() { return WeakS.IsValid() && WeakS->bReducedMotion; },
			[WeakAdapter](bool b) { if (WeakAdapter.IsValid()) { WeakAdapter->SetReducedMotion(b); } });
		if (UGOTASettingRowWidget* Row = AddRow())
		{
			Row->SetupSlider(NSLOCTEXT("GOTA", "UIScale", "INTERFACE SCALE"), 0.75f, 1.5f, 0.05f,
				[WeakS]() { return WeakS.IsValid() ? WeakS->UIScale : 1.f; },
				[WeakAdapter](float V) { if (WeakAdapter.IsValid()) { WeakAdapter->SetUIScale(V); } },
				&GOTASettingsText::Percent);
		}
		Note = NSLOCTEXT("GOTA", "AccessNote", "Reduced Motion removes menu drift, card lift and camera blends.");
		break;
	}
	if (Text_TabNote)
	{
		Text_TabNote->SetText(Note);
	}
}

void UGOTASettingsWidget::RefreshRows()
{
	for (UGOTASettingRowWidget* Row : Rows)
	{
		if (Row)
		{
			Row->Refresh();
		}
	}
}

void UGOTASettingsWidget::MarkPending(bool bPending)
{
	bPendingChanges = bPending;
	if (Text_Pending)
	{
		Text_Pending->SetText(bPending ? NSLOCTEXT("GOTA", "PendingChanges", "UNSAVED CHANGES") : FText::GetEmpty());
	}
	if (Btn_Apply) { Btn_Apply->SetButtonEnabled(bPending); }
	if (Btn_Cancel) { Btn_Cancel->SetButtonEnabled(bPending); }
}

void UGOTASettingsWidget::HandleApply()
{
	UGOTAMenuSettingsAdapter* Adapter = GetAdapter();
	UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem();
	if (!Adapter || !Subsystem)
	{
		return;
	}
	Subsystem->PlayUISound(EGOTAMenuSound::Confirm);
	Adapter->Apply(false);
	Subsystem->RefreshMenuAudioVolumes();
	MarkPending(false);
	RefreshRows();
	if (Adapter->DidLastApplyChangeDisplay())
	{
		FGOTAModalRequest Request;
		Request.Modal = EGOTAMenuModal::ConfirmDisplay;
		Request.Title = NSLOCTEXT("GOTA", "KeepDisplayTitle", "KEEP THESE DISPLAY SETTINGS?");
		Request.Body = NSLOCTEXT("GOTA", "KeepDisplayBody", "Your display mode or resolution changed.");
		Request.ConfirmLabel = NSLOCTEXT("GOTA", "Keep", "KEEP");
		Request.CancelLabel = NSLOCTEXT("GOTA", "RevertDisplay", "REVERT");
		Subsystem->ShowModal(Request);
	}
}

void UGOTASettingsWidget::HandleCancel()
{
	if (UGOTAMenuSettingsAdapter* Adapter = GetAdapter())
	{
		Adapter->Cancel();
	}
	MarkPending(false);
	RefreshRows();
}

void UGOTASettingsWidget::HandleDefaults()
{
	if (UGOTAMenuSettingsAdapter* Adapter = GetAdapter())
	{
		Adapter->RestoreDefaults();
	}
	MarkPending(true);
	RebuildRows();
}

void UGOTASettingsWidget::HideScreen(bool bAnimate)
{
	if (bPendingChanges)
	{
		HandleCancel();
	}
	Super::HideScreen(bAnimate);
}

void UGOTASettingsWidget::HandleBack()
{
	if (bPendingChanges)
	{
		HandleCancel(); // Back discards unapplied edits — never leaves half-applied state.
	}
	if (UGOTAMenuSubsystem* Subsystem = GetMenuSubsystem())
	{
		Subsystem->PlayUISound(EGOTAMenuSound::Back);
		Subsystem->GoBack(false);
	}
}
