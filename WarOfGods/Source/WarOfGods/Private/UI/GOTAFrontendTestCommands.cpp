// White Tiger Studios 2024
// Development-only console commands for automated frontend + Fight AI validation.
// Scripts (gota.Test.Run) survive map travel because the runner lives on the core ticker.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Arenas/Duel/DuelGameMode.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "EngineUtils.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "NavigationSystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Combat/GOTACombatantObserver.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/GOTAMenuSubsystem.h"
#include "UnrealClient.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTATest, Log, All);

namespace GOTATest
{
	static UWorld* GetGameWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	static UGOTAMenuSubsystem* GetMenu()
	{
		UWorld* World = GetGameWorld();
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UGOTAMenuSubsystem>() : nullptr;
	}

	template <typename TEnum>
	static bool ParseEnum(const FString& Name, TEnum& Out)
	{
		const int64 Value = StaticEnum<TEnum>()->GetValueByNameString(Name);
		if (Value == INDEX_NONE)
		{
			UE_LOG(LogGOTATest, Error, TEXT("Unknown %s value '%s'"), *StaticEnum<TEnum>()->GetName(), *Name);
			return false;
		}
		Out = static_cast<TEnum>(Value);
		return true;
	}

	// ------------------------------------------------------------------ script runner
	struct FRunner
	{
		TArray<FString> Lines;
		int32 Index = 0;
		/** Script time advances by capped frame deltas, so blocking map loads don't eat waits. */
		double ScriptTime = 0.0;
		double ResumeAt = 0.0;
		FTSTicker::FDelegateHandle Handle;

		bool Tick(float DeltaTime)
		{
			ScriptTime += FMath::Min(DeltaTime, 0.1f);
			const double Now = ScriptTime;
			while (Index < Lines.Num() && Now >= ResumeAt)
			{
				FString Line = Lines[Index++].TrimStartAndEnd();
				if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
				{
					continue;
				}
				if (Line.StartsWith(TEXT("wait ")))
				{
					ResumeAt = Now + FCString::Atof(*Line.Mid(5));
					UE_LOG(LogGOTATest, Log, TEXT("[script] %s"), *Line);
					return true;
				}
				UE_LOG(LogGOTATest, Log, TEXT("[script] > %s"), *Line);
				if (UWorld* World = GetGameWorld())
				{
					GEngine->Exec(World, *Line);
				}
			}
			if (Index >= Lines.Num())
			{
				UE_LOG(LogGOTATest, Log, TEXT("[script] complete"));
				return false;
			}
			return true;
		}
	};
	static TUniquePtr<FRunner> Runner;

	static void RunScript(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			return;
		}
		FString Path = Args[0];
		if (FPaths::IsRelative(Path))
		{
			Path = FPaths::Combine(FPaths::ProjectDir(), Path);
		}
		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *Path))
		{
			UE_LOG(LogGOTATest, Error, TEXT("Cannot read script %s"), *Path);
			return;
		}
		Runner = MakeUnique<FRunner>();
		Runner->Lines = MoveTemp(Lines);
		Runner->Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float Dt)
		{
			return Runner.IsValid() && Runner->Tick(Dt);
		}), 0.05f);
		UE_LOG(LogGOTATest, Log, TEXT("Running %s (%d lines)"), *Path, Runner->Lines.Num());
	}

	// ------------------------------------------------------------------ helpers
	static void Shot(const TArray<FString>& Args)
	{
		const FString Name = Args.Num() > 0 ? Args[0] : TEXT("shot");
		const FString File = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("GOTA"), Name + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogGOTATest, Log, TEXT("Screenshot -> %s"), *File);
	}

	static void Key(const TArray<FString>& Args)
	{
		if (Args.Num() < 1 || !FSlateApplication::IsInitialized())
		{
			return;
		}
		const FKey K(*Args[0]);
		if (!K.IsValid())
		{
			UE_LOG(LogGOTATest, Error, TEXT("Invalid key %s"), *Args[0]);
			return;
		}
		// Real Slate key events: exercises focus navigation, Back handling and button accept.
		const FKeyEvent Down(K, FModifierKeysState(), 0, false, 0, 0);
		const bool bHandled = FSlateApplication::Get().ProcessKeyDownEvent(Down);
		const FKeyEvent Up(K, FModifierKeysState(), 0, false, 0, 0);
		FSlateApplication::Get().ProcessKeyUpEvent(Up);
		const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetUserFocusedWidget(0);
		UE_LOG(LogGOTATest, Log, TEXT("Key %s handled=%d focus=%s"), *K.ToString(), bHandled ? 1 : 0,
			Focused.IsValid() ? *Focused->GetReadableLocation() : TEXT("<none>"));
	}

	static void DumpTree(UWidget* W, int32 Depth, int32 MaxDepth)
	{
		if (!W || Depth > MaxDepth)
		{
			return;
		}
		const FGeometry& G = W->GetCachedGeometry();
		const FVector2D Abs = G.GetAbsolutePosition();
		const FVector2D Size = G.GetLocalSize();
		UE_LOG(LogGOTATest, Log, TEXT("   %s%s [%s] vis=%s op=%.2f pos=%.0f,%.0f size=%.0fx%.0f"),
			*FString::ChrN(Depth * 2, TEXT(' ')), *W->GetName(), *W->GetClass()->GetName(),
			*StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(W->GetVisibility())),
			W->GetRenderOpacity(), Abs.X, Abs.Y, Size.X, Size.Y);
		if (UUserWidget* UW = Cast<UUserWidget>(W))
		{
			DumpTree(UW->GetRootWidget(), Depth + 1, MaxDepth);
		}
		else if (UPanelWidget* Panel = Cast<UPanelWidget>(W))
		{
			for (UWidget* Child : Panel->GetAllChildren())
			{
				DumpTree(Child, Depth + 1, MaxDepth);
			}
		}
	}

	static void MenuTree(const TArray<FString>& Args)
	{
		UGOTAMenuSubsystem* Menu = GetMenu();
		UUserWidget* Root = Menu ? Menu->GetRootMenuWidget() : nullptr;
		if (!Root || !Root->WidgetTree)
		{
			return;
		}
		const FName Target = Args.Num() > 0 ? FName(*Args[0]) : NAME_None;
		const int32 MaxDepth = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 4;
		if (UWidget* W = Root->WidgetTree->FindWidget(Target))
		{
			DumpTree(W, 0, MaxDepth);
		}
	}
	static FAutoConsoleCommand CmdMenuTree(TEXT("gota.Menu.Tree"), TEXT("Dump a root layer's widget tree: <LayerName> [depth]."), FConsoleCommandWithArgsDelegate::CreateStatic(&MenuTree));

	static void MenuDump(const TArray<FString>&)
	{
		UWorld* World = GetGameWorld();
		UGOTAMenuSubsystem* Menu = GetMenu();
		if (!World || !Menu)
		{
			UE_LOG(LogGOTATest, Warning, TEXT("MenuDump: no world/menu"));
			return;
		}
		TArray<UUserWidget*> TopLevel;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, TopLevel, UUserWidget::StaticClass(), true);
		int32 Roots = 0;
		FString Names;
		for (UUserWidget* W : TopLevel)
		{
			Names += W->GetClass()->GetName() + TEXT(" ");
			Roots += W->GetClass()->GetName().Contains(TEXT("FrontendRoot")) ? 1 : 0;
		}
		const TSharedPtr<SWidget> Focused = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetUserFocusedWidget(0) : nullptr;
		const APlayerController* PC = World->GetFirstPlayerController();
		UE_LOG(LogGOTATest, Log, TEXT("MENU map=%s screen=%s modal=%s travel=%d roots=%d cursor=%d topLevel=[%s] focus=%s continue=%d"),
			*World->GetMapName(),
			*StaticEnum<EGOTAMenuScreen>()->GetNameStringByValue(static_cast<int64>(Menu->GetCurrentScreen())),
			*StaticEnum<EGOTAMenuModal>()->GetNameStringByValue(static_cast<int64>(Menu->GetActiveModal().Modal)),
			Menu->IsTravelInProgress() ? 1 : 0, Roots, PC && PC->ShouldShowMouseCursor() ? 1 : 0, *Names,
			Focused.IsValid() ? *Focused->GetReadableLocation() : TEXT("<none>"),
			Menu->HasValidContinueSave() ? 1 : 0);

		if (UUserWidget* Root = Menu->GetRootMenuWidget(); Root && Root->WidgetTree)
		{
			Root->WidgetTree->ForEachWidget([](UWidget* W)
			{
				if (UUserWidget* Child = Cast<UUserWidget>(W))
				{
					const FVector2D Size = Child->GetCachedGeometry().GetLocalSize();
					UE_LOG(LogGOTATest, Log, TEXT("   layer %-16s vis=%s opacity=%.2f size=%.0fx%.0f"),
						*Child->GetName(),
						*StaticEnum<ESlateVisibility>()->GetNameStringByValue(static_cast<int64>(Child->GetVisibility())),
						Child->GetRenderOpacity(), Size.X, Size.Y);
				}
			});
		}
	}

	static void MenuGo(const TArray<FString>& Args)
	{
		EGOTAMenuScreen Screen;
		if (UGOTAMenuSubsystem* Menu = GetMenu(); Menu && Args.Num() > 0 && ParseEnum(Args[0], Screen))
		{
			Menu->NavigateTo(Screen, true);
		}
	}

	static void MenuAction(const TArray<FString>& Args)
	{
		EGOTAMenuAction Action;
		if (UGOTAMenuSubsystem* Menu = GetMenu(); Menu && Args.Num() > 0 && ParseEnum(Args[0], Action))
		{
			Menu->ExecuteMenuAction(Action);
		}
	}

	static void MenuModal(const TArray<FString>& Args)
	{
		if (UGOTAMenuSubsystem* Menu = GetMenu())
		{
			Menu->ResolveModal(Args.Num() > 0 && Args[0] == TEXT("1"));
		}
	}

	static void MenuArena(const TArray<FString>& Args)
	{
		EGOTAArenaRuleset Ruleset = EGOTAArenaRuleset::FightAI;
		EArenaId Arena = EArenaId::TitansQuarry;
		if (Args.Num() > 0) { ParseEnum(Args[0], Ruleset); }
		if (Args.Num() > 1) { ParseEnum(Args[1], Arena); }
		if (UGOTAMenuSubsystem* Menu = GetMenu())
		{
			Menu->RequestLaunchArena(Ruleset, Arena);
		}
	}

	// ------------------------------------------------------------------ duel
	static ADuelGameMode* GetDuelGM()
	{
		UWorld* World = GetGameWorld();
		return World ? World->GetAuthGameMode<ADuelGameMode>() : nullptr;
	}

	static void DuelDump(const TArray<FString>&)
	{
		ADuelGameMode* GM = GetDuelGM();
		ADuelGameState* GS = GM ? GM->GetGameState<ADuelGameState>() : nullptr;
		if (!GM || !GS)
		{
			UE_LOG(LogGOTATest, Warning, TEXT("DuelDump: not in a duel"));
			return;
		}
		auto Describe = [](APawn* Pawn)
		{
			const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Pawn);
			const FVector Loc = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
			return FString::Printf(TEXT("%s[%s hp=%.1f/%.1f dead=%d ctrl=%s at %.0f,%.0f,%.0f]"), *GetNameSafe(Pawn),
				V.bHasDamageSystem ? TEXT("DS") : TEXT("noDS"), V.Health, V.MaxHealth, V.bIsDead ? 1 : 0,
				Pawn ? *GetNameSafe(Pawn->GetController()) : TEXT("-"), Loc.X, Loc.Y, Loc.Z);
		};
		const ADuelPlayerState* PS0 = GM->GetFighterPawn(0) ? GM->GetFighterPawn(0)->GetPlayerState<ADuelPlayerState>() : nullptr;
		UE_LOG(LogGOTATest, Log, TEXT("DUEL hero0=%s"), PS0 ? *PS0->HeroId.ToString() : TEXT("-"));
		UE_LOG(LogGOTATest, Log, TEXT("DUEL ai=%d phase=%s round=%d score=%d:%d outcome=%s P0=%s P1=%s"),
			GM->IsVersusAI() ? 1 : 0,
			*StaticEnum<EDuelPhase>()->GetNameStringByValue(static_cast<int64>(GS->DuelPhase)),
			GS->CurrentRound, GS->ScorePlayer0, GS->ScorePlayer1,
			*StaticEnum<EDuelMatchOutcome>()->GetNameStringByValue(static_cast<int64>(GS->MatchOutcome)),
			*Describe(GM->GetFighterPawn(0)), *Describe(GM->GetFighterPawn(1)));
	}

	static void DuelInspect(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		const int32 Slot = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
		APawn* Pawn = GM ? GM->GetFighterPawn(Slot) : nullptr;
		UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Pawn);
		if (!DS)
		{
			UE_LOG(LogGOTATest, Warning, TEXT("Inspect: no damage system on %s"), *GetNameSafe(Pawn));
			return;
		}
		for (TFieldIterator<FProperty> It(DS->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			UE_LOG(LogGOTATest, Log, TEXT("  prop %s : %s"), *It->GetName(), *It->GetCPPType());
		}
		for (TFieldIterator<UFunction> It(DS->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			FString Params;
			for (TFieldIterator<FProperty> P(*It); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
			{
				Params += FString::Printf(TEXT("%s:%s "), *P->GetName(), *P->GetCPPType());
			}
			UE_LOG(LogGOTATest, Log, TEXT("  func %s(%s)"), *It->GetName(), *Params);
		}
	}

	/** Damage through the Blueprint damage system's own TakeDamage function (real path). */
	static void DuelHurt(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		const int32 Slot = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
		const float Amount = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 25.f;
		APawn* Pawn = GM ? GM->GetFighterPawn(Slot) : nullptr;
		UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Pawn);
		UFunction* Fn = nullptr;
		for (TFieldIterator<UFunction> It(DS ? DS->GetClass() : nullptr, EFieldIteratorFlags::ExcludeSuper); DS && It; ++It)
		{
			if (It->GetName().Contains(TEXT("TakeDamage")))
			{
				Fn = *It;
				break;
			}
		}
		if (!Fn)
		{
			UE_LOG(LogGOTATest, Error, TEXT("Hurt: no TakeDamage on %s"), *GetNameSafe(DS));
			return;
		}
		uint8* Buffer = static_cast<uint8*>(FMemory_Alloca(Fn->ParmsSize));
		FMemory::Memzero(Buffer, Fn->ParmsSize);
		Fn->InitializeStruct(Buffer);
		bool bSetAmount = false;
		for (TFieldIterator<FProperty> P(Fn); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
		{
			if (const FStructProperty* StructProp = CastField<FStructProperty>(*P))
			{
				void* StructData = StructProp->ContainerPtrToValuePtr<void>(Buffer);
				for (TFieldIterator<FProperty> Inner(StructProp->Struct); Inner; ++Inner)
				{
					const FString Name = Inner->GetName();
					if (Name.Contains(TEXT("Amount")))
					{
						if (FDoubleProperty* D = CastField<FDoubleProperty>(*Inner)) { D->SetPropertyValue_InContainer(StructData, Amount); bSetAmount = true; }
						if (FFloatProperty* F = CastField<FFloatProperty>(*Inner)) { F->SetPropertyValue_InContainer(StructData, Amount); bSetAmount = true; }
					}
					if (Name.Contains(TEXT("CanBeBlocked")) || Name.Contains(TEXT("CanBeParried")))
					{
						if (FBoolProperty* B = CastField<FBoolProperty>(*Inner)) { B->SetPropertyValue_InContainer(StructData, false); }
					}
				}
			}
			else if (P->GetName().Contains(TEXT("Amount")))
			{
				if (FDoubleProperty* D = CastField<FDoubleProperty>(*P)) { D->SetPropertyValue_InContainer(Buffer, Amount); bSetAmount = true; }
				if (FFloatProperty* F = CastField<FFloatProperty>(*P)) { F->SetPropertyValue_InContainer(Buffer, Amount); bSetAmount = true; }
			}
		}
		DS->ProcessEvent(Fn, Buffer);
		Fn->DestroyStruct(Buffer);
		const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Pawn);
		UE_LOG(LogGOTATest, Log, TEXT("Hurt slot %d by %.1f via %s (amountSet=%d) -> hp=%.1f dead=%d"),
			Slot, Amount, *Fn->GetName(), bSetAmount ? 1 : 0, V.Health, V.bIsDead ? 1 : 0);
	}

	static void Probe(const TArray<FString>& Args)
	{
		UWorld* World = GetGameWorld();
		if (!World || Args.Num() < 2)
		{
			return;
		}
		const float X = FCString::Atof(*Args[0]);
		const float Y = FCString::Atof(*Args[1]);
		FHitResult Hit;
		const FVector Start(X, Y, 2000.f), End(X, Y, -3000.f);
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn);
		const bool bHitVis = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);
		UE_LOG(LogGOTATest, Log, TEXT("Probe %.0f,%.0f pawnHit=%d visHit=%d at z=%.1f actor=%s comp=%s"), X, Y, bHit ? 1 : 0, bHitVis ? 1 : 0,
			Hit.ImpactPoint.Z, *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()));
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetClass()->GetName().Contains(TEXT("Graybox")) || It->GetClass()->GetName().Contains(TEXT("ArenaPlayerStart")))
			{
				UE_LOG(LogGOTATest, Log, TEXT("   %s at %s"), *It->GetName(), *It->GetActorLocation().ToCompactString());
			}
		}
		UE_LOG(LogGOTATest, Log, TEXT("   KillZ=%.0f"), World->GetWorldSettings()->KillZ);
		if (Args.Num() > 2)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->GetClass()->GetName().Contains(TEXT("Graybox")))
				{
					continue;
				}
				TArray<UStaticMeshComponent*> Comps;
				It->GetComponents(Comps);
				for (UStaticMeshComponent* C : Comps)
				{
					const FBox Box = C->Bounds.GetBox();
					if (Args[2] == TEXT("all") || (Box.Min.X <= X && Box.Max.X >= X && Box.Min.Y <= Y && Box.Max.Y >= Y))
					{
						UE_LOG(LogGOTATest, Log, TEXT("   comp %s mesh=%s z[%.0f..%.0f] coll=%d body=%d visible=%d scale=%s"),
							*C->GetName(), *GetNameSafe(C->GetStaticMesh()), Box.Min.Z, Box.Max.Z,
							static_cast<int32>(C->GetCollisionEnabled()), C->GetBodyInstance() && C->GetBodyInstance()->IsValidBodyInstance() ? 1 : 0,
							C->IsVisible() ? 1 : 0, *C->GetComponentScale().ToCompactString());
					}
				}
			}
		}
	}
	static FAutoConsoleCommand CmdProbe(TEXT("gota.Test.Probe"), TEXT("Trace down at <x> <y>."), FConsoleCommandWithArgsDelegate::CreateStatic(&Probe));

	static void DuelAIState(const TArray<FString>&)
	{
		ADuelGameMode* GM = GetDuelGM();
		APawn* Champion = GM ? GM->GetFighterPawn(1) : nullptr;
		APawn* Warden = GM ? GM->GetFighterPawn(0) : nullptr;
		AAIController* AIC = Champion ? Cast<AAIController>(Champion->GetController()) : nullptr;
		UBrainComponent* Brain = AIC ? AIC->GetBrainComponent() : nullptr;
		UBlackboardComponent* BB = AIC ? AIC->GetBlackboardComponent() : nullptr;
		FString Keys;
		if (BB && BB->GetBlackboardAsset())
		{
			for (const FBlackboardEntry& Key : BB->GetBlackboardAsset()->Keys)
			{
				Keys += FString::Printf(TEXT("%s=%s; "), *Key.EntryName.ToString(), *BB->DescribeKeyValue(Key.EntryName, EBlackboardDescription::OnlyValue));
			}
		}
		UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetGameWorld());
		FNavLocation Out;
		const bool bNavW = Nav && Warden && Nav->ProjectPointToNavigation(Warden->GetActorLocation(), Out, FVector(200, 200, 300));
		const bool bNavC = Nav && Champion && Nav->ProjectPointToNavigation(Champion->GetActorLocation(), Out, FVector(200, 200, 300));
		UE_LOG(LogGOTATest, Log, TEXT("AI brain=%s running=%d paused=%d navWarden=%d navChampion=%d navBuilding=%d BB=[%s]"),
			*GetNameSafe(Brain), Brain && Brain->IsRunning() ? 1 : 0, Brain && Brain->IsPaused() ? 1 : 0,
			bNavW ? 1 : 0, bNavC ? 1 : 0, Nav && Nav->IsNavigationBuildInProgress() ? 1 : 0, *Keys);
	}
	static FAutoConsoleCommand CmdDuelAI(TEXT("gota.Duel.AIState"), TEXT("Log champion brain / blackboard / navmesh state."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelAIState));

	static void DuelRematch(const TArray<FString>&)
	{
		if (ADuelGameMode* GM = GetDuelGM()) { GM->RequestRematch(); }
	}

	static void DuelReturn(const TArray<FString>&)
	{
		if (ADuelGameMode* GM = GetDuelGM()) { GM->RequestReturnToLobby(); }
	}

	static FAutoConsoleCommand CmdRun(TEXT("gota.Test.Run"), TEXT("Run a GOTA test script (lines: console commands, 'wait <s>')."), FConsoleCommandWithArgsDelegate::CreateStatic(&RunScript));
	static FAutoConsoleCommand CmdShot(TEXT("gota.Test.Shot"), TEXT("Screenshot to Saved/Screenshots/GOTA/<name>.png"), FConsoleCommandWithArgsDelegate::CreateStatic(&Shot));
	static FAutoConsoleCommand CmdKey(TEXT("gota.Test.Key"), TEXT("Synthesize a Slate key press (e.g. Down, Gamepad_DPad_Down, Escape, Enter)."), FConsoleCommandWithArgsDelegate::CreateStatic(&Key));
	static FAutoConsoleCommand CmdMenuDump(TEXT("gota.Menu.Dump"), TEXT("Log frontend state."), FConsoleCommandWithArgsDelegate::CreateStatic(&MenuDump));
	static FAutoConsoleCommand CmdMenuGo(TEXT("gota.Menu.Go"), TEXT("Navigate to EGOTAMenuScreen."), FConsoleCommandWithArgsDelegate::CreateStatic(&MenuGo));
	static FAutoConsoleCommand CmdMenuAction(TEXT("gota.Menu.Action"), TEXT("Execute EGOTAMenuAction."), FConsoleCommandWithArgsDelegate::CreateStatic(&MenuAction));
	static FAutoConsoleCommand CmdMenuModal(TEXT("gota.Menu.Modal"), TEXT("Resolve the modal (1=confirm, 0=cancel)."), FConsoleCommandWithArgsDelegate::CreateStatic(&MenuModal));
	static FAutoConsoleCommand CmdMenuArena(TEXT("gota.Menu.Arena"), TEXT("Launch arena: <FightAI|Horde> <EArenaId>."), FConsoleCommandWithArgsDelegate::CreateStatic(&MenuArena));
	static FAutoConsoleCommand CmdDuelDump(TEXT("gota.Duel.Dump"), TEXT("Log duel state + fighter vitals."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelDump));
	static FAutoConsoleCommand CmdDuelInspect(TEXT("gota.Duel.Inspect"), TEXT("List damage-system props/functions for slot."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelInspect));
	static FAutoConsoleCommand CmdDuelHurt(TEXT("gota.Duel.Hurt"), TEXT("Damage <slot> <amount> through the BP damage system."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelHurt));
	static FAutoConsoleCommand CmdDuelRematch(TEXT("gota.Duel.Rematch"), TEXT("Request rematch."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelRematch));
	static FAutoConsoleCommand CmdDuelReturn(TEXT("gota.Duel.Return"), TEXT("Return to main menu."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelReturn));
}

#endif // !UE_BUILD_SHIPPING
