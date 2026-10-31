// White Tiger Studios 2024

#include "Story/GOTAStoryGameMode.h"

#include "Arenas/ArenaGrayboxBuilder.h"
#include "Arenas/ArenaPlayerStart.h"
#include "Arenas/ArenaTypes.h"
#include "Arenas/ArenaVisualDressDefaults.h"
#include "Arenas/ArenaVisualDressDefinition.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Story/GOTAStoryGameState.h"
#include "Story/GOTAStoryMissionDefinition.h"
#include "Story/GOTAStoryObjectiveActor.h"
#include "Story/GOTAStorySaveGame.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGOTAStoryGameMode::AGOTAStoryGameMode()
{
	GameStateClass = AGOTAStoryGameState::StaticClass();
	SoftMaxPlayers = 4;

	SoftPlayerClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/Player/BP_Player.BP_Player_C")));
	SoftEnemyClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/Enemies/MeleeEnemy/BP_Enemy_Melee.BP_Enemy_Melee_C")));
	SoftGuardianClass = SoftEnemyClass;

	static ConstructorHelpers::FClassFinder<APawn> PlayerFinder(TEXT("/Game/Player/BP_Player"));
	if (PlayerFinder.Succeeded())
	{
		DefaultPawnClass = PlayerFinder.Class;
	}
	else
	{
		static ConstructorHelpers::FClassFinder<APawn> TPFinder(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
		if (TPFinder.Succeeded())
		{
			DefaultPawnClass = TPFinder.Class;
		}
	}
}

void AGOTAStoryGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	ResolveClasses();
	Super::InitGame(MapName, Options, ErrorMessage);

	if (AGameSession* Session = GameSession)
	{
		Session->MaxPlayers = SoftMaxPlayers;
	}

	if (UGameplayStatics::HasOption(Options, TEXT("Checkpoint")))
	{
		ResumeCheckpointId = FName(*UGameplayStatics::ParseOption(Options, TEXT("Checkpoint")));
	}
	if (UGameplayStatics::HasOption(Options, TEXT("listen"))
		|| UGameplayStatics::HasOption(Options, TEXT("Listen")))
	{
		bHostCoopSave = true;
	}
}

void AGOTAStoryGameMode::ResolveClasses()
{
	if (!DefaultPawnClass && !SoftPlayerClass.IsNull())
	{
		DefaultPawnClass = SoftPlayerClass.LoadSynchronous();
	}
}

void AGOTAStoryGameMode::BeginPlay()
{
	Super::BeginPlay();
	ResolveClasses();

	if (!HasAuthority())
	{
		return;
	}

	EnsureWorldInfrastructure();
	StartOrResumeMission();

	GetWorldTimerManager().SetTimer(
		EncounterWatchHandle,
		this,
		&AGOTAStoryGameMode::WatchEncounters,
		0.35f,
		true);
}

void AGOTAStoryGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	PresentHud(NewPlayer);

	if (AGOTAStoryGameState* GS = GetStoryGS())
	{
		UE_LOG(LogTemp, Log, TEXT("Story PostLogin players=%d objective=%d phase=%d"),
			GetNumPlayers(), static_cast<int32>(GS->CurrentObjectiveId), static_cast<int32>(GS->MissionPhase));
	}
}

AActor* AGOTAStoryGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const int32 Slot = NextPlayerSlot % SoftMaxPlayers;
	for (TActorIterator<AArenaPlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerSlot == Slot)
		{
			++NextPlayerSlot;
			return *It;
		}
	}
	++NextPlayerSlot;
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AGOTAStoryGameMode::EnsureWorldInfrastructure()
{
	EnsureGraybox();
	EnsurePlayerStarts();
	EnsureObjectiveActors();
}

void AGOTAStoryGameMode::EnsureGraybox()
{
	if (!bAutoBuildGraybox)
	{
		return;
	}

	auto BindFallenDress = [](AArenaGrayboxBuilder* Builder)
	{
		if (!Builder)
		{
			return;
		}
		const FSoftObjectPath DressPath =
			ArenaVisualDressDefaults::GetCanonicalDressPath(EArenaGrayboxLayout::FallenSanctuary);
		if (DressPath.IsValid())
		{
			Builder->VisualDress = TSoftObjectPtr<UArenaVisualDressDefinition>(DressPath);
		}
	};

	for (TActorIterator<AArenaGrayboxBuilder> It(GetWorld()); It; ++It)
	{
		SpawnedGraybox = *It;
		SpawnedGraybox->Layout = EArenaGrayboxLayout::FallenSanctuary;
		SpawnedGraybox->bSpawnPlayerStarts = false; // Story owns starts near camp.
		BindFallenDress(SpawnedGraybox);
		SpawnedGraybox->BuildLayout();
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedGraybox = GetWorld()->SpawnActor<AArenaGrayboxBuilder>(
		AArenaGrayboxBuilder::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (SpawnedGraybox)
	{
		SpawnedGraybox->Layout = EArenaGrayboxLayout::FallenSanctuary;
		SpawnedGraybox->bSpawnPlayerStarts = false;
		SpawnedGraybox->bBuildOnBeginPlay = false;
		BindFallenDress(SpawnedGraybox);
		SpawnedGraybox->BuildLayout();
	}
}

void AGOTAStoryGameMode::EnsurePlayerStarts()
{
	int32 Existing = 0;
	for (TActorIterator<AArenaPlayerStart> It(GetWorld()); It; ++It)
	{
		++Existing;
	}
	if (Existing >= SoftMaxPlayers)
	{
		return;
	}

	// Four safe camp spawns (SW of courtyard) — matches Fallen Sanctuary graybox camp.
	const TArray<FVector> Locs = {
		FVector(-1450.f, -200.f, 120.f),
		FVector(-1150.f, -200.f, 120.f),
		FVector(-1450.f, -900.f, 120.f),
		FVector(-1150.f, -900.f, 120.f)
	};

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = Existing; i < SoftMaxPlayers && i < Locs.Num(); ++i)
	{
		if (AArenaPlayerStart* Start = GetWorld()->SpawnActor<AArenaPlayerStart>(
			AArenaPlayerStart::StaticClass(), Locs[i], FRotator::ZeroRotator, Params))
		{
			Start->PlayerSlot = i;
			Start->PreferredBiome = EBiomeId::StoneRuins;
		}
	}
}

void AGOTAStoryGameMode::EnsureObjectiveActors()
{
	if (!bAutoPlaceRuntimeObjectives)
	{
		return;
	}

	auto HasObjective = [this](EStoryObjectiveId Id) -> bool
	{
		for (TActorIterator<AGOTAStoryObjectiveActor> It(GetWorld()); It; ++It)
		{
			if (It->ObjectiveId == Id)
			{
				return true;
			}
		}
		return false;
	};

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto SpawnObj = [&](EStoryObjectiveId Id, const FVector& Loc, const FVector& Extent, bool bInteract, FName Checkpoint)
	{
		if (HasObjective(Id))
		{
			return;
		}
		AGOTAStoryObjectiveActor* Obj = GetWorld()->SpawnActor<AGOTAStoryObjectiveActor>(
			AGOTAStoryObjectiveActor::StaticClass(), Loc, FRotator::ZeroRotator, Params);
		if (Obj)
		{
			Obj->ObjectiveId = Id;
			Obj->bRequiresInteraction = bInteract;
			Obj->CheckpointId = Checkpoint;
			if (Obj->TriggerVolume)
			{
				Obj->TriggerVolume->SetBoxExtent(Extent);
			}
		}
	};

	// Gate path south of courtyard
	SpawnObj(EStoryObjectiveId::ReachSanctuaryGates, FVector(0.f, -2600.f, 150.f), FVector(500.f, 200.f, 250.f), false, NAME_None);
	// Shrine near temple — overlap advances for graybox; BP can call TryCompleteFromInteractor for Use-key.
	SpawnObj(EStoryObjectiveId::ActivateDivineShrine, FVector(0.f, 900.f, 120.f), FVector(250.f, 250.f, 200.f), false, TEXT("DivineShrine"));
	// Guardian arena east training clearing
	SpawnObj(EStoryObjectiveId::DefeatSanctuaryGuardian, FVector(1400.f, 200.f, 120.f), FVector(700.f, 700.f, 300.f), false, NAME_None);
}

void AGOTAStoryGameMode::StartOrResumeMission()
{
	AGOTAStoryGameState* GS = GetStoryGS();
	if (!GS)
	{
		return;
	}

	if (!MissionDefinitionAsset.IsNull())
	{
		RuntimeMission = MissionDefinitionAsset.LoadSynchronous();
	}

	GS->MissionId = RuntimeMission ? RuntimeMission->MissionId : FName(TEXT("FallenSanctuary"));
	GS->ChapterId = RuntimeMission ? RuntimeMission->ChapterId : FName(TEXT("Chapter1"));
	GS->bMissionCompleted = false;
	CompletedObjectives.Reset();
	bMissionFlowComplete = false;

	if (ResumeCheckpointId.IsNone())
	{
		if (UGOTAStorySaveGame* Save = LoadCampaignSave())
		{
			if (!Save->bMissionCompleted && !Save->CheckpointId.IsNone())
			{
				ResumeCheckpointId = Save->CheckpointId;
			}
		}
	}

	GS->SetMissionPhase(EStoryMissionPhase::Intro);
	GS->OnStoryMissionStarted.Broadcast(GS->MissionId);

	if (ResumeCheckpointId == TEXT("DivineShrine"))
	{
		CompletedObjectives.Add(EStoryObjectiveId::ReachSanctuaryGates);
		CompletedObjectives.Add(EStoryObjectiveId::SurviveFirstEncounter);
		CompletedObjectives.Add(EStoryObjectiveId::ActivateDivineShrine);
		GS->SetCheckpoint(ResumeCheckpointId);
		GS->SetMissionPhase(EStoryMissionPhase::CheckpointReached);
		AdvanceToObjective(EStoryObjectiveId::DefeatSanctuaryGuardian);
		SpawnGuardian();
	}
	else if (ResumeCheckpointId == TEXT("AfterGates"))
	{
		CompletedObjectives.Add(EStoryObjectiveId::ReachSanctuaryGates);
		AdvanceToObjective(EStoryObjectiveId::SurviveFirstEncounter);
		SpawnFirstEncounter();
	}
	else
	{
		AdvanceToObjective(EStoryObjectiveId::ReachSanctuaryGates);
	}

	GS->SetMissionPhase(EStoryMissionPhase::Active);
}

FText AGOTAStoryGameMode::GetObjectiveText(EStoryObjectiveId Id) const
{
	if (RuntimeMission)
	{
		for (const FStoryObjectiveDefinition& Def : RuntimeMission->Objectives)
		{
			if (Def.ObjectiveId == Id && !Def.DisplayText.IsEmpty())
			{
				return Def.DisplayText;
			}
		}
	}
	switch (Id)
	{
	case EStoryObjectiveId::ReachSanctuaryGates:
		return NSLOCTEXT("GOTA", "Obj_Gates", "Reach the Sanctuary Gates");
	case EStoryObjectiveId::SurviveFirstEncounter:
		return NSLOCTEXT("GOTA", "Obj_Encounter", "Clear Perimeter Hostiles");
	case EStoryObjectiveId::ActivateDivineShrine:
		return NSLOCTEXT("GOTA", "Obj_Shrine", "Activate the Ascendant Beacon");
	case EStoryObjectiveId::DefeatSanctuaryGuardian:
		return NSLOCTEXT("GOTA", "Obj_Guardian", "Defeat the Sanctuary Guardian");
	case EStoryObjectiveId::MissionComplete:
		return NSLOCTEXT("GOTA", "Obj_Done", "Site Secured — Report to Command");
	default:
		return FText::GetEmpty();
	}
}

FStoryObjectiveProgress AGOTAStoryGameMode::MakeProgress(EStoryObjectiveId Id, int32 Current, int32 Required, bool bDone) const
{
	FStoryObjectiveProgress P;
	P.ObjectiveId = Id;
	P.DisplayText = GetObjectiveText(Id);
	P.CurrentCount = Current;
	P.RequiredCount = Required;
	P.bCompleted = bDone;
	return P;
}

void AGOTAStoryGameMode::AdvanceToObjective(EStoryObjectiveId NextId)
{
	AGOTAStoryGameState* GS = GetStoryGS();
	if (!GS || !HasAuthority())
	{
		return;
	}
	const int32 Required = (NextId == EStoryObjectiveId::SurviveFirstEncounter)
		? (RuntimeMission ? RuntimeMission->FirstEncounterEnemyCount : 3)
		: 1;
	GS->SetCurrentObjective(MakeProgress(NextId, 0, Required, false));
	GS->EncounterEnemiesRemaining = (NextId == EStoryObjectiveId::SurviveFirstEncounter
		|| NextId == EStoryObjectiveId::DefeatSanctuaryGuardian) ? Required : 0;

	UE_LOG(LogTemp, Log, TEXT("Story objective -> %d"), static_cast<int32>(NextId));
}

bool AGOTAStoryGameMode::CanAdvanceObjective(EStoryObjectiveId Id) const
{
	const AGOTAStoryGameState* GS = GetStoryGS();
	if (!GS || GS->MissionPhase == EStoryMissionPhase::Completed || bMissionFlowComplete)
	{
		return false;
	}
	if (CompletedObjectives.Contains(Id))
	{
		return false;
	}
	return GS->CurrentObjectiveId == Id;
}

void AGOTAStoryGameMode::NotifyObjectiveActorCompleted(AGOTAStoryObjectiveActor* ObjectiveActor, AActor* InstigatorActor)
{
	if (!HasAuthority() || !ObjectiveActor)
	{
		return;
	}

	const EStoryObjectiveId Id = ObjectiveActor->ObjectiveId;
	if (!CanAdvanceObjective(Id))
	{
		return;
	}

	if (Id == EStoryObjectiveId::ReachSanctuaryGates)
	{
		CompleteCurrentObjective();
		ActivateCheckpoint(TEXT("AfterGates"));
		AdvanceToObjective(EStoryObjectiveId::SurviveFirstEncounter);
		SpawnFirstEncounter();
		return;
	}

	if (Id == EStoryObjectiveId::ActivateDivineShrine)
	{
		CompleteCurrentObjective();
		ActivateCheckpoint(ObjectiveActor->CheckpointId.IsNone() ? FName(TEXT("DivineShrine")) : ObjectiveActor->CheckpointId);
		AdvanceToObjective(EStoryObjectiveId::DefeatSanctuaryGuardian);
		SpawnGuardian();
		return;
	}

	if (Id == EStoryObjectiveId::DefeatSanctuaryGuardian)
	{
		// Volume reach alone does not finish — combat watch handles kill.
		return;
	}
}

void AGOTAStoryGameMode::CompleteCurrentObjective()
{
	AGOTAStoryGameState* GS = GetStoryGS();
	if (!GS)
	{
		return;
	}
	FStoryObjectiveProgress Done = GS->CurrentObjective;
	Done.bCompleted = true;
	Done.CurrentCount = Done.RequiredCount;
	CompletedObjectives.Add(Done.ObjectiveId);
	GS->SetCurrentObjective(Done);
}

void AGOTAStoryGameMode::SpawnFirstEncounter()
{
	UClass* EnemyClass = SoftEnemyClass.LoadSynchronous();
	if (!EnemyClass)
	{
		EnemyClass = StaticLoadClass(APawn::StaticClass(), nullptr, TEXT("/Game/Enemies/BP_Enemy_Melee.BP_Enemy_Melee_C"));
	}
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Story: no melee enemy class — auto-completing encounter for progression."));
		CompleteCurrentObjective();
		AdvanceToObjective(EStoryObjectiveId::ActivateDivineShrine);
		return;
	}

	EncounterEnemies.Reset();
	const int32 Count = RuntimeMission ? RuntimeMission->FirstEncounterEnemyCount : 3;
	const FVector Center(1400.f, 200.f, 120.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < Count; ++i)
	{
		const float Ang = i * (2.f * PI / FMath::Max(Count, 1));
		const FVector Loc = Center + FVector(FMath::Cos(Ang) * 350.f, FMath::Sin(Ang) * 350.f, 0.f);
		if (APawn* Enemy = GetWorld()->SpawnActor<APawn>(EnemyClass, Loc, FRotator::ZeroRotator, Params))
		{
			EncounterEnemies.Add(Enemy);
		}
	}

	if (AGOTAStoryGameState* GS = GetStoryGS())
	{
		GS->EncounterEnemiesRemaining = EncounterEnemies.Num();
		GS->SetCurrentObjective(MakeProgress(EStoryObjectiveId::SurviveFirstEncounter, 0, EncounterEnemies.Num(), false));
	}
}

void AGOTAStoryGameMode::SpawnGuardian()
{
	UClass* GuardianClass = SoftGuardianClass.LoadSynchronous();
	if (!GuardianClass)
	{
		GuardianClass = SoftEnemyClass.LoadSynchronous();
	}
	if (!GuardianClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Story: no guardian class — completing mission."));
		CompleteMission();
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GuardianPawn = GetWorld()->SpawnActor<APawn>(GuardianClass, FVector(1400.f, 200.f, 120.f), FRotator(0.f, 180.f, 0.f), Params);
	if (AGOTAStoryGameState* GS = GetStoryGS())
	{
		GS->EncounterEnemiesRemaining = GuardianPawn ? 1 : 0;
		GS->SetCurrentObjective(MakeProgress(EStoryObjectiveId::DefeatSanctuaryGuardian, 0, 1, false));
	}
}

void AGOTAStoryGameMode::WatchEncounters()
{
	if (!HasAuthority() || bMissionFlowComplete)
	{
		return;
	}
	AGOTAStoryGameState* GS = GetStoryGS();
	if (!GS)
	{
		return;
	}

	if (GS->CurrentObjectiveId == EStoryObjectiveId::SurviveFirstEncounter)
	{
		int32 Alive = 0;
		for (const TObjectPtr<APawn>& Enemy : EncounterEnemies)
		{
			if (Enemy && IsValid(Enemy) && !Enemy->IsActorBeingDestroyed())
			{
				++Alive;
			}
		}
		GS->EncounterEnemiesRemaining = Alive;
		const int32 Required = EncounterEnemies.Num();
		const int32 Defeated = Required - Alive;
		GS->SetCurrentObjective(MakeProgress(EStoryObjectiveId::SurviveFirstEncounter, Defeated, Required, false));

		if (Required > 0 && Alive == 0)
		{
			CompleteCurrentObjective();
			AdvanceToObjective(EStoryObjectiveId::ActivateDivineShrine);
		}
		return;
	}

	if (GS->CurrentObjectiveId == EStoryObjectiveId::DefeatSanctuaryGuardian)
	{
		const bool bAlive = GuardianPawn && IsValid(GuardianPawn) && !GuardianPawn->IsActorBeingDestroyed();
		GS->EncounterEnemiesRemaining = bAlive ? 1 : 0;
		GS->SetCurrentObjective(MakeProgress(EStoryObjectiveId::DefeatSanctuaryGuardian, bAlive ? 0 : 1, 1, false));
		if (!bAlive && GuardianPawn)
		{
			CompleteCurrentObjective();
			CompleteMission();
		}
	}
}

void AGOTAStoryGameMode::ActivateCheckpoint(FName CheckpointId)
{
	if (AGOTAStoryGameState* GS = GetStoryGS())
	{
		GS->SetCheckpoint(CheckpointId);
		GS->SetMissionPhase(EStoryMissionPhase::CheckpointReached);
		GS->SetMissionPhase(EStoryMissionPhase::Active);
	}
	PersistProgress(true);
}

void AGOTAStoryGameMode::CompleteMission()
{
	if (bMissionFlowComplete)
	{
		return;
	}
	bMissionFlowComplete = true;
	GetWorldTimerManager().ClearTimer(EncounterWatchHandle);

	if (AGOTAStoryGameState* GS = GetStoryGS())
	{
		GS->bMissionCompleted = true;
		GS->SetCurrentObjective(MakeProgress(EStoryObjectiveId::MissionComplete, 1, 1, true));
		GS->SetMissionPhase(EStoryMissionPhase::Completed);
		GS->OnStoryMissionCompleted.Broadcast(GS->MissionId);
	}
	PersistProgress(false);
	PresentCompleteUI();
	UE_LOG(LogTemp, Log, TEXT("Story mission complete: Fallen Sanctuary"));
}

FString AGOTAStoryGameMode::GetActiveSaveSlot() const
{
	return bHostCoopSave ? HostSaveSlot : SoloSaveSlot;
}

UGOTAStorySaveGame* AGOTAStoryGameMode::LoadCampaignSave() const
{
	if (UGameplayStatics::DoesSaveGameExist(GetActiveSaveSlot(), 0))
	{
		return Cast<UGOTAStorySaveGame>(UGameplayStatics::LoadGameFromSlot(GetActiveSaveSlot(), 0));
	}
	// Also accept legacy menu slot name for Continue compatibility.
	if (UGameplayStatics::DoesSaveGameExist(TEXT("GOTA_Campaign_Slot"), 0))
	{
		return Cast<UGOTAStorySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("GOTA_Campaign_Slot"), 0));
	}
	return nullptr;
}

void AGOTAStoryGameMode::PersistProgress(bool bAsync)
{
	UGOTAStorySaveGame* Save = Cast<UGOTAStorySaveGame>(
		UGameplayStatics::CreateSaveGameObject(UGOTAStorySaveGame::StaticClass()));
	if (!Save)
	{
		return;
	}

	AGOTAStoryGameState* GS = GetStoryGS();
	Save->SaveVersion = 1;
	Save->ChapterId = GS ? GS->ChapterId : TEXT("Chapter1");
	Save->MissionId = GS ? GS->MissionId : TEXT("FallenSanctuary");
	Save->CheckpointId = GS ? GS->ActiveCheckpointId : NAME_None;
	Save->LastObjectiveId = GS ? GS->CurrentObjectiveId : EStoryObjectiveId::ReachSanctuaryGates;
	Save->bMissionCompleted = GS ? GS->bMissionCompleted : false;
	Save->ProfileTag = bHostCoopSave ? TEXT("Host") : TEXT("Solo");

	const FString Slot = GetActiveSaveSlot();
	if (bAsync)
	{
		FAsyncSaveGameToSlotDelegate Done;
		UGameplayStatics::AsyncSaveGameToSlot(Save, Slot, 0, Done);
		// Mirror legacy menu Continue slot for solo.
		if (!bHostCoopSave)
		{
			UGameplayStatics::AsyncSaveGameToSlot(Save, TEXT("GOTA_Campaign_Slot"), 0, Done);
		}
	}
	else
	{
		UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
		if (!bHostCoopSave)
		{
			UGameplayStatics::SaveGameToSlot(Save, TEXT("GOTA_Campaign_Slot"), 0);
		}
	}
}

void AGOTAStoryGameMode::PresentHud(APlayerController* PC)
{
	if (!PC || !StoryHudWidgetClass.ToSoftObjectPath().IsValid())
	{
		return;
	}
	if (UClass* Cls = StoryHudWidgetClass.LoadSynchronous())
	{
		if (UUserWidget* Hud = CreateWidget<UUserWidget>(PC, Cls))
		{
			Hud->AddToViewport(10);
		}
	}
}

void AGOTAStoryGameMode::PresentCompleteUI()
{
	if (!StoryCompleteWidgetClass.ToSoftObjectPath().IsValid())
	{
		return;
	}
	UClass* Cls = StoryCompleteWidgetClass.LoadSynchronous();
	if (!Cls)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (UUserWidget* W = CreateWidget<UUserWidget>(PC, Cls))
			{
				W->AddToViewport(50);
			}
		}
	}
}

void AGOTAStoryGameMode::RequestReturnToMenu()
{
	if (!HasAuthority())
	{
		return;
	}
	const FString MapPackage = MainMenuMap.GetLongPackageName();
	UGameplayStatics::OpenLevel(this, FName(*MapPackage), true);
}

void AGOTAStoryGameMode::RequestRestartMission()
{
	if (!HasAuthority())
	{
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetMapName()), true,
		TEXT("game=/Script/WarOfGods.GOTAStoryGameMode?Mode=Story"));
}

AGOTAStoryGameState* AGOTAStoryGameMode::GetStoryGS() const
{
	return GetGameState<AGOTAStoryGameState>();
}
