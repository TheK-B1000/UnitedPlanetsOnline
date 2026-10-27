// White Tiger Studios 2024

#include "Arenas/ArenaGrayboxBuilder.h"

#include "Arenas/ArenaPlayerStart.h"
#include "Arenas/BiomeVolume.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AArenaGrayboxBuilder::AArenaGrayboxBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
	}
	if (CylinderFinder.Succeeded())
	{
		CylinderMesh = CylinderFinder.Object;
	}
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
	// Deterministic local build on server and clients (meshes are not replicated).
	if (bBuildOnBeginPlay)
	{
		BuildLayout();
	}
}

void AArenaGrayboxBuilder::EnsureMeshes()
{
	if (!CubeMesh)
	{
		CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}
	if (!CylinderMesh)
	{
		CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	}
}

void AArenaGrayboxBuilder::ClearBuiltActors()
{
	for (UStaticMeshComponent* Comp : BuiltMeshes)
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
	}
	BuiltMeshes.Reset();

	for (AActor* Actor : BuiltActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	BuiltActors.Reset();
	bBuilt = false;
}

void AArenaGrayboxBuilder::AddBox(const FVector& Center, const FVector& Extent, const FLinearColor& Color)
{
	EnsureMeshes();
	if (!CubeMesh)
	{
		return;
	}

	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	Mesh->SetStaticMesh(CubeMesh);
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->RegisterComponent();
	Mesh->SetWorldLocation(GetActorLocation() + Center);
	// SM_Cube is often 100uu; scale so final size ~= Extent*2
	const FVector Scale(
		(Extent.X * 2.f) / 100.f,
		(Extent.Y * 2.f) / 100.f,
		(Extent.Z * 2.f) / 100.f);
	Mesh->SetWorldScale3D(Scale);
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
	if (!MeshAsset)
	{
		return;
	}

	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	Mesh->SetStaticMesh(MeshAsset);
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->RegisterComponent();
	Mesh->SetWorldLocation(GetActorLocation() + Center + FVector(0.f, 0.f, HeightUU * 0.5f));
	Mesh->SetWorldScale3D(FVector(1.2f, 1.2f, HeightUU / 100.f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BuiltMeshes.Add(Mesh);
}

UStaticMesh* AArenaGrayboxBuilder::TryLoadMesh(const TCHAR* Path) const
{
	return LoadObject<UStaticMesh>(nullptr, Path);
}

void AArenaGrayboxBuilder::AddMeshAt(UStaticMesh* MeshAsset, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	if (!MeshAsset)
	{
		return;
	}

	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	Mesh->SetStaticMesh(MeshAsset);
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->RegisterComponent();
	Mesh->SetWorldLocation(GetActorLocation() + Location);
	Mesh->SetWorldRotation(Rotation);
	Mesh->SetWorldScale3D(Scale);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BuiltMeshes.Add(Mesh);
}

void AArenaGrayboxBuilder::SpawnPlayerStartAt(const FVector& Location, int32 Slot, EBiomeId Biome)
{
	if (!bSpawnPlayerStarts || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArenaPlayerStart* Start = GetWorld()->SpawnActor<AArenaPlayerStart>(
		AArenaPlayerStart::StaticClass(),
		GetActorLocation() + Location,
		FRotator::ZeroRotator,
		Params);
	if (Start)
	{
		Start->PlayerSlot = Slot;
		Start->PreferredBiome = Biome;
		BuiltActors.Add(Start);
	}
}

void AArenaGrayboxBuilder::SpawnBiomeVolumeAt(const FVector& Center, const FVector& Extent, EBiomeId BiomeId)
{
	if (!bSpawnBiomeVolumes || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	ABiomeVolume* Volume = GetWorld()->SpawnActor<ABiomeVolume>(
		ABiomeVolume::StaticClass(),
		GetActorLocation() + Center,
		FRotator::ZeroRotator,
		Params);
	if (Volume)
	{
		Volume->BiomeId = BiomeId;
		Volume->Area->SetBoxExtent(Extent);
		BuiltActors.Add(Volume);
	}
}

void AArenaGrayboxBuilder::BuildLayout()
{
	if (bBuilt)
	{
		ClearBuiltActors();
	}

	EnsureMeshes();
	switch (Layout)
	{
	case EArenaGrayboxLayout::TitansQuarry:
		BuildTitansQuarry();
		break;
	case EArenaGrayboxLayout::ShatteredDominionThreeBiome:
		BuildShatteredDominionThreeBiome();
		break;
	}
	bBuilt = true;
	UE_LOG(LogTemp, Log, TEXT("ArenaGrayboxBuilder built layout %d (%d meshes, %d actors)"),
		static_cast<int32>(Layout), BuiltMeshes.Num(), BuiltActors.Num());
}

void AArenaGrayboxBuilder::BuildTitansQuarry()
{
	const FLinearColor Stone(0.35f, 0.35f, 0.38f);
	const FLinearColor Accent(0.55f, 0.45f, 0.25f);

	// Main floor ~40m x 40m
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2000.f, 2000.f, 50.f), Stone);

	// Outer walls / bounds
	AddBox(FVector(0.f, 2100.f, 300.f), FVector(2200.f, 50.f, 300.f), Stone);
	AddBox(FVector(0.f, -2100.f, 300.f), FVector(2200.f, 50.f, 300.f), Stone);
	AddBox(FVector(2100.f, 0.f, 300.f), FVector(50.f, 2200.f, 300.f), Stone);
	AddBox(FVector(-2100.f, 0.f, 300.f), FVector(50.f, 2200.f, 300.f), Stone);

	// Central platform
	AddBox(FVector(0.f, 0.f, 80.f), FVector(600.f, 600.f, 80.f), Accent);

	// Raised walkways / cover pillars
	AddBox(FVector(1200.f, 0.f, 200.f), FVector(400.f, 150.f, 40.f), Stone);
	AddBox(FVector(-1200.f, 0.f, 200.f), FVector(400.f, 150.f, 40.f), Stone);
	AddBox(FVector(0.f, 1200.f, 200.f), FVector(150.f, 400.f, 40.f), Stone);
	AddBox(FVector(0.f, -1200.f, 200.f), FVector(150.f, 400.f, 40.f), Stone);

	for (int32 i = 0; i < 8; ++i)
	{
		const float Ang = i * (PI / 4.f);
		const FVector PillarLoc(FMath::Cos(Ang) * 900.f, FMath::Sin(Ang) * 900.f, 0.f);
		AddPillar(PillarLoc, 600.f);
	}

	// Ramps to raised platforms (simple stepped boxes)
	AddBox(FVector(800.f, 0.f, 60.f), FVector(200.f, 120.f, 60.f), Accent);
	AddBox(FVector(-800.f, 0.f, 60.f), FVector(200.f, 120.f, 60.f), Accent);

	SpawnPlayerStartAt(FVector(-1500.f, -1500.f, 120.f), 0, EBiomeId::StoneRuins);
	SpawnPlayerStartAt(FVector(1500.f, -1500.f, 120.f), 1, EBiomeId::StoneRuins);
	SpawnPlayerStartAt(FVector(1500.f, 1500.f, 120.f), 2, EBiomeId::StoneRuins);
	SpawnPlayerStartAt(FVector(-1500.f, 1500.f, 120.f), 3, EBiomeId::StoneRuins);

	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 500.f), FVector(2200.f, 2200.f, 800.f), EBiomeId::StoneRuins);
}

void AArenaGrayboxBuilder::BuildShatteredDominionThreeBiome()
{
	const FLinearColor Coliseum(0.4f, 0.38f, 0.35f);
	const FLinearColor Ruins(0.45f, 0.42f, 0.4f);
	const FLinearColor Forest(0.2f, 0.45f, 0.2f);
	const FLinearColor Volcanic(0.55f, 0.15f, 0.05f);

	// --- Pantheon Coliseum (center) ---
	AddBox(FVector(0.f, 0.f, -50.f), FVector(2500.f, 2500.f, 50.f), Coliseum);
	AddBox(FVector(0.f, 0.f, 40.f), FVector(1200.f, 1200.f, 40.f), Coliseum);
	for (int32 i = 0; i < 12; ++i)
	{
		const float Ang = i * (2.f * PI / 12.f);
		AddPillar(FVector(FMath::Cos(Ang) * 1400.f, FMath::Sin(Ang) * 1400.f, 0.f), 800.f);
	}
	SpawnBiomeVolumeAt(FVector(0.f, 0.f, 600.f), FVector(2500.f, 2500.f, 900.f), EBiomeId::Coliseum);

	// --- Stone Ruins (north) ---
	const FVector RuinsCenter(0.f, 6000.f, 0.f);
	AddBox(RuinsCenter + FVector(0.f, 0.f, -50.f), FVector(2200.f, 2200.f, 50.f), Ruins);
	AddBox(RuinsCenter + FVector(-800.f, -400.f, 200.f), FVector(100.f, 600.f, 200.f), Ruins);
	AddBox(RuinsCenter + FVector(700.f, 500.f, 250.f), FVector(120.f, 500.f, 250.f), Ruins);
	AddPillar(RuinsCenter + FVector(400.f, -600.f, 0.f), 700.f);
	AddPillar(RuinsCenter + FVector(-500.f, 200.f, 0.f), 500.f);
	// Bridge from coliseum to ruins
	AddBox(FVector(0.f, 3500.f, 40.f), FVector(300.f, 1000.f, 40.f), Ruins);
	SpawnBiomeVolumeAt(RuinsCenter + FVector(0.f, 0.f, 600.f), FVector(2300.f, 2300.f, 900.f), EBiomeId::StoneRuins);

	// --- Forest (east) — European Beech when available, else placeholder trunks ---
	const FVector ForestCenter(6000.f, 0.f, 0.f);
	AddBox(ForestCenter + FVector(0.f, 0.f, -50.f), FVector(2200.f, 2200.f, 50.f), Forest);

	static const TCHAR* BeechPaths[] = {
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01.SM_EuropeanBeech_Forest_01"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_02.SM_EuropeanBeech_Forest_02"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_03.SM_EuropeanBeech_Forest_03"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_04.SM_EuropeanBeech_Forest_04"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_05.SM_EuropeanBeech_Forest_05"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_06.SM_EuropeanBeech_Forest_06"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_07.SM_EuropeanBeech_Forest_07"),
		TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_08.SM_EuropeanBeech_Forest_08"),
	};

	TArray<UStaticMesh*> BeechMeshes;
	for (const TCHAR* Path : BeechPaths)
	{
		if (UStaticMesh* M = TryLoadMesh(Path))
		{
			BeechMeshes.Add(M);
		}
	}

	for (int32 i = 0; i < 16; ++i)
	{
		const float X = -1500.f + (i % 4) * 1000.f;
		const float Y = -1500.f + (i / 4) * 1000.f;
		const FVector TreeLoc = ForestCenter + FVector(X, Y, 0.f);
		if (BeechMeshes.Num() > 0)
		{
			UStaticMesh* Tree = BeechMeshes[i % BeechMeshes.Num()];
			AddMeshAt(Tree, TreeLoc, FRotator(0.f, i * 45.f, 0.f), FVector(1.f));
		}
		else
		{
			AddPillar(TreeLoc, 900.f + (i % 3) * 100.f);
		}
	}
	AddBox(FVector(3500.f, 0.f, 40.f), FVector(1000.f, 300.f, 40.f), Forest);
	SpawnBiomeVolumeAt(ForestCenter + FVector(0.f, 0.f, 600.f), FVector(2300.f, 2300.f, 900.f), EBiomeId::Forest);

	// --- Volcanic (west) ---
	const FVector VolcanoCenter(-6000.f, 0.f, 0.f);
	AddBox(VolcanoCenter + FVector(0.f, 0.f, -50.f), FVector(2200.f, 2200.f, 50.f), Volcanic);
	AddBox(VolcanoCenter + FVector(0.f, 0.f, 20.f), FVector(600.f, 600.f, 30.f), FLinearColor(1.f, 0.3f, 0.05f)); // "lava" pit marker
	AddBox(VolcanoCenter + FVector(900.f, -200.f, 150.f), FVector(400.f, 80.f, 40.f), Volcanic); // bridge stub
	AddBox(FVector(-3500.f, 0.f, 40.f), FVector(1000.f, 300.f, 40.f), Volcanic);
	SpawnBiomeVolumeAt(VolcanoCenter + FVector(0.f, 0.f, 600.f), FVector(2300.f, 2300.f, 900.f), EBiomeId::Volcanic);

	// 4 player starts around coliseum
	SpawnPlayerStartAt(FVector(-1800.f, -1800.f, 120.f), 0, EBiomeId::Coliseum);
	SpawnPlayerStartAt(FVector(1800.f, -1800.f, 120.f), 1, EBiomeId::Coliseum);
	SpawnPlayerStartAt(FVector(1800.f, 1800.f, 120.f), 2, EBiomeId::Coliseum);
	SpawnPlayerStartAt(FVector(-1800.f, 1800.f, 120.f), 3, EBiomeId::Coliseum);
}
