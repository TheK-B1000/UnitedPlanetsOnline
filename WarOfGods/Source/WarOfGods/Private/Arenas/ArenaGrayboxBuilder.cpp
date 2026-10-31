// White Tiger Studios 2024

#include "Arenas/ArenaGrayboxBuilder.h"

#include "Arenas/ArenaPlayerStart.h"
#include "Arenas/ArenaVisualDressDefaults.h"
#include "Arenas/ArenaVisualDressDefinition.h"
#include "Arenas/BiomeVolume.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor C_Stone(0.35f, 0.35f, 0.38f);
	const FLinearColor C_Accent(0.55f, 0.45f, 0.25f);
	const FLinearColor C_Lava(0.55f, 0.12f, 0.04f);
	const FLinearColor C_LavaGlow(1.f, 0.35f, 0.05f);
	const FLinearColor C_Ice(0.55f, 0.72f, 0.85f);
	const FLinearColor C_Snow(0.85f, 0.9f, 0.95f);
	const FLinearColor C_Forest(0.18f, 0.42f, 0.18f);
	const FLinearColor C_Sand(0.72f, 0.58f, 0.32f);
	const FLinearColor C_Cliff(0.45f, 0.4f, 0.35f);
	const FLinearColor C_Coast(0.25f, 0.4f, 0.5f);
	const FLinearColor C_Water(0.1f, 0.25f, 0.45f);
	const FLinearColor C_Coliseum(0.4f, 0.38f, 0.35f);
}

AArenaGrayboxBuilder::AArenaGrayboxBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (CubeFinder.Succeeded()) { CubeMesh = CubeFinder.Object; }
	if (CylinderFinder.Succeeded()) { CylinderMesh = CylinderFinder.Object; }
}

void AArenaGrayboxBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AArenaGrayboxBuilder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AArenaGrayboxBuilder, Layout);
}

void AArenaGrayboxBuilder::OnRep_Layout()
{
	BuildLayout();
}

void AArenaGrayboxBuilder::BeginPlay()
{
	Super::BeginPlay();
	if (bBuildOnBeginPlay)
	{
		BuildLayout();
	}
}

void AArenaGrayboxBuilder::EnsureMeshes()
{
	if (!CubeMesh) { CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")); }
	if (!CylinderMesh) { CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
}

void AArenaGrayboxBuilder::ClearBuiltActors()
{
	for (UStaticMeshComponent* Comp : BuiltMeshes)
	{
		if (IsValid(Comp)) { Comp->DestroyComponent(); }
	}
	BuiltMeshes.Reset();
	for (AActor* Actor : BuiltActors)
	{
		if (IsValid(Actor)) { Actor->Destroy(); }
	}
	BuiltActors.Reset();
	bBuilt = false;
}

void AArenaGrayboxBuilder::AddBox(const FVector& Center, const FVector& Extent, const FLinearColor& Color, bool bPlaceholderBoundary)
{
	EnsureMeshes();
	if (!CubeMesh) { return; }

	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	Mesh->SetStaticMesh(CubeMesh);
	if (bPlaceholderBoundary)
	{
		Mesh->ComponentTags.Add(FName(TEXT("GOTA_PlaceholderBoundary")));
	}
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->RegisterComponent();
	// Normalize by the mesh's real local bounds: LevelPrototyping SM_Cube is not centre-pivoted, so
	// placing it at Center unadjusted shifted every graybox box (e.g. the Quarry floor covered only
	// one quadrant and fighters at start 0 fell through the world).
	const FBox LocalBounds = CubeMesh->GetBoundingBox();
	const FVector LocalSize = LocalBounds.GetSize().ComponentMax(FVector(1.f));
	const FVector Scale((Extent.X * 2.f) / LocalSize.X, (Extent.Y * 2.f) / LocalSize.Y, (Extent.Z * 2.f) / LocalSize.Z);
	Mesh->SetWorldScale3D(Scale);
	Mesh->SetWorldLocation(GetActorLocation() + Center - LocalBounds.GetCenter() * Scale);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	if (UMaterialInterface* BaseMat = Mesh->GetMaterial(0))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Mesh->SetMaterial(0, MID);
		}
	}
	BuiltMeshes.Add(Mesh);
}

void AArenaGrayboxBuilder::AddPillar(const FVector& Center, float HeightUU)
{
	EnsureMeshes();
	UStaticMesh* MeshAsset = CylinderMesh ? CylinderMesh.Get() : CubeMesh.Get();
	if (!MeshAsset) { return; }

	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	Mesh->SetStaticMesh(MeshAsset);
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->RegisterComponent();
	// Same pivot normalization as AddBox: pillar base sits on Center regardless of mesh pivot.
	const FBox LocalBounds = MeshAsset->GetBoundingBox();
	const FVector LocalSize = LocalBounds.GetSize().ComponentMax(FVector(1.f));
	const FVector Scale(120.f / LocalSize.X, 120.f / LocalSize.Y, HeightUU / LocalSize.Z);
	Mesh->SetWorldScale3D(Scale);
	Mesh->SetWorldLocation(GetActorLocation() + Center + FVector(0.f, 0.f, HeightUU * 0.5f) - LocalBounds.GetCenter() * Scale);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BuiltMeshes.Add(Mesh);
}

UStaticMesh* AArenaGrayboxBuilder::TryLoadMesh(const TCHAR* Path) const
{
	return LoadObject<UStaticMesh>(nullptr, Path);
}

void AArenaGrayboxBuilder::AddMeshAt(UStaticMesh* MeshAsset, const FVector& Location, const FRotator& Rotation, const FVector& Scale, bool bVisualOnly)
{
	if (!MeshAsset) { return; }
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	Mesh->SetStaticMesh(MeshAsset);
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->RegisterComponent();
	Mesh->SetWorldLocation(GetActorLocation() + Location);
	Mesh->SetWorldRotation(Rotation);
	Mesh->SetWorldScale3D(Scale);
	if (bVisualOnly)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	BuiltMeshes.Add(Mesh);
}

UStaticMesh* AArenaGrayboxBuilder::TryLoadSoftMesh(const TSoftObjectPtr<UStaticMesh>& SoftMesh) const
{
	if (SoftMesh.IsNull())
	{
		return nullptr;
	}
	if (UStaticMesh* Loaded = SoftMesh.Get())
	{
		return Loaded;
	}
	return SoftMesh.LoadSynchronous();
}

void AArenaGrayboxBuilder::PlaceDressPoolRing(const TArray<TSoftObjectPtr<UStaticMesh>>& Pool, float Radius, int32 Count, float Z, float UniformScale)
{
	if (Pool.Num() == 0 || Count <= 0)
	{
		return;
	}
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = (2.f * PI * i) / Count;
		const FVector Loc(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Z);
		const FRotator Rot(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f);
		if (UStaticMesh* Mesh = TryLoadSoftMesh(Pool[i % Pool.Num()]))
		{
			AddMeshAt(Mesh, Loc, Rot, FVector(UniformScale), true);
		}
	}
}

void AArenaGrayboxBuilder::ApplyVisualDress()
{
	if (!bApplyVisualDress)
	{
		return;
	}

	UArenaVisualDressDefinition* Dress = ArenaVisualDressDefaults::ResolveDress(Layout, VisualDress, this);
	if (!Dress)
	{
		return;
	}
	if (Dress->TargetLayout != Layout)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArenaVisualDress %s targets layout %d but builder is %d — applying soft slots with fallback"),
			*Dress->GetName(), static_cast<int32>(Dress->TargetLayout), static_cast<int32>(Layout));
	}

	int32 Placed = 0;
	for (const FArenaDressMeshSlot& Slot : Dress->DressSlots)
	{
		if (UStaticMesh* Mesh = TryLoadSoftMesh(Slot.Mesh))
		{
			AddMeshAt(Mesh, Slot.RelativeLocation, Slot.RelativeRotation, Slot.Scale, Slot.bVisualOnly);
			++Placed;
		}
	}

	float CliffRadius = 2100.f;
	float CliffScale = 0.4f;
	float DebrisRadius = 780.f;
	float DebrisScale = 0.75f;
	float PillarRadius = 1050.f;
	float PillarScale = 0.35f;
	int32 CliffCount = 6;
	int32 DebrisCount = 6;
	int32 PillarCount = 5;

	switch (Layout)
	{
	case EArenaGrayboxLayout::AshfallCrucible:
		CliffRadius = 2300.f; CliffScale = 0.32f; DebrisRadius = 900.f; PillarRadius = 1500.f; PillarScale = 0.45f;
		break;
	case EArenaGrayboxLayout::FrostboundCitadel:
		CliffRadius = 2300.f; CliffScale = 0.22f; DebrisRadius = 850.f; DebrisScale = 1.1f;
		PillarRadius = 1200.f; PillarScale = 0.9f; CliffCount = 5;
		break;
	case EArenaGrayboxLayout::VerdantSanctuary:
		CliffRadius = 2400.f; CliffScale = 0.2f; DebrisRadius = 1000.f; DebrisScale = 1.f;
		PillarRadius = 1500.f; PillarScale = 1.f; PillarCount = 8; DebrisCount = 8;
		break;
	case EArenaGrayboxLayout::SunscarExpanse:
		CliffRadius = 2600.f; CliffScale = 0.85f; DebrisRadius = 1100.f; DebrisScale = 1.f;
		PillarRadius = 1300.f; PillarScale = 1.f;
		break;
	case EArenaGrayboxLayout::StormbreakCoast:
		CliffRadius = 2100.f; CliffScale = 0.35f; DebrisRadius = 900.f; DebrisScale = 0.7f;
		PillarRadius = 1400.f; PillarScale = 0.7f;
		break;
	case EArenaGrayboxLayout::ShatteredDominion:
		CliffRadius = 2200.f; CliffScale = 0.3f; DebrisRadius = 900.f; PillarRadius = 1500.f; PillarScale = 0.4f;
		CliffCount = 8; DebrisCount = 8;
		break;
	case EArenaGrayboxLayout::FallenSanctuary:
		CliffRadius = 2600.f; CliffScale = 0.3f; DebrisRadius = 1100.f; DebrisScale = 1.f;
		PillarRadius = 1800.f; PillarScale = 1.f; PillarCount = 6;
		break;
	default:
		break;
	}

	PlaceDressPoolRing(Dress->CliffMeshPool, CliffRadius, FMath::Min(CliffCount, Dress->CliffMeshPool.Num()), -80.f, CliffScale);
	PlaceDressPoolRing(Dress->DebrisMeshPool, DebrisRadius, FMath::Min(DebrisCount, Dress->DebrisMeshPool.Num()), 20.f, DebrisScale);
	PlaceDressPoolRing(Dress->PillarMeshPool, PillarRadius, FMath::Min(PillarCount, Dress->PillarMeshPool.Num()), 0.f, PillarScale);

	const int32 SoftPlaced = Placed
		+ FMath::Min(CliffCount, Dress->CliffMeshPool.Num())
		+ FMath::Min(DebrisCount, Dress->DebrisMeshPool.Num())
		+ FMath::Min(PillarCount, Dress->PillarMeshPool.Num());

	if (Dress->bHidePlaceholderWallsWhenDressed && SoftPlaced > 0)
	{
		for (UStaticMeshComponent* Comp : BuiltMeshes)
		{
			if (Comp && Comp->ComponentHasTag(FName(TEXT("GOTA_PlaceholderBoundary"))))
			{
				Comp->SetVisibility(false, true);
				// Keep collision so players stay in-bounds.
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ArenaVisualDress applied %d landmarks (+pools) from %s"), Placed, *Dress->GetName());
}

void AArenaGrayboxBuilder::SpawnPlayerStartAt(const FVector& Location, int32 Slot, EBiomeId Biome)
{
	if (!bSpawnPlayerStarts || !GetWorld()) { return; }
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArenaPlayerStart* Start = GetWorld()->SpawnActor<AArenaPlayerStart>(
		AArenaPlayerStart::StaticClass(), GetActorLocation() + Location, FRotator::ZeroRotator, Params);
	if (Start)
	{
		Start->PlayerSlot = Slot;
		Start->PreferredBiome = Biome;
		BuiltActors.Add(Start);
	}
}

void AArenaGrayboxBuilder::SpawnBiomeVolumeAt(const FVector& Center, const FVector& Extent, EBiomeId BiomeId)
{
	if (!bSpawnBiomeVolumes || !GetWorld()) { return; }
	FActorSpawnParameters Params;
	Params.Owner = this;
	ABiomeVolume* Volume = GetWorld()->SpawnActor<ABiomeVolume>(
		ABiomeVolume::StaticClass(), GetActorLocation() + Center, FRotator::ZeroRotator, Params);
	if (Volume)
	{
		Volume->BiomeId = BiomeId;
		Volume->Area->SetBoxExtent(Extent);
		BuiltActors.Add(Volume);
	}
}

void AArenaGrayboxBuilder::SpawnFourCornerStarts(float Radius, float Z, EBiomeId Biome)
{
	SpawnPlayerStartAt(FVector(-Radius, -Radius, Z), 0, Biome);
	SpawnPlayerStartAt(FVector(Radius, -Radius, Z), 1, Biome);
	SpawnPlayerStartAt(FVector(Radius, Radius, Z), 2, Biome);
	SpawnPlayerStartAt(FVector(-Radius, Radius, Z), 3, Biome);
}

void AArenaGrayboxBuilder::AddRingWalls(float HalfExtent, float WallHeight, const FLinearColor& Color)
{
	const float T = 50.f;
	// Tagged so visual dress can hide cube walls once cliffs/meshes replace them (collision stays).
	AddBox(FVector(0.f, HalfExtent + T, WallHeight * 0.5f), FVector(HalfExtent + T * 2.f, T, WallHeight * 0.5f), Color, true);
	AddBox(FVector(0.f, -(HalfExtent + T), WallHeight * 0.5f), FVector(HalfExtent + T * 2.f, T, WallHeight * 0.5f), Color, true);
	AddBox(FVector(HalfExtent + T, 0.f, WallHeight * 0.5f), FVector(T, HalfExtent + T * 2.f, WallHeight * 0.5f), Color, true);
	AddBox(FVector(-(HalfExtent + T), 0.f, WallHeight * 0.5f), FVector(T, HalfExtent + T * 2.f, WallHeight * 0.5f), Color, true);
}

void AArenaGrayboxBuilder::AddRingPillars(float Radius, int32 Count, float Height, const FVector& Center)
{
	for (int32 i = 0; i < Count; ++i)
	{
		const float Ang = i * (2.f * PI / Count);
		AddPillar(Center + FVector(FMath::Cos(Ang) * Radius, FMath::Sin(Ang) * Radius, 0.f), Height);
	}
}

void AArenaGrayboxBuilder::PlaceTreeRing(const FVector& Center, float Radius, int32 Count)
{
	static const TCHAR* TreePaths[] = {
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01.SM_EuropeanBeech_Forest_01"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_02.SM_EuropeanBeech_Forest_02"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_03.SM_EuropeanBeech_Forest_03"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_04.SM_EuropeanBeech_Forest_04"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_05.SM_EuropeanBeech_Forest_05"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_06.SM_EuropeanBeech_Forest_06"),
		TEXT("/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_01.SM_NorwayMaple_Forest_01"),
		TEXT("/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_02.SM_NorwayMaple_Forest_02"),
		TEXT("/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_03.SM_NorwayMaple_Forest_03"),
		TEXT("/Game/WinterForestSet/Meshes/SM_WinterTree_A.SM_WinterTree_A"),
		TEXT("/Game/WinterForestSet/Meshes/SM_WinterTree_C.SM_WinterTree_C"),
	};

	TArray<UStaticMesh*> Trees;
	for (const TCHAR* Path : TreePaths)
	{
		if (UStaticMesh* M = TryLoadMesh(Path)) { Trees.Add(M); }
	}

	for (int32 i = 0; i < Count; ++i)
	{
		const float Ang = i * (2.f * PI / Count);
		const FVector Loc = Center + FVector(FMath::Cos(Ang) * Radius, FMath::Sin(Ang) * Radius, 0.f);
		if (Trees.Num() > 0)
		{
			AddMeshAt(Trees[i % Trees.Num()], Loc, FRotator(0.f, i * 37.f, 0.f), FVector(1.f));
		}
		else
		{
			AddPillar(Loc, 800.f + (i % 4) * 80.f);
		}
	}
}

void AArenaGrayboxBuilder::BuildLayout()
{
	// Always clear so rebuilds (and first build after partial editor spawn) never stack meshes.
	ClearBuiltActors();
	EnsureMeshes();

	switch (Layout)
	{
	case EArenaGrayboxLayout::TitansQuarry: BuildTitansQuarry(); break;
	case EArenaGrayboxLayout::AshfallCrucible: BuildAshfallCrucible(); break;
	case EArenaGrayboxLayout::FrostboundCitadel: BuildFrostboundCitadel(); break;
	case EArenaGrayboxLayout::VerdantSanctuary: BuildVerdantSanctuary(); break;
	case EArenaGrayboxLayout::SunscarExpanse: BuildSunscarExpanse(); break;
	case EArenaGrayboxLayout::StormbreakCoast: BuildStormbreakCoast(); break;
	case EArenaGrayboxLayout::ShatteredDominion: BuildShatteredDominion(); break;
	case EArenaGrayboxLayout::FallenSanctuary: BuildFallenSanctuary(); break;
	}

	ApplyVisualDress();

	bBuilt = true;
	UE_LOG(LogTemp, Log, TEXT("ArenaGrayboxBuilder built %d (%d meshes, %d actors)"),
		static_cast<int32>(Layout), BuiltMeshes.Num(), BuiltActors.Num());
}

// ---------------------------------------------------------------------------
// 1) Titan's Quarry — stone PvP-friendly pit
// ---------------------------------------------------------------------------
void AArenaGrayboxBuilder::BuildTitansQuarry()
{
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2000.f, 2000.f, 50.f), C_Stone);
	AddRingWalls(2000.f, 600.f, C_Stone);
	AddBox(FVector(0.f, 0.f, 80.f), FVector(600.f, 600.f, 80.f), C_Accent);
	AddBox(FVector(1200.f, 0.f, 200.f), FVector(400.f, 150.f, 40.f), C_Stone);
	AddBox(FVector(-1200.f, 0.f, 200.f), FVector(400.f, 150.f, 40.f), C_Stone);
	AddBox(FVector(0.f, 1200.f, 200.f), FVector(150.f, 400.f, 40.f), C_Stone);
	AddBox(FVector(0.f, -1200.f, 200.f), FVector(150.f, 400.f, 40.f), C_Stone);
	AddBox(FVector(800.f, 0.f, 60.f), FVector(200.f, 120.f, 60.f), C_Accent);
	AddBox(FVector(-800.f, 0.f, 60.f), FVector(200.f, 120.f, 60.f), C_Accent);
	AddRingPillars(900.f, 8, 600.f);
	SpawnFourCornerStarts(1500.f, 120.f, EBiomeId::StoneRuins);
	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 500.f), FVector(2200.f, 2200.f, 800.f), EBiomeId::StoneRuins);
}

// ---------------------------------------------------------------------------
// 2) Ashfall Crucible — volcanic, lava channels, bridges
// ---------------------------------------------------------------------------
void AArenaGrayboxBuilder::BuildAshfallCrucible()
{
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2200.f, 2200.f, 50.f), C_Lava);
	AddRingWalls(2200.f, 700.f, C_Cliff);

	// Lava moat ring (visual hazard marker — flat glowing plane)
	AddBox(FVector(0.f, 0.f, 10.f), FVector(900.f, 900.f, 15.f), C_LavaGlow);

	// Cross bridges over lava
	AddBox(FVector(0.f, 0.f, 80.f), FVector(1100.f, 180.f, 40.f), C_Stone);
	AddBox(FVector(0.f, 0.f, 80.f), FVector(180.f, 1100.f, 40.f), C_Stone);

	// Corner islands / raised combat pads
	AddBox(FVector(1400.f, 1400.f, 120.f), FVector(450.f, 450.f, 80.f), C_Cliff);
	AddBox(FVector(-1400.f, 1400.f, 120.f), FVector(450.f, 450.f, 80.f), C_Cliff);
	AddBox(FVector(1400.f, -1400.f, 120.f), FVector(450.f, 450.f, 80.f), C_Cliff);
	AddBox(FVector(-1400.f, -1400.f, 120.f), FVector(450.f, 450.f, 80.f), C_Cliff);

	// Collapsing-bridge lookalikes (narrow spans)
	AddBox(FVector(700.f, 700.f, 100.f), FVector(350.f, 60.f, 30.f), C_Stone);
	AddBox(FVector(-700.f, -700.f, 100.f), FVector(350.f, 60.f, 30.f), C_Stone);

	// Boss pedestal center
	AddBox(FVector(0.f, 0.f, 160.f), FVector(400.f, 400.f, 100.f), C_Accent);
	AddRingPillars(1600.f, 6, 500.f);

	SpawnFourCornerStarts(1600.f, 220.f, EBiomeId::Volcanic);
	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 600.f), FVector(2400.f, 2400.f, 900.f), EBiomeId::Volcanic);
}

// ---------------------------------------------------------------------------
// 3) Frostbound Citadel — ice fortress, open yard, narrow gates
// ---------------------------------------------------------------------------
void AArenaGrayboxBuilder::BuildFrostboundCitadel()
{
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2100.f, 2100.f, 50.f), C_Snow);
	AddRingWalls(2100.f, 900.f, C_Ice);

	// Inner keep walls with gate gaps (north/south openings)
	AddBox(FVector(-900.f, 1200.f, 400.f), FVector(700.f, 80.f, 400.f), C_Ice);
	AddBox(FVector(900.f, 1200.f, 400.f), FVector(700.f, 80.f, 400.f), C_Ice);
	AddBox(FVector(-900.f, -1200.f, 400.f), FVector(700.f, 80.f, 400.f), C_Ice);
	AddBox(FVector(900.f, -1200.f, 400.f), FVector(700.f, 80.f, 400.f), C_Ice);
	AddBox(FVector(1200.f, 0.f, 400.f), FVector(80.f, 1000.f, 400.f), C_Ice);
	AddBox(FVector(-1200.f, 0.f, 400.f), FVector(80.f, 1000.f, 400.f), C_Ice);

	// Courtyard combat floor
	AddBox(FVector(0.f, 0.f, 40.f), FVector(1000.f, 1000.f, 40.f), C_Stone);

	// Ice pillars / shatterable stand-ins
	AddRingPillars(700.f, 8, 450.f);
	AddBox(FVector(0.f, 0.f, 200.f), FVector(250.f, 250.f, 200.f), C_Ice); // central ice monolith

	// Ramparts / elevated walkways
	AddBox(FVector(0.f, 1800.f, 500.f), FVector(1800.f, 100.f, 40.f), C_Ice);
	AddBox(FVector(0.f, -1800.f, 500.f), FVector(1800.f, 100.f, 40.f), C_Ice);

	SpawnFourCornerStarts(1550.f, 120.f, EBiomeId::Frozen);
	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 500.f), FVector(2300.f, 2300.f, 900.f), EBiomeId::Frozen);
}

// ---------------------------------------------------------------------------
// 4) Verdant Sanctuary — forest temple, flanks, ambush rings
// ---------------------------------------------------------------------------
void AArenaGrayboxBuilder::BuildVerdantSanctuary()
{
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2300.f, 2300.f, 50.f), C_Forest);
	AddRingWalls(2300.f, 500.f, C_Cliff);

	// Ruined temple floor + broken walls
	AddBox(FVector(0.f, 0.f, 40.f), FVector(800.f, 800.f, 40.f), C_Stone);
	AddBox(FVector(-600.f, -500.f, 250.f), FVector(80.f, 500.f, 250.f), C_Stone);
	AddBox(FVector(650.f, 400.f, 220.f), FVector(90.f, 450.f, 220.f), C_Stone);
	AddPillar(FVector(400.f, -350.f, 0.f), 700.f);
	AddPillar(FVector(-350.f, 450.f, 0.f), 550.f);

	// Tree ambush rings (Beech/Maple if present)
	PlaceTreeRing(FVector::ZeroVector, 1400.f, 12);
	PlaceTreeRing(FVector::ZeroVector, 1900.f, 16);

	// Flanking alleys
	AddBox(FVector(1600.f, 0.f, 80.f), FVector(200.f, 900.f, 80.f), C_Cliff);
	AddBox(FVector(-1600.f, 0.f, 80.f), FVector(200.f, 900.f, 80.f), C_Cliff);

	SpawnFourCornerStarts(1700.f, 120.f, EBiomeId::Forest);
	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 600.f), FVector(2500.f, 2500.f, 900.f), EBiomeId::Forest);
}

// ---------------------------------------------------------------------------
// 5) Sunscar Expanse — desert colosseum + canyon lanes
// ---------------------------------------------------------------------------
void AArenaGrayboxBuilder::BuildSunscarExpanse()
{
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2500.f, 2500.f, 50.f), C_Sand);
	AddRingWalls(2500.f, 650.f, C_Cliff);

	// Central sandstone colosseum ring
	AddBox(FVector(0.f, 0.f, 40.f), FVector(900.f, 900.f, 40.f), C_Accent);
	AddRingPillars(1100.f, 10, 550.f);

	// Canyon corridors (vertical cover)
	AddBox(FVector(1800.f, 0.f, 300.f), FVector(200.f, 1400.f, 300.f), C_Cliff);
	AddBox(FVector(-1800.f, 0.f, 300.f), FVector(200.f, 1400.f, 300.f), C_Cliff);
	AddBox(FVector(0.f, 1800.f, 250.f), FVector(1200.f, 180.f, 250.f), C_Cliff);
	AddBox(FVector(0.f, -1800.f, 250.f), FVector(1200.f, 180.f, 250.f), C_Cliff);

	// Elevated dune platforms
	AddBox(FVector(900.f, 900.f, 180.f), FVector(350.f, 350.f, 100.f), C_Sand);
	AddBox(FVector(-900.f, -900.f, 180.f), FVector(350.f, 350.f, 100.f), C_Sand);
	AddBox(FVector(500.f, 0.f, 90.f), FVector(250.f, 100.f, 50.f), C_Accent); // ramp stub
	AddBox(FVector(-500.f, 0.f, 90.f), FVector(250.f, 100.f, 50.f), C_Accent);

	SpawnFourCornerStarts(1900.f, 120.f, EBiomeId::Desert);
	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 600.f), FVector(2700.f, 2700.f, 900.f), EBiomeId::Desert);
}

// ---------------------------------------------------------------------------
// 6) Stormbreak Coast — cliffs, towers, water edge
// ---------------------------------------------------------------------------
void AArenaGrayboxBuilder::BuildStormbreakCoast()
{
	// Main cliff shelf (playable)
	AddBox(FVector(0.f, 400.f, -50.f), FVector(2200.f, 1800.f, 50.f), C_Cliff);
	// Water / drop-off south (visual hazard, not playable)
	AddBox(FVector(0.f, -2200.f, -200.f), FVector(2400.f, 800.f, 40.f), C_Water);

	// Seaward cliff wall
	AddBox(FVector(0.f, -900.f, 200.f), FVector(2200.f, 80.f, 250.f), C_Cliff);

	// Ruined coastal towers
	AddPillar(FVector(-1200.f, 200.f, 0.f), 900.f);
	AddPillar(FVector(1200.f, 200.f, 0.f), 900.f);
	AddPillar(FVector(0.f, 1400.f, 0.f), 700.f);
	AddBox(FVector(-1200.f, 200.f, 950.f), FVector(200.f, 200.f, 60.f), C_Stone);
	AddBox(FVector(1200.f, 200.f, 950.f), FVector(200.f, 200.f, 60.f), C_Stone);

	// Fortification walls / choke points
	AddBox(FVector(-700.f, 800.f, 250.f), FVector(80.f, 600.f, 250.f), C_Stone);
	AddBox(FVector(700.f, 800.f, 250.f), FVector(80.f, 600.f, 250.f), C_Stone);
	AddBox(FVector(0.f, 400.f, 40.f), FVector(600.f, 600.f, 40.f), C_Stone); // courtyard

	// Narrow cliffside path east-west
	AddBox(FVector(0.f, -400.f, 80.f), FVector(1800.f, 120.f, 40.f), C_Coast);

	// North bound wall
	AddBox(FVector(0.f, 2200.f, 300.f), FVector(2300.f, 50.f, 300.f), C_Cliff);
	AddBox(FVector(2300.f, 400.f, 300.f), FVector(50.f, 1900.f, 300.f), C_Cliff);
	AddBox(FVector(-2300.f, 400.f, 300.f), FVector(50.f, 1900.f, 300.f), C_Cliff);

	SpawnPlayerStartAt(FVector(-1500.f, 1200.f, 120.f), 0, EBiomeId::Coast);
	SpawnPlayerStartAt(FVector(1500.f, 1200.f, 120.f), 1, EBiomeId::Coast);
	SpawnPlayerStartAt(FVector(800.f, 0.f, 120.f), 2, EBiomeId::Coast);
	SpawnPlayerStartAt(FVector(-800.f, 0.f, 120.f), 3, EBiomeId::Coast);
	SpawnBiomeVolumeAt(FVector(0.f, 400.f, 600.f), FVector(2400.f, 2000.f, 900.f), EBiomeId::Coast);
}

// ---------------------------------------------------------------------------
// 7) Shattered Dominion — 6 connected biomes + central coliseum
// ---------------------------------------------------------------------------
void AArenaGrayboxBuilder::BuildShatteredDominion()
{
	// Center coliseum
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2500.f, 2500.f, 50.f), C_Coliseum);
	AddBox(FVector(0.f, 0.f, 40.f), FVector(1200.f, 1200.f, 40.f), C_Coliseum);
	AddRingPillars(1400.f, 12, 800.f);
	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 600.f), FVector(2500.f, 2500.f, 900.f), EBiomeId::Coliseum);

	auto Bridge = [this](const FVector& Mid, const FVector& Extent, const FLinearColor& Color)
	{
		AddBox(Mid, Extent, Color);
	};

	// North — Stone Ruins
	{
		const FVector C(0.f, 6500.f, 0.f);
		AddBox(C + FVector(0.f, 0.f, -50.f), FVector(2200.f, 2200.f, 50.f), C_Stone);
		AddBox(C + FVector(-700.f, -300.f, 200.f), FVector(100.f, 600.f, 200.f), C_Stone);
		AddBox(C + FVector(600.f, 400.f, 250.f), FVector(120.f, 500.f, 250.f), C_Stone);
		AddPillar(C + FVector(400.f, -500.f, 0.f), 700.f);
		Bridge(FVector(0.f, 3800.f, 40.f), FVector(300.f, 1200.f, 40.f), C_Stone);
		SpawnBiomeVolumeAt(C + FVector(0.f, 0.f, 600.f), FVector(2300.f, 2300.f, 900.f), EBiomeId::StoneRuins);
	}

	// East — Forest
	{
		const FVector C(6500.f, 0.f, 0.f);
		AddBox(C + FVector(0.f, 0.f, -50.f), FVector(2200.f, 2200.f, 50.f), C_Forest);
		PlaceTreeRing(C, 1200.f, 14);
		Bridge(FVector(3800.f, 0.f, 40.f), FVector(1200.f, 300.f, 40.f), C_Forest);
		SpawnBiomeVolumeAt(C + FVector(0.f, 0.f, 600.f), FVector(2300.f, 2300.f, 900.f), EBiomeId::Forest);
	}

	// West — Volcanic
	{
		const FVector C(-6500.f, 0.f, 0.f);
		AddBox(C + FVector(0.f, 0.f, -50.f), FVector(2200.f, 2200.f, 50.f), C_Lava);
		AddBox(C + FVector(0.f, 0.f, 20.f), FVector(700.f, 700.f, 25.f), C_LavaGlow);
		AddBox(C + FVector(800.f, 0.f, 100.f), FVector(500.f, 120.f, 40.f), C_Stone);
		Bridge(FVector(-3800.f, 0.f, 40.f), FVector(1200.f, 300.f, 40.f), C_Lava);
		SpawnBiomeVolumeAt(C + FVector(0.f, 0.f, 600.f), FVector(2300.f, 2300.f, 900.f), EBiomeId::Volcanic);
	}

	// South — Coast
	{
		const FVector C(0.f, -6500.f, 0.f);
		AddBox(C + FVector(0.f, 400.f, -50.f), FVector(2200.f, 1800.f, 50.f), C_Cliff);
		AddBox(C + FVector(0.f, -1600.f, -180.f), FVector(2200.f, 600.f, 40.f), C_Water);
		AddPillar(C + FVector(-900.f, 200.f, 0.f), 800.f);
		AddPillar(C + FVector(900.f, 200.f, 0.f), 800.f);
		Bridge(FVector(0.f, -3800.f, 40.f), FVector(300.f, 1200.f, 40.f), C_Coast);
		SpawnBiomeVolumeAt(C + FVector(0.f, 200.f, 600.f), FVector(2300.f, 2100.f, 900.f), EBiomeId::Coast);
	}

	// Northeast — Frozen (between ruins & forest)
	{
		const FVector C(5200.f, 5200.f, 0.f);
		AddBox(C + FVector(0.f, 0.f, -50.f), FVector(1800.f, 1800.f, 50.f), C_Snow);
		AddBox(C + FVector(0.f, 0.f, 40.f), FVector(700.f, 700.f, 40.f), C_Ice);
		AddRingPillars(900.f, 6, 500.f, C);
		// Diagonal approach bridges
		Bridge(FVector(2800.f, 2800.f, 40.f), FVector(900.f, 160.f, 40.f), C_Ice);
		SpawnBiomeVolumeAt(C + FVector(0.f, 0.f, 600.f), FVector(1900.f, 1900.f, 900.f), EBiomeId::Frozen);
	}

	// Northwest — Desert (between ruins & volcano)
	{
		const FVector C(-5200.f, 5200.f, 0.f);
		AddBox(C + FVector(0.f, 0.f, -50.f), FVector(1800.f, 1800.f, 50.f), C_Sand);
		AddBox(C + FVector(0.f, 0.f, 40.f), FVector(600.f, 600.f, 40.f), C_Accent);
		AddBox(C + FVector(700.f, 0.f, 200.f), FVector(120.f, 800.f, 200.f), C_Cliff);
		Bridge(FVector(-2800.f, 2800.f, 40.f), FVector(900.f, 160.f, 40.f), C_Sand);
		SpawnBiomeVolumeAt(C + FVector(0.f, 0.f, 600.f), FVector(1900.f, 1900.f, 900.f), EBiomeId::Desert);
	}

	SpawnFourCornerStarts(1800.f, 120.f, EBiomeId::Coliseum);
}

// ---------------------------------------------------------------------------
// Story Prologue — The Fallen Sanctuary (near Pantheon Coliseum outer gates)
// ---------------------------------------------------------------------------
void AArenaGrayboxBuilder::BuildFallenSanctuary()
{
	// Sanctuary grounds (playable intro space)
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2800.f, 3200.f, 50.f), C_Stone);
	AddBox(FVector(0.f, 0.f, 20.f), FVector(1200.f, 1400.f, 25.f), C_Accent); // courtyard flagstones

	// Ruined temple (north) — broken walls + collapsed roof slabs
	AddBox(FVector(0.f, 1600.f, 350.f), FVector(900.f, 80.f, 350.f), C_Stone);
	AddBox(FVector(-900.f, 1100.f, 300.f), FVector(80.f, 500.f, 300.f), C_Stone);
	AddBox(FVector(900.f, 1100.f, 280.f), FVector(80.f, 450.f, 280.f), C_Stone);
	AddBox(FVector(-400.f, 1400.f, 520.f), FVector(350.f, 200.f, 40.f), C_Stone); // fallen roof
	AddPillar(FVector(-500.f, 900.f, 0.f), 650.f);
	AddPillar(FVector(500.f, 900.f, 0.f), 520.f);
	AddPillar(FVector(-200.f, 1200.f, 0.f), 380.f); // broken stump look via short height

	// Warrior encampment (southwest) — tents as boxes + fire ring
	AddBox(FVector(-1400.f, -400.f, 120.f), FVector(280.f, 200.f, 120.f), C_Cliff);
	AddBox(FVector(-1100.f, -700.f, 100.f), FVector(220.f, 180.f, 100.f), C_Cliff);
	AddBox(FVector(-1600.f, -700.f, 90.f), FVector(180.f, 160.f, 90.f), C_Cliff);
	AddBox(FVector(-1300.f, -550.f, 40.f), FVector(80.f, 80.f, 40.f), C_LavaGlow); // campfire marker
	AddBox(FVector(-1000.f, -300.f, 60.f), FVector(150.f, 60.f, 40.f), C_Accent); // supply crates

	// Training / first-encounter clearing (east of courtyard)
	AddBox(FVector(1400.f, 200.f, 40.f), FVector(700.f, 700.f, 40.f), C_Forest);
	AddPillar(FVector(1800.f, 600.f, 0.f), 400.f);
	AddPillar(FVector(1100.f, -100.f, 0.f), 350.f);

	// Path toward the Coliseum gates (south)
	AddBox(FVector(0.f, -1800.f, 40.f), FVector(350.f, 900.f, 30.f), C_Stone);
	AddBox(FVector(-500.f, -2600.f, 250.f), FVector(120.f, 80.f, 250.f), C_Coliseum); // gate pillar L
	AddBox(FVector(500.f, -2600.f, 250.f), FVector(120.f, 80.f, 250.f), C_Coliseum);  // gate pillar R
	AddBox(FVector(0.f, -2700.f, 450.f), FVector(600.f, 60.f, 80.f), C_Coliseum);     // lintel / arch

	// Distant Pantheon Coliseum silhouette (far south — visual destination, not playable yet)
	{
		const FVector Col(0.f, -9000.f, 0.f);
		AddBox(Col + FVector(0.f, 0.f, -50.f), FVector(3500.f, 3500.f, 40.f), C_Coliseum);
		AddRingPillars(2800.f, 16, 1200.f, Col);
		AddBox(Col + FVector(0.f, 0.f, 80.f), FVector(1600.f, 1600.f, 60.f), C_Accent);
		SpawnBiomeVolumeAt(Col + FVector(0.f, 0.f, 800.f), FVector(3600.f, 3600.f, 1200.f), EBiomeId::Coliseum);
	}

	// Soft forest fringe (reuse Beech/Maple when present)
	PlaceTreeRing(FVector(-2000.f, 800.f, 0.f), 600.f, 8);
	PlaceTreeRing(FVector(2000.f, 1000.f, 0.f), 700.f, 8);

	// Bound walls (keep players in sanctuary for Chapter 1)
	AddBox(FVector(0.f, 3300.f, 400.f), FVector(2900.f, 50.f, 400.f), C_Cliff);
	AddBox(FVector(2900.f, 0.f, 400.f), FVector(50.f, 3300.f, 400.f), C_Cliff);
	AddBox(FVector(-2900.f, 0.f, 400.f), FVector(50.f, 3300.f, 400.f), C_Cliff);
	// South open toward gates but with soft side rails
	AddBox(FVector(2000.f, -2800.f, 300.f), FVector(50.f, 800.f, 300.f), C_Cliff);
	AddBox(FVector(-2000.f, -2800.f, 300.f), FVector(50.f, 800.f, 300.f), C_Cliff);

	// Four co-op starts near camp — non-overlapping
	SpawnPlayerStartAt(FVector(-1450.f, -200.f, 120.f), 0, EBiomeId::StoneRuins);
	SpawnPlayerStartAt(FVector(-1150.f, -200.f, 120.f), 1, EBiomeId::StoneRuins);
	SpawnPlayerStartAt(FVector(-1450.f, -900.f, 120.f), 2, EBiomeId::StoneRuins);
	SpawnPlayerStartAt(FVector(-1150.f, -900.f, 120.f), 3, EBiomeId::StoneRuins);

	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 600.f), FVector(3000.f, 3400.f, 900.f), EBiomeId::StoneRuins);
}
