// White Tiger Studios 2024
// Development-only console commands for automated frontend + Fight AI validation.
// Scripts (gota.Test.Run) survive map travel because the runner lives on the core ticker.

#include "CoreMinimal.h"
#include "RHI.h"
#include "HAL/PlatformMemory.h"

#if !UE_BUILD_SHIPPING

#include "Arenas/Duel/DuelGameMode.h"
#include "Arenas/Duel/DuelGameState.h"
#include "Arenas/Duel/DuelPlayerState.h"
#include "Arenas/Duel/GOTADuelCombat.h"
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
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraActor.h"
#include "Engine/SpotLight.h"
#include "Components/SpotLightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
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
					// Viewport commands (HighResShot …) only exist on the game viewport client.
					if (!(GEngine->GameViewport && GEngine->GameViewport->Exec(World, *Line, *GLog)))
					{
						GEngine->Exec(World, *Line);
					}
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
			FString Value;
			It->ExportTextItem_InContainer(Value, DS, nullptr, nullptr, PPF_None);
			UE_LOG(LogGOTATest, Log, TEXT("  prop %s : %s = %s"), *It->GetName(), *It->GetCPPType(), *Value.Left(120));
		}
		for (UClass* C = Pawn->GetClass(); C && C->ClassGeneratedBy; C = C->GetSuperClass())
		{
			for (TFieldIterator<FProperty> It(C, EFieldIteratorFlags::ExcludeSuper); It; ++It)
			{
				if (It->IsA<FMulticastDelegateProperty>()) { continue; }
				FString Value;
				It->ExportTextItem_InContainer(Value, Pawn, nullptr, nullptr, PPF_None);
				UE_LOG(LogGOTATest, Log, TEXT("  pawn[%s] %s : %s = %s"), *C->GetName(), *It->GetName(), *It->GetCPPType(), *Value.Left(100));
			}
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

	static FString ReadBoolProp(UObject* Obj, const TCHAR* Name)
	{
		const FBoolProperty* P = Obj ? FindFProperty<FBoolProperty>(Obj->GetClass(), Name) : nullptr;
		return P ? (P->GetPropertyValue_InContainer(Obj) ? TEXT("1") : TEXT("0")) : TEXT("-");
	}

	/** One-line combat state of a fighter: health, block flags, active montage. */
	static void DuelState(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		const int32 Slot = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		APawn* Pawn = GM ? GM->GetFighterPawn(Slot) : nullptr;
		if (!Pawn)
		{
			return;
		}
		UActorComponent* DS = UGOTACombatantObserver::FindDamageSystem(Pawn);
		const FGOTACombatantVitals V = UGOTACombatantObserver::ReadVitals(Pawn);
		FString Montage = TEXT("-");
		if (const ACharacter* Char = Cast<ACharacter>(Pawn))
		{
			if (UAnimInstance* Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
			{
				if (UAnimMontage* M = Anim->GetCurrentActiveMontage())
				{
					Montage = FString::Printf(TEXT("%s@%.2f/%s"), *M->GetName(), Anim->Montage_GetPosition(M), *Anim->Montage_GetCurrentSection(M).ToString());
				}
			}
		}
		APawn* Other = GM->GetFighterPawn(Slot == 0 ? 1 : 0);
		const UGOTADuelFighterComponent* FC = Pawn->FindComponentByClass<UGOTADuelFighterComponent>();
		UE_LOG(LogGOTATest, Log, TEXT("STATE s%d t=%.2f hp=%.0f mom=%.0f inv=%s blk=%s parryWin=%s react=%s atk=%s montage=%s dist=%.0f"), Slot,
			Pawn->GetWorld()->GetTimeSeconds(), V.Health, FC ? FC->GetMomentum() : -1.f, *ReadBoolProp(DS, TEXT("IsInvincible")), *ReadBoolProp(DS, TEXT("IsBlocking")),
			*ReadBoolProp(Pawn, TEXT("IsWithinParryWindow")), *ReadBoolProp(Pawn, TEXT("IsReactingToBlock")),
			*ReadBoolProp(Pawn, TEXT("Attacking")), *Montage,
			Other ? FVector::Dist(Other->GetActorLocation(), Pawn->GetActorLocation()) : -1.f);
	}
	/** Sovereign Champion director: `state` or force `block|sidestep|guardbreak|phase2|surge`. */
	static void DuelDirector(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		UGOTAChampionDirectorComponent* D = GM ? GM->GetChampionDirector() : nullptr;
		if (!D)
		{
			UE_LOG(LogGOTATest, Warning, TEXT("Director: none"));
			return;
		}
		if (Args.Num() > 0 && !Args[0].Equals(TEXT("state"), ESearchCase::IgnoreCase))
		{
			D->DebugForce(Args[0]);
		}
		UE_LOG(LogGOTATest, Log, TEXT("DIRECTOR %s"), *D->DescribeState());
	}
	static FAutoConsoleCommand CmdDuelDirector(TEXT("gota.Duel.Director"), TEXT("Champion director: state | block | sidestep | guardbreak | phase2 | surge"), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelDirector));

	static UGOTADuelFighterComponent* FighterComp(int32 Slot)
	{
		ADuelGameMode* GM = GetDuelGM();
		APawn* Pawn = GM ? GM->GetFighterPawn(Slot) : nullptr;
		return Pawn ? Pawn->FindComponentByClass<UGOTADuelFighterComponent>() : nullptr;
	}

	/** Same path as pressing Q: Divine Counter for fighter <slot>. */
	static void DuelCounter(const TArray<FString>& Args)
	{
		if (UGOTADuelFighterComponent* F = FighterComp(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0))
		{
			F->RequestDivineCounter();
			UE_LOG(LogGOTATest, Log, TEXT("Counter requested (momentum now %.0f)"), F->GetMomentum());
		}
	}
	static FAutoConsoleCommand CmdDuelCounter(TEXT("gota.Duel.Counter"), TEXT("Request a Divine Counter for fighter <slot> (as if Q was pressed)."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelCounter));

	static void DuelMomentum(const TArray<FString>& Args)
	{
		if (UGOTADuelFighterComponent* F = FighterComp(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0))
		{
			F->GrantMomentum(Args.Num() > 1 ? FCString::Atof(*Args[1]) : 50.f, TEXT("test grant"));
		}
	}
	static FAutoConsoleCommand CmdDuelMomentum(TEXT("gota.Duel.Momentum"), TEXT("Test: grant <amount> Divine Momentum to fighter <slot>."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelMomentum));

	/** Call a Blueprint function with one numeric argument (e.g. DodgeInDirection 400). */
	static void DuelCallF(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		APawn* Pawn = (GM && Args.Num() > 2) ? GM->GetFighterPawn(FCString::Atoi(*Args[0])) : nullptr;
		const bool bOk = Pawn && FGOTABlueprintCombat::Call(Pawn, FName(*Args[1]), nullptr, FString(), FCString::Atod(*Args[2]));
		UE_LOG(LogGOTATest, Log, TEXT("CallF %s(%s) on %s -> %d"), Args.Num() > 1 ? *Args[1] : TEXT("?"), Args.Num() > 2 ? *Args[2] : TEXT("?"), *GetNameSafe(Pawn), bOk ? 1 : 0);
	}
	static FAutoConsoleCommand CmdDuelCallF(TEXT("gota.Duel.CallF"), TEXT("Call BP function <slot> <Function> <number>."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelCallF));

	/**
	 * Test-only challenger bot (no second human, no extra PlayerController): drives the Warden through
	 * the same Blueprint functions its input calls. Modes: off | parry | dodge | fight.
	 * It reads the champion's montage timing — acceptable for a test driver, never used by the AI.
	 */
	struct FDuelTestBot
	{
		int32 Slot = 0;
		FString Mode = TEXT("off");
		float BlockLead = 0.25f;
		float DodgeLead = 0.22f;
		double NextAttack = 0.0;
		double LastDefend = -10.0;
		int32 Blocks = 0, Dodges = 0, Attacks = 0, Counters = 0;
		FTSTicker::FDelegateHandle Handle;
	};
	static FDuelTestBot GDuelBot;

	static void BotFace(APawn* Me, const FVector& Dir)
	{
		FRotator R = Dir.Rotation();
		R.Pitch = 0.f;
		Me->SetActorRotation(R);
		if (AController* C = Me->GetController())
		{
			C->SetControlRotation(R);
		}
	}

	static bool DuelBotTick(float)
	{
		ADuelGameMode* GM = GetDuelGM();
		ADuelGameState* GS = GM ? GM->GetGameState<ADuelGameState>() : nullptr;
		APawn* Me = GM ? GM->GetFighterPawn(GDuelBot.Slot) : nullptr;
		APawn* Opp = GM ? GM->GetFighterPawn(GDuelBot.Slot == 0 ? 1 : 0) : nullptr;
		if (!GS || !Me || !Opp || GS->DuelPhase != EDuelPhase::InRound)
		{
			return true;
		}
		const double Now = Me->GetWorld()->GetTimeSeconds();
		const FVector ToOpp = (Opp->GetActorLocation() - Me->GetActorLocation());
		const float Dist = ToOpp.Size2D();
		float Pos = 0.f;
		UAnimMontage* M = FGOTABlueprintCombat::GetActiveMontage(Opp, &Pos);
		const bool bOppAttacking = FGOTABlueprintCombat::GetBool(Opp, TEXT("Attacking"));
		const float Impact = FGOTABlueprintCombat::FindNextImpactTime(M, Pos);
		const float Lead = (M && Impact >= 0.f) ? Impact - Pos : -1.f;
		const bool bUnblockable = M && M->GetName().Contains(TEXT("GroundSmash"));
		const FString& Mode = GDuelBot.Mode;

		if (bOppAttacking && Lead >= 0.f && Now - GDuelBot.LastDefend > 0.7)
		{
			const bool bWantDodge = Mode == TEXT("dodge") || (Mode == TEXT("fight") && bUnblockable);
			const bool bWantBlock = !bWantDodge && (Mode == TEXT("parry") || Mode == TEXT("fight"));
			if (bWantBlock && Lead <= GDuelBot.BlockLead && Dist < 400.f)
			{
				BotFace(Me, ToOpp);
				FGOTABlueprintCombat::Call(Me, TEXT("StartBlock"));
				GDuelBot.LastDefend = Now;
				++GDuelBot.Blocks;
				UE_LOG(LogGOTATest, Log, TEXT("BOT block (lead %.2f, %s)"), Lead, *GetNameSafe(M));
				return true;
			}
			if (bWantDodge && Lead <= GDuelBot.DodgeLead && Dist < 700.f)
			{
				BotFace(Me, -ToOpp);
				FGOTABlueprintCombat::Call(Me, TEXT("DodgeInDirection"), nullptr, FString(), 300.0);
				GDuelBot.LastDefend = Now;
				++GDuelBot.Dodges;
				UE_LOG(LogGOTATest, Log, TEXT("BOT dodge (lead %.2f, %s)"), Lead, *GetNameSafe(M));
				return true;
			}
		}
		if (Mode == TEXT("fight"))
		{
			UGOTADuelFighterComponent* F = Me->FindComponentByClass<UGOTADuelFighterComponent>();
			if (F && F->CanDivineCounter() && Dist < 300.f && !bOppAttacking)
			{
				F->RequestDivineCounter();
				++GDuelBot.Counters;
				UE_LOG(LogGOTATest, Log, TEXT("BOT divine counter"));
			}
			else if (Dist > 190.f)
			{
				BotFace(Me, ToOpp);
				Me->AddMovementInput(ToOpp.GetSafeNormal2D(), 1.f);
			}
			else if (Now >= GDuelBot.NextAttack && !bOppAttacking && Now - GDuelBot.LastDefend > 0.5)
			{
				BotFace(Me, ToOpp);
				FGOTABlueprintCombat::Call(Me, TEXT("MeleeAttack"));
				GDuelBot.NextAttack = Now + 0.85;
				++GDuelBot.Attacks;
			}
		}
		return true;
	}

	static void DuelBot(const TArray<FString>& Args)
	{
		GDuelBot.Mode = Args.Num() > 0 ? Args[0].ToLower() : TEXT("off");
		GDuelBot.Slot = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
		if (Args.Num() > 2) { GDuelBot.BlockLead = FCString::Atof(*Args[2]); }
		if (GDuelBot.Mode == TEXT("off"))
		{
			if (GDuelBot.Handle.IsValid())
			{
				FTSTicker::GetCoreTicker().RemoveTicker(GDuelBot.Handle);
				GDuelBot.Handle.Reset();
			}
			UE_LOG(LogGOTATest, Log, TEXT("BOT off (blocks=%d dodges=%d attacks=%d counters=%d)"), GDuelBot.Blocks, GDuelBot.Dodges, GDuelBot.Attacks, GDuelBot.Counters);
			return;
		}
		if (!GDuelBot.Handle.IsValid())
		{
			GDuelBot.Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&DuelBotTick));
		}
		UE_LOG(LogGOTATest, Log, TEXT("BOT mode=%s slot=%d blockLead=%.2f"), *GDuelBot.Mode, GDuelBot.Slot, GDuelBot.BlockLead);
	}
	static FAutoConsoleCommand CmdDuelBot(TEXT("gota.Duel.Bot"), TEXT("Test challenger bot: off|parry|dodge|fight [slot] [blockLead]"), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelBot));

	/** Lists a montage's notifies (name, class, time, duration) — e.g. to read ParryWindow timing. */
	static void DuelNotifies(const TArray<FString>& Args)
	{
		UAnimMontage* M = Args.Num() > 0 ? LoadObject<UAnimMontage>(nullptr, *Args[0]) : nullptr;
		if (!M)
		{
			UE_LOG(LogGOTATest, Warning, TEXT("Notifies: cannot load %s"), Args.Num() > 0 ? *Args[0] : TEXT("?"));
			return;
		}
		UE_LOG(LogGOTATest, Log, TEXT("NOTIFIES %s length=%.2f rate=%.2f"), *M->GetName(), M->GetPlayLength(), M->RateScale);
		for (const FAnimNotifyEvent& N : M->Notifies)
		{
			UE_LOG(LogGOTATest, Log, TEXT("  %s cls=%s state=%s t=%.3f dur=%.3f"), *N.NotifyName.ToString(),
				*GetNameSafe(N.Notify ? N.Notify->GetClass() : nullptr), *GetNameSafe(N.NotifyStateClass ? N.NotifyStateClass->GetClass() : nullptr),
				N.GetTriggerTime(), N.GetDuration());
		}
	}
	static FAutoConsoleCommand CmdDuelNotifies(TEXT("gota.Duel.Notifies"), TEXT("List notifies of montage <object path>."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelNotifies));

	/**
	 * Art capture: frames fighter <slot> with a portrait camera (shallow depth of field) and hides all UI,
	 * so `HighResShot` produces a real in-engine hero portrait. Args: slot yawOffset distance fov height.
	 */
	static void TestPortrait(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		const int32 Slot = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		UWorld* GameWorld = GetGameWorld();
		APlayerController* PC = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		// Duel: the slot's fighter. Any other mode: the local player's pawn.
		ACharacter* Char = GM ? Cast<ACharacter>(GM->GetFighterPawn(Slot)) : (PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr);
		if (!Char || !PC)
		{
			UE_LOG(LogGOTATest, Warning, TEXT("Portrait: no fighter %d"), Slot);
			return;
		}
		const float Yaw = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 25.f;
		const float Dist = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 230.f;
		const float Fov = Args.Num() > 3 ? FCString::Atof(*Args[3]) : 32.f;
		const float Drop = Args.Num() > 4 ? FCString::Atof(*Args[4]) : 35.f;
		USkeletalMeshComponent* Mesh = Char->GetMesh();
		const FVector Head = (Mesh && Mesh->DoesSocketExist(TEXT("head"))) ? Mesh->GetSocketLocation(TEXT("head")) : Char->GetActorLocation() + FVector(0, 0, 70);
		const FVector Target = Head - FVector(0, 0, Drop);
		const FVector Fwd = Char->GetActorForwardVector().RotateAngleAxis(Yaw, FVector::UpVector);
		const FVector CamLoc = Target + Fwd * Dist + FVector(0, 0, 6);
		UWorld* W = Char->GetWorld();
		if (Args.Num() > 5 && FCString::Atoi(*Args[5]) != 0)
		{
			// Studio (art capture only, never gameplay): freeze world time so the pose holds,
			// hide the opponent, dark backdrop behind the subject, warm key / cool rim / fill.
			UGameplayStatics::SetGlobalTimeDilation(W, 0.0001f);
			if (APawn* Other = GM ? GM->GetFighterPawn(Slot == 0 ? 1 : 0) : nullptr)
			{
				if (AAIController* AIC = Cast<AAIController>(Other->GetController()))
				{
					AIC->StopMovement();
					if (UBrainComponent* Brain = AIC->GetBrainComponent()) { Brain->PauseLogic(TEXT("Portrait")); }
				}
				Other->SetActorHiddenInGame(true);
				Other->SetActorEnableCollision(false);
				Other->SetActorLocation(Other->GetActorLocation() + FVector(0, 0, -5000));
			}
			const FVector Back = -Char->GetActorForwardVector().RotateAngleAxis(Yaw, FVector::UpVector);
			AStaticMeshActor* Wall = W->SpawnActor<AStaticMeshActor>(Target + Back * 420.f, Back.Rotation());
			if (Wall)
			{
				Wall->SetMobility(EComponentMobility::Movable);
				Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
				Wall->SetActorScale3D(FVector(0.2f, 60.f, 60.f));
				if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Wall);
					MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.006f, 0.008f, 0.02f));
					Wall->GetStaticMeshComponent()->SetMaterial(0, MID);
				}
			}
			auto Light = [W, Target](const FVector& From, const FLinearColor& Color, float Intensity, float Cone)
			{
				if (ASpotLight* L = W->SpawnActor<ASpotLight>(From, (Target - From).Rotation()))
				{
					L->SpotLightComponent->SetMobility(EComponentMobility::Movable);
					L->SpotLightComponent->SetLightColor(Color);
					L->SpotLightComponent->SetIntensity(Intensity);
					L->SpotLightComponent->SetOuterConeAngle(Cone);
					L->SpotLightComponent->SetAttenuationRadius(2000.f);
				}
			};
			const FVector Fw = Char->GetActorForwardVector();
			const FVector Rt = Char->GetActorRightVector();
			Light(Target + Fw * 260.f - Rt * 220.f + FVector(0, 0, 160), FLinearColor(1.f, 0.78f, 0.52f), 60000.f, 30.f);   // warm key
			Light(Target - Fw * 230.f + Rt * 200.f + FVector(0, 0, 120), FLinearColor(0.45f, 0.65f, 1.f), 90000.f, 28.f);  // cool rim
			Light(Target + Fw * 300.f + Rt * 260.f + FVector(0, 0, -20), FLinearColor(0.6f, 0.7f, 1.f), 9000.f, 40.f);     // fill
		}
		ACameraActor* Cam = W->SpawnActor<ACameraActor>(CamLoc, (Target - CamLoc).Rotation());
		if (!Cam)
		{
			return;
		}
		UCameraComponent* CC = Cam->GetCameraComponent();
		CC->SetFieldOfView(Fov);
		CC->SetConstraintAspectRatio(false);
		CC->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
		CC->PostProcessSettings.DepthOfFieldFocalDistance = Dist;
		CC->PostProcessSettings.bOverride_DepthOfFieldFstop = true;
		CC->PostProcessSettings.DepthOfFieldFstop = 1.4f;
		CC->PostProcessSettings.bOverride_DepthOfFieldSensorWidth = true;
		CC->PostProcessSettings.DepthOfFieldSensorWidth = 36.f;
		PC->SetViewTarget(Cam);
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveAllViewportWidgets();
		}
		UE_LOG(LogGOTATest, Log, TEXT("Portrait camera on %s (yaw %.0f dist %.0f fov %.0f) actor=%s head=%s cam=%s mesh=%s"), *GetNameSafe(Char), Yaw, Dist, Fov,
			*Char->GetActorLocation().ToCompactString(), *Head.ToCompactString(), *CamLoc.ToCompactString(), Mesh ? *Mesh->GetComponentLocation().ToCompactString() : TEXT("-"));
	}
	static FAutoConsoleCommand CmdTestPortrait(TEXT("gota.Test.Portrait"), TEXT("Art capture camera on fighter: slot yaw dist fov drop"), FConsoleCommandWithArgsDelegate::CreateStatic(&TestPortrait));

	/** Composition camera: gota.Test.Cam x y z pitch yaw [fov] [hideUI 0|1]. `gota.Test.Cam off` returns to the pawn. */
	static void TestCam(const TArray<FString>& Args)
	{
		UWorld* World = GetGameWorld();
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC)
		{
			return;
		}
		if (Args.Num() > 0 && Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase))
		{
			PC->SetViewTarget(PC->GetPawn());
			return;
		}
		if (Args.Num() < 5)
		{
			UE_LOG(LogGOTATest, Warning, TEXT("gota.Test.Cam x y z pitch yaw [fov] [hideUI]"));
			return;
		}
		const FVector Loc(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2]));
		const FRotator Rot(FCString::Atof(*Args[3]), FCString::Atof(*Args[4]), 0.f);
		static TWeakObjectPtr<ACameraActor> GCam;
		if (!GCam.IsValid() || GCam->GetWorld() != World)
		{
			GCam = World->SpawnActor<ACameraActor>(Loc, Rot);
		}
		if (ACameraActor* Cam = GCam.Get())
		{
			Cam->SetActorLocationAndRotation(Loc, Rot);
			Cam->GetCameraComponent()->SetFieldOfView(Args.Num() > 5 ? FCString::Atof(*Args[5]) : 70.f);
			Cam->GetCameraComponent()->SetConstraintAspectRatio(false);
			PC->SetViewTarget(Cam);
		}
		if (Args.Num() > 6 && FCString::Atoi(*Args[6]) != 0 && GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveAllViewportWidgets();
		}
	}
	static FAutoConsoleCommand CmdTestCam(TEXT("gota.Test.Cam"), TEXT("Composition camera: x y z pitch yaw [fov] [hideUI] | off"), FConsoleCommandWithArgsDelegate::CreateStatic(&TestCam));

	/** Performance probe: samples frame / game / render / GPU time for N seconds, then logs averages, p95 and memory. */
	struct FPerfProbe
	{
		TArray<float> Frame, Game, Render, Gpu;
		double EndTime = 0.0;
		FString Label;
		FTSTicker::FDelegateHandle Handle;
	};
	static FPerfProbe GPerf;

	static bool PerfTick(float DeltaTime)
	{
		GPerf.Frame.Add(DeltaTime * 1000.f);
		GPerf.Game.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
		GPerf.Render.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
		GPerf.Gpu.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
		if (FPlatformTime::Seconds() < GPerf.EndTime)
		{
			return true;
		}
		auto Stat = [](TArray<float> V, float& Avg, float& P95, float& Max)
		{
			V.Sort();
			Avg = 0.f;
			for (float X : V) { Avg += X; }
			Avg = V.Num() ? Avg / V.Num() : 0.f;
			P95 = V.Num() ? V[FMath::Clamp(int32(V.Num() * 0.95f), 0, V.Num() - 1)] : 0.f;
			Max = V.Num() ? V.Last() : 0.f;
		};
		float fa, fp, fm, ga, gp, gm, ra, rp, rm, ua, up, um;
		Stat(GPerf.Frame, fa, fp, fm);
		Stat(GPerf.Game, ga, gp, gm);
		Stat(GPerf.Render, ra, rp, rm);
		Stat(GPerf.Gpu, ua, up, um);
		const FPlatformMemoryStats Mem = FPlatformMemory::GetStats();
		UE_LOG(LogGOTATest, Log, TEXT("PERF %s frames=%d frame avg %.2fms p95 %.2fms max %.2fms (%.1f fps) | game %.2f/%.2f | render %.2f/%.2f | gpu %.2f/%.2f | mem used %.0f MB"),
			*GPerf.Label, GPerf.Frame.Num(), fa, fp, fm, fa > 0.f ? 1000.f / fa : 0.f, ga, gp, ra, rp, ua, up, Mem.UsedPhysical / (1024.0 * 1024.0));
		GPerf.Handle.Reset();
		return false;
	}

	static void TestPerf(const TArray<FString>& Args)
	{
		GPerf = FPerfProbe();
		const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.f;
		GPerf.Label = Args.Num() > 1 ? Args[1] : TEXT("sample");
		GPerf.EndTime = FPlatformTime::Seconds() + Seconds;
		GPerf.Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&PerfTick));
	}
	static FAutoConsoleCommand CmdTestPerf(TEXT("gota.Test.Perf"), TEXT("Sample performance for <seconds> [label]"), FConsoleCommandWithArgsDelegate::CreateStatic(&TestPerf));

	static FAutoConsoleCommand CmdDuelState(TEXT("gota.Duel.State"), TEXT("One-line combat state for fighter <slot>."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelState));

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
	/** Audit: list Blueprint-defined functions (with params) on a fighter, its components and controller. */
	static void DuelFuncs(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		APawn* Pawn = GM ? GM->GetFighterPawn(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0) : nullptr;
		if (!Pawn)
		{
			return;
		}
		TArray<UObject*> Targets = { Pawn, Pawn->GetController() };
		for (UActorComponent* C : Pawn->GetComponents())
		{
			if (C && C->GetClass()->ClassGeneratedBy) { Targets.Add(C); }
		}
		for (UObject* Obj : Targets)
		{
			for (UClass* C = Obj ? Obj->GetClass() : nullptr; C && C->ClassGeneratedBy; C = C->GetSuperClass())
			{
				for (TFieldIterator<UFunction> It(C, EFieldIteratorFlags::ExcludeSuper); It; ++It)
				{
					if (It->GetName().StartsWith(TEXT("ExecuteUbergraph")) || It->GetName().StartsWith(TEXT("BndEvt"))) { continue; }
					FString Params;
					for (TFieldIterator<FProperty> P(*It); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
					{
						Params += FString::Printf(TEXT("%s:%s "), *P->GetName(), *P->GetCPPType());
					}
					UE_LOG(LogGOTATest, Log, TEXT("  fn %s::%s(%s)"), *C->GetName(), *It->GetName(), *Params);
				}
			}
		}
	}
	static FAutoConsoleCommand CmdDuelFuncs(TEXT("gota.Duel.Funcs"), TEXT("List BP functions on a fighter + components + controller."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelFuncs));

	/** Test driver: rotate a fighter (and its controller) to face its opponent. */
	static void DuelFace(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		const int32 Slot = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		APawn* Me = GM ? GM->GetFighterPawn(Slot) : nullptr;
		APawn* Other = GM ? GM->GetFighterPawn(Slot == 0 ? 1 : 0) : nullptr;
		if (!Me || !Other)
		{
			return;
		}
		FRotator Rot = (Other->GetActorLocation() - Me->GetActorLocation()).Rotation();
		Rot.Pitch = 0.f;
		Me->SetActorRotation(Rot);
		if (AController* C = Me->GetController())
		{
			C->SetControlRotation(Rot);
		}
	}
	static FAutoConsoleCommand CmdDuelFace(TEXT("gota.Duel.Face"), TEXT("Face fighter <slot> toward its opponent."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelFace));

	/** Test driver: call a parameterless Blueprint function on a fighter (the same functions its input events call). */
	static void DuelCall(const TArray<FString>& Args)
	{
		ADuelGameMode* GM = GetDuelGM();
		APawn* Pawn = (GM && Args.Num() > 1) ? GM->GetFighterPawn(FCString::Atoi(*Args[0])) : nullptr;
		UFunction* Fn = Pawn ? Pawn->FindFunction(FName(*Args[1])) : nullptr;
		if (!Fn || Fn->ParmsSize != 0)
		{
			UE_LOG(LogGOTATest, Warning, TEXT("Call: %s not found or takes params"), Args.Num() > 1 ? *Args[1] : TEXT("?"));
			return;
		}
		Pawn->ProcessEvent(Fn, nullptr);
		const FGOTACombatantVitals Self = UGOTACombatantObserver::ReadVitals(Pawn);
		const float Dist = FVector::Dist(Pawn->GetActorLocation(), GM->GetFighterPawn(FCString::Atoi(*Args[0]) == 0 ? 1 : 0) ? GM->GetFighterPawn(FCString::Atoi(*Args[0]) == 0 ? 1 : 0)->GetActorLocation() : FVector::ZeroVector);
		UE_LOG(LogGOTATest, Log, TEXT("Call %s on %s (hp=%.0f dist=%.0f)"), *Args[1], *GetNameSafe(Pawn), Self.Health, Dist);
	}
	static FAutoConsoleCommand CmdDuelCall(TEXT("gota.Duel.Call"), TEXT("Call parameterless BP function <slot> <Function> on a fighter."), FConsoleCommandWithArgsDelegate::CreateStatic(&DuelCall));

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
