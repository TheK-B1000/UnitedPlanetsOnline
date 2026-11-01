// White Tiger Studios 2024

#include "Characters/GOTAEnemyDefinition.h"
#include "Characters/GOTAHeroBodyComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAEnemyDefinition)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAEnemies, Log, All);

static TAutoConsoleVariable<int32> CVarGOTAEnemyBodies(
	TEXT("gota.Enemies.Bodies"), 1,
	TEXT("1 = enemies wear their roster bodies (default). 0 = leave enemy combat frames undressed (diagnostics only)."));

int32 UGOTAEnemyBodySubsystem::SuppressAutoDress = 0;

bool UGOTAEnemyBodySubsystem::AreEnemyBodiesEnabled()
{
	return CVarGOTAEnemyBodies.GetValueOnGameThread() != 0;
}

void UGOTAEnemyRegistry::EnsureLoaded() const
{
	if (bLoaded)
	{
		return;
	}
	bLoaded = true;
	UAssetManager* Manager = UAssetManager::GetIfInitialized();
	if (!Manager)
	{
		return;
	}
	TArray<FSoftObjectPath> Paths;
	Manager->GetPrimaryAssetPathList(FPrimaryAssetType(TEXT("GOTAEnemy")), Paths);
	for (const FSoftObjectPath& Path : Paths)
	{
		if (UGOTAEnemyDefinition* Def = Cast<UGOTAEnemyDefinition>(Path.TryLoad()))
		{
			if (!Def->HeroId.IsNone())
			{
				Enemies.Add(Def);
			}
		}
	}
	UE_LOG(LogGOTAEnemies, Log, TEXT("Enemy registry: %d enemy bodies"), Enemies.Num());
}

const UGOTAEnemyDefinition* UGOTAEnemyRegistry::FindEnemy(FName EnemyId) const
{
	EnsureLoaded();
	for (const UGOTAEnemyDefinition* Def : Enemies)
	{
		if (Def && Def->HeroId == EnemyId)
		{
			return Def;
		}
	}
	return nullptr;
}

const UGOTAEnemyDefinition* UGOTAEnemyRegistry::FindAutoDressForClass(const UClass* PawnClass) const
{
	EnsureLoaded();
	if (!PawnClass)
	{
		return nullptr;
	}
	for (const UGOTAEnemyDefinition* Def : Enemies)
	{
		if (Def && Def->bAutoDressPawnClass && !Def->PawnClass.IsNull() && Def->PawnClass.ToSoftObjectPath() == FSoftObjectPath(PawnClass))
		{
			return Def;
		}
	}
	return nullptr;
}

TArray<const UGOTAEnemyDefinition*> UGOTAEnemyRegistry::GetEnemies() const
{
	EnsureLoaded();
	TArray<const UGOTAEnemyDefinition*> Out;
	for (const UGOTAEnemyDefinition* Def : Enemies)
	{
		Out.Add(Def);
	}
	return Out;
}

bool UGOTAEnemyBodySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UGOTAEnemyBodySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() == NM_Client)
	{
		return; // bodies are chosen by the server; clients rebuild them from the replicated id
	}
	SpawnHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UGOTAEnemyBodySubsystem::HandleActorSpawned));
	for (TActorIterator<APawn> It(&InWorld); It; ++It)
	{
		DressPawn(*It); // enemies placed in the level
	}
}

void UGOTAEnemyBodySubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(SpawnHandle);
	}
	Super::Deinitialize();
}

void UGOTAEnemyBodySubsystem::HandleActorSpawned(AActor* Actor)
{
	if (SuppressAutoDress > 0)
	{
		return; // e.g. the Fight AI champion: dressed once by ADuelGameMode (never twice in one frame)
	}
	DressPawn(Cast<APawn>(Actor));
}

void UGOTAEnemyBodySubsystem::DressPawn(APawn* Pawn)
{
	if (!Pawn || !Pawn->HasAuthority() || Pawn->IsPlayerControlled() || !AreEnemyBodiesEnabled())
	{
		return;
	}
	const UGameInstance* GI = Pawn->GetGameInstance();
	const UGOTAEnemyRegistry* Registry = GI ? GI->GetSubsystem<UGOTAEnemyRegistry>() : nullptr;
	const UGOTAEnemyDefinition* Def = Registry ? Registry->FindAutoDressForClass(Pawn->GetClass()) : nullptr;
	if (!Def)
	{
		return;
	}
	if (const UGOTAHeroBodyComponent* Existing = Pawn->FindComponentByClass<UGOTAHeroBodyComponent>())
	{
		if (!Existing->GetHeroId().IsNone())
		{
			return; // already dressed (e.g. explicitly as the Fight AI champion)
		}
	}
	if (!UGOTAHeroBodyComponent::ApplyToPawn(Pawn, Def))
	{
		// Never fall back silently: the roster says this enemy has a body, so its absence is an error.
		UE_LOG(LogGOTAEnemies, Error, TEXT("Enemy %s: body %s could not be applied to %s (missing asset?) - the combat frame is showing"),
			*Def->HeroId.ToString(), *Def->BodyMesh.ToString(), *GetNameSafe(Pawn));
	}
}
