// White Tiger Studios 2024

#include "Characters/GOTAHeroRegistry.h"
#include "Characters/GOTAHeroDefinition.h"
#include "Engine/AssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTAHeroes, Log, All);

void UGOTAHeroRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGOTAHeroRegistry::EnsureLoaded() const
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
	Manager->GetPrimaryAssetPathList(FPrimaryAssetType(TEXT("GOTAHero")), Paths);
	for (const FSoftObjectPath& Path : Paths)
	{
		// Definitions are small (text + soft refs); art loads on demand.
		if (UGOTAHeroDefinition* Def = Cast<UGOTAHeroDefinition>(Path.TryLoad()))
		{
			if (Def->HeroId.IsNone())
			{
				UE_LOG(LogGOTAHeroes, Warning, TEXT("Hero definition %s has no HeroId — ignored"), *Path.ToString());
				continue;
			}
			Heroes.Add(Def);
		}
	}
	Heroes.Sort([](const UGOTAHeroDefinition& A, const UGOTAHeroDefinition& B) { return A.SortOrder < B.SortOrder; });
	UE_LOG(LogGOTAHeroes, Log, TEXT("Hero registry: %d heroes"), Heroes.Num());
}

TArray<UGOTAHeroDefinition*> UGOTAHeroRegistry::GetHeroes() const
{
	EnsureLoaded();
	TArray<UGOTAHeroDefinition*> Out;
	for (UGOTAHeroDefinition* Def : Heroes)
	{
		Out.Add(Def);
	}
	return Out;
}

UGOTAHeroDefinition* UGOTAHeroRegistry::FindHero(FName HeroId) const
{
	EnsureLoaded();
	for (UGOTAHeroDefinition* Def : Heroes)
	{
		if (Def && Def->HeroId == HeroId)
		{
			return Def;
		}
	}
	return nullptr;
}

bool UGOTAHeroRegistry::ValidateSelection(FName HeroId, EArenaMode Mode, FText& OutReason) const
{
	const UGOTAHeroDefinition* Def = FindHero(HeroId);
	if (!Def)
	{
		OutReason = NSLOCTEXT("GOTA", "HeroUnknown", "Unknown hero.");
		return false;
	}
	if (!Def->IsPlayable())
	{
		OutReason = Def->UnavailableReason.IsEmpty()
			? NSLOCTEXT("GOTA", "HeroNotPlayable", "This hero does not have a playable character yet.")
			: Def->UnavailableReason;
		return false;
	}
	if (Def->AllowedModes.Num() > 0 && !Def->AllowedModes.Contains(Mode))
	{
		OutReason = NSLOCTEXT("GOTA", "HeroModeBlocked", "This hero is not available in this mode.");
		return false;
	}
	if (!Def->GameplayPawnClass.LoadSynchronous())
	{
		OutReason = NSLOCTEXT("GOTA", "HeroPawnMissing", "This hero's gameplay character could not be loaded.");
		return false;
	}
	OutReason = FText::GetEmpty();
	return true;
}

FName UGOTAHeroRegistry::GetDefaultHero(EArenaMode Mode) const
{
	FText Ignored;
	for (const UGOTAHeroDefinition* Def : GetHeroes())
	{
		if (Def && ValidateSelection(Def->HeroId, Mode, Ignored))
		{
			return Def->HeroId;
		}
	}
	return NAME_None;
}
