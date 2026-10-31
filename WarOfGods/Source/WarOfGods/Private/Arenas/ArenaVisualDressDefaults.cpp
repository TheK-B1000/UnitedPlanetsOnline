// White Tiger Studios 2024

#include "Arenas/ArenaVisualDressDefaults.h"

#include "Arenas/ArenaVisualDressDefinition.h"
#include "Engine/StaticMesh.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	TSoftObjectPtr<UStaticMesh> SoftMesh(const TCHAR* ObjectPath)
	{
		return TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(ObjectPath));
	}

	void ResetDress(UArenaVisualDressDefinition& Dress, EArenaGrayboxLayout Layout, const TCHAR* Name, const TCHAR* Notes)
	{
		Dress.TargetLayout = Layout;
		Dress.DisplayName = FText::FromString(Name);
		Dress.SourcePackNotes = Notes;
		Dress.bHidePlaceholderWallsWhenDressed = true;
		Dress.DressSlots.Reset();
		Dress.CliffMeshPool.Reset();
		Dress.DebrisMeshPool.Reset();
		Dress.PillarMeshPool.Reset();
	}

	void AddLandmark(
		UArenaVisualDressDefinition& Dress,
		FName SlotId,
		const TCHAR* MeshPath,
		const FVector& Loc,
		const FRotator& Rot,
		const FVector& Scale)
	{
		FArenaDressMeshSlot Slot;
		Slot.SlotId = SlotId;
		Slot.Mesh = SoftMesh(MeshPath);
		Slot.RelativeLocation = Loc;
		Slot.RelativeRotation = Rot;
		Slot.Scale = Scale;
		Slot.bVisualOnly = true;
		Dress.DressSlots.Add(Slot);
	}
}

FSoftObjectPath ArenaVisualDressDefaults::GetCanonicalDressPath(EArenaGrayboxLayout Layout)
{
	switch (Layout)
	{
	case EArenaGrayboxLayout::TitansQuarry:
		return FSoftObjectPath(TEXT("/Game/Arenas/Data/DA_Dress_TitansQuarry.DA_Dress_TitansQuarry"));
	case EArenaGrayboxLayout::AshfallCrucible:
		return FSoftObjectPath(TEXT("/Game/Arenas/Data/DA_Dress_AshfallCrucible.DA_Dress_AshfallCrucible"));
	case EArenaGrayboxLayout::FrostboundCitadel:
		return FSoftObjectPath(TEXT("/Game/Arenas/Data/DA_Dress_FrostboundCitadel.DA_Dress_FrostboundCitadel"));
	case EArenaGrayboxLayout::VerdantSanctuary:
		return FSoftObjectPath(TEXT("/Game/Arenas/Data/DA_Dress_VerdantSanctuary.DA_Dress_VerdantSanctuary"));
	case EArenaGrayboxLayout::SunscarExpanse:
		return FSoftObjectPath(TEXT("/Game/Arenas/Data/DA_Dress_SunscarExpanse.DA_Dress_SunscarExpanse"));
	case EArenaGrayboxLayout::StormbreakCoast:
		return FSoftObjectPath(TEXT("/Game/Arenas/Data/DA_Dress_StormbreakCoast.DA_Dress_StormbreakCoast"));
	case EArenaGrayboxLayout::ShatteredDominion:
		return FSoftObjectPath(TEXT("/Game/Arenas/Data/DA_Dress_ShatteredDominion.DA_Dress_ShatteredDominion"));
	case EArenaGrayboxLayout::FallenSanctuary:
		return FSoftObjectPath(TEXT("/Game/Arenas/Data/DA_Dress_FallenSanctuary.DA_Dress_FallenSanctuary"));
	default:
		return FSoftObjectPath();
	}
}

void ArenaVisualDressDefaults::FillTitansQuarryPilot(UArenaVisualDressDefinition& Dress)
{
	ResetDress(Dress, EArenaGrayboxLayout::TitansQuarry,
		TEXT("Titan's Quarry — Quarry Collections"),
		TEXT("Quarry Collections + Limestone Megascans."));

	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_L_01.SM_Quarry_Cliff_L_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_L_02.SM_Quarry_Cliff_L_02")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_L_03.SM_Quarry_Cliff_L_03")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_M_01.SM_Quarry_Cliff_M_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_M_02.SM_Quarry_Cliff_M_02")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_M_03.SM_Quarry_Cliff_M_03")));

	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_RockPile_S_01.SM_Quarry_RockPile_S_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_RockPile_S_02.SM_Quarry_RockPile_S_02")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Ridge_S_01.SM_Quarry_Ridge_S_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Limestone_Rocks_01/SM_Limestone_Rocks_01.SM_Limestone_Rocks_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Limestone_Rubble_01/SM_Limestone_Rubble_01.SM_Limestone_Rubble_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01.SM_Quarry_Outcrop_01")));

	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Spire_L_01.SM_Quarry_Spire_L_01")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Spire_L_02.SM_Quarry_Spire_L_02")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01.SM_Limestone_Slab_01")));

	AddLandmark(Dress, TEXT("Spire_N"), TEXT("/Game/Geometry/SM_Quarry_Spire_L_01.SM_Quarry_Spire_L_01"),
		FVector(0.f, 2100.f, -40.f), FRotator(0.f, 0.f, 0.f), FVector(0.45f));
	AddLandmark(Dress, TEXT("Spire_S"), TEXT("/Game/Geometry/SM_Quarry_Spire_L_02.SM_Quarry_Spire_L_02"),
		FVector(0.f, -2100.f, -40.f), FRotator(0.f, 180.f, 0.f), FVector(0.4f));
	AddLandmark(Dress, TEXT("Ridge_E"), TEXT("/Game/Geometry/SM_Quarry_Ridge_S_01.SM_Quarry_Ridge_S_01"),
		FVector(2050.f, 0.f, -20.f), FRotator(0.f, 90.f, 0.f), FVector(0.55f));
	AddLandmark(Dress, TEXT("Outcrop_Cover"), TEXT("/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01.SM_Quarry_Outcrop_01"),
		FVector(650.f, 450.f, 0.f), FRotator(0.f, 35.f, 0.f), FVector(1.f));
	AddLandmark(Dress, TEXT("GroundPatch"), TEXT("/Game/Geometry/Custom/SM_RockGround_01.SM_RockGround_01"),
		FVector(0.f, 0.f, -20.f), FRotator::ZeroRotator, FVector(8.f, 8.f, 1.f));
}

void ArenaVisualDressDefaults::FillAshfallPilot(UArenaVisualDressDefinition& Dress)
{
	ResetDress(Dress, EArenaGrayboxLayout::AshfallCrucible,
		TEXT("Ashfall Crucible — Lava Field"),
		TEXT("Lava Field Collections + custom scree."));

	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_Cliff_L_01.SM_Lava_Cliff_L_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_Cliff_L_02.SM_Lava_Cliff_L_02")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_Cliff_L_03.SM_Lava_Cliff_L_03")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_Cliff_L_04.SM_Lava_Cliff_L_04")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_Cliff_M_01.SM_Lava_Cliff_M_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_Cliff_M_02.SM_Lava_Cliff_M_02")));

	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_RockMound_S_01.SM_Lava_RockMound_S_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_RockMound_S_02.SM_Lava_RockMound_S_02")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_RockMound_S_03.SM_Lava_RockMound_S_03")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_RockMound_S_04.SM_Lava_RockMound_S_04")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Custom/SM_Custom_Scree_L_04.SM_Custom_Scree_L_04")));

	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_TreeMound_M_01.SM_Lava_TreeMound_M_01")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_TreeMound_M_02.SM_Lava_TreeMound_M_02")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_TreeMound_M_03.SM_Lava_TreeMound_M_03")));

	AddLandmark(Dress, TEXT("LavaCliff_N"), TEXT("/Game/Geometry/SM_Lava_Cliff_L_01.SM_Lava_Cliff_L_01"),
		FVector(0.f, 2300.f, -60.f), FRotator(0.f, 0.f, 0.f), FVector(0.35f));
	AddLandmark(Dress, TEXT("LavaCliff_S"), TEXT("/Game/Geometry/SM_Lava_Cliff_L_02.SM_Lava_Cliff_L_02"),
		FVector(0.f, -2300.f, -60.f), FRotator(0.f, 180.f, 0.f), FVector(0.35f));
	AddLandmark(Dress, TEXT("BossMound"), TEXT("/Game/Geometry/SM_Lava_TreeMound_M_04.SM_Lava_TreeMound_M_04"),
		FVector(0.f, 0.f, 40.f), FRotator::ZeroRotator, FVector(0.55f));
	AddLandmark(Dress, TEXT("Scree_E"), TEXT("/Game/Geometry/Custom/SM_Custom_Scree_XXL_02.SM_Custom_Scree_XXL_02"),
		FVector(2000.f, 400.f, -30.f), FRotator(0.f, 90.f, 0.f), FVector(0.25f));
}

void ArenaVisualDressDefaults::FillFrostboundPilot(UArenaVisualDressDefinition& Dress)
{
	ResetDress(Dress, EArenaGrayboxLayout::FrostboundCitadel,
		TEXT("Frostbound Citadel — Iceland + Winter Forest"),
		TEXT("Iceland_Environment mountains + WinterForestSet props."));

	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Iceland_Environment/Static_Meshes/SM_Mountain_01.SM_Mountain_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Iceland_Environment/Static_Meshes/SM_Mountain_02.SM_Mountain_02")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Iceland_Environment/Static_Meshes/SM_Mountain_03.SM_Mountain_03")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Iceland_Environment/Static_Meshes/SM_Mountain_04.SM_Mountain_04")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Eroded_Mountain.SM_Iceland_Eroded_Mountain")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_SnowMountain_A.SM_SnowMountain_A")));

	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_SnowPile_A.SM_SnowPile_A")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_SnowPile_B.SM_SnowPile_B")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_WinterRock_A.SM_WinterRock_A")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_WinterRock_C.SM_WinterRock_C")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_RockCluster_A.SM_RockCluster_A")));

	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_WinterTree_A.SM_WinterTree_A")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_WinterTree_C.SM_WinterTree_C")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_WinterTree_E.SM_WinterTree_E")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Crest.SM_Iceland_Crest")));

	AddLandmark(Dress, TEXT("KeepMountain"), TEXT("/Game/Iceland_Environment/Static_Meshes/SM_Mountain_Plateu_01.SM_Mountain_Plateu_01"),
		FVector(0.f, 2400.f, -80.f), FRotator::ZeroRotator, FVector(0.2f));
	AddLandmark(Dress, TEXT("IceMonolith"), TEXT("/Game/WinterForestSet/Meshes/SM_WinterRock_F.SM_WinterRock_F"),
		FVector(0.f, 0.f, 20.f), FRotator::ZeroRotator, FVector(2.5f));
	AddLandmark(Dress, TEXT("SnowYard"), TEXT("/Game/WinterForestSet/Meshes/SM_SnowPile_C.SM_SnowPile_C"),
		FVector(500.f, -400.f, 0.f), FRotator::ZeroRotator, FVector(1.5f));
}

void ArenaVisualDressDefaults::FillVerdantPilot(UArenaVisualDressDefinition& Dress)
{
	ResetDress(Dress, EArenaGrayboxLayout::VerdantSanctuary,
		TEXT("Verdant Sanctuary — Beech / Maple / Moss"),
		TEXT("EuropeanBeech + NorwayMaple SimpleWind + moss ground."));

	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Custom/SM_Custom_Cliff_XL_06.SM_Custom_Cliff_XL_06")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Custom/SM_Custom_Cliff_XL_08.SM_Custom_Cliff_XL_08")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Custom/SM_Custom_Plateau_XXL_01.SM_Custom_Plateau_XXL_01")));

	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Sapling_01.SM_EuropeanBeech_Sapling_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Sapling_02.SM_EuropeanBeech_Sapling_02")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Field_01.SM_NorwayMaple_Field_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/A_Surface_Footstep/Geometry/SM_Bush.SM_Bush")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Custom/SM_MossGround_01.SM_MossGround_01")));

	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01.SM_EuropeanBeech_Forest_01")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_03.SM_EuropeanBeech_Forest_03")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_05.SM_EuropeanBeech_Forest_05")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_01.SM_NorwayMaple_Forest_01")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_02.SM_NorwayMaple_Forest_02")));

	AddLandmark(Dress, TEXT("TempleTree_A"), TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_02.SM_EuropeanBeech_Forest_02"),
		FVector(-900.f, 700.f, 0.f), FRotator(0.f, 40.f, 0.f), FVector(1.1f));
	AddLandmark(Dress, TEXT("TempleTree_B"), TEXT("/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_03.SM_NorwayMaple_Forest_03"),
		FVector(950.f, -650.f, 0.f), FRotator(0.f, -25.f, 0.f), FVector(1.f));
	AddLandmark(Dress, TEXT("MossFloor"), TEXT("/Game/Geometry/Custom/SM_MossGround_02.SM_MossGround_02"),
		FVector(0.f, 0.f, -15.f), FRotator::ZeroRotator, FVector(10.f, 10.f, 1.f));
	AddLandmark(Dress, TEXT("Statue"), TEXT("/Game/StarterContent/Props/SM_Statue.SM_Statue"),
		FVector(0.f, 200.f, 40.f), FRotator(0.f, 180.f, 0.f), FVector(2.f));
}

void ArenaVisualDressDefaults::FillSunscarPilot(UArenaVisualDressDefinition& Dress)
{
	ResetDress(Dress, EArenaGrayboxLayout::SunscarExpanse,
		TEXT("Sunscar Expanse — Sandstone / Moab"),
		TEXT("RockSandstone Megascans + Geometry sand/succulents."));

	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/CanyonSandstone/SM_CanyonSandstone.SM_CanyonSandstone")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_00.SM_RockSandstone_00")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_01.SM_RockSandstone_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_02.SM_RockSandstone_02")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_03.SM_RockSandstone_03")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_05.SM_RockSandstone_05")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Custom/SM_Custom_Cliff_XL_08.SM_Custom_Cliff_XL_08")));

	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_07.SM_RockSandstone_07")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_08.SM_RockSandstone_08")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_09.SM_RockSandstone_09")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Succulents/SM_Succulent_Green_Short_01.SM_Succulent_Green_Short_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Succulents/SM_Succulent_Dead_Short_01.SM_Succulent_Dead_Short_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Succulents/SM_Succulent_Red_Short_01.SM_Succulent_Red_Short_01")));

	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_010.SM_RockSandstone_010")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_011.SM_RockSandstone_011")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_012.SM_RockSandstone_012")));

	AddLandmark(Dress, TEXT("Hoodoo_N"), TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_013.SM_RockSandstone_013"),
		FVector(0.f, 2300.f, -20.f), FRotator::ZeroRotator, FVector(1.2f));
	AddLandmark(Dress, TEXT("CanyonBlock"), TEXT("/Game/Megascans/3D_Assets/CanyonSandstone/SM_CanyonSandstone.SM_CanyonSandstone"),
		FVector(2400.f, 0.f, -40.f), FRotator(0.f, 90.f, 0.f), FVector(0.5f));
	AddLandmark(Dress, TEXT("SandFloor"), TEXT("/Game/Geometry/Custom/SM_Sand_01.SM_Sand_01"),
		FVector(0.f, 0.f, -18.f), FRotator::ZeroRotator, FVector(12.f, 12.f, 1.f));
	AddLandmark(Dress, TEXT("SucculentPatch"), TEXT("/Game/Geometry/Succulents/SM_Succulent_Green_Long_01.SM_Succulent_Green_Long_01"),
		FVector(700.f, -600.f, 0.f), FRotator::ZeroRotator, FVector(1.4f));
}

void ArenaVisualDressDefaults::FillStormbreakPilot(UArenaVisualDressDefinition& Dress)
{
	ResetDress(Dress, EArenaGrayboxLayout::StormbreakCoast,
		TEXT("Stormbreak Coast — Nordic Dolerite + Water"),
		TEXT("Dolerite cliffs/assemblies + WaterPlane."));

	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01.SM_Dolerite_Cliff_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Cliff_03/SM_Dolerite_Cliff_03.SM_Dolerite_Cliff_03")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Cliff_04/SM_Dolerite_Cliff_04.SM_Dolerite_Cliff_04")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Cliff_05/SM_Dolerite_Cliff_05.SM_Dolerite_Cliff_05")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Cliff_06/SM_Dolerite_Cliff_06.SM_Dolerite_Cliff_06")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/Custom/SM_Custom_Cliff_XL_06.SM_Custom_Cliff_XL_06")));

	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Assembly_01/SM_Dolerite_Assembly_01.SM_Dolerite_Assembly_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Assembly_03/SM_Dolerite_Assembly_03.SM_Dolerite_Assembly_03")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Assembly_04/SM_Dolerite_Assembly_04.SM_Dolerite_Assembly_04")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Thai_Beach_Branch_01/SM_Thai_Beach_Branch_01.SM_Thai_Beach_Branch_01")));

	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Assembly_05/SM_Dolerite_Assembly_05.SM_Dolerite_Assembly_05")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Assembly_06/SM_Dolerite_Assembly_06.SM_Dolerite_Assembly_06")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/StarterContent/Props/SM_PillarFrame300.SM_PillarFrame300")));

	AddLandmark(Dress, TEXT("SeawardCliff"), TEXT("/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01.SM_Dolerite_Cliff_01"),
		FVector(0.f, -1400.f, -80.f), FRotator(0.f, 180.f, 0.f), FVector(0.45f));
	AddLandmark(Dress, TEXT("TowerRock_L"), TEXT("/Game/Megascans/3D_Assets/Dolerite_Assembly_01/SM_Dolerite_Assembly_01.SM_Dolerite_Assembly_01"),
		FVector(-1200.f, 200.f, 0.f), FRotator::ZeroRotator, FVector(0.8f));
	AddLandmark(Dress, TEXT("TowerRock_R"), TEXT("/Game/Megascans/3D_Assets/Dolerite_Assembly_03/SM_Dolerite_Assembly_03.SM_Dolerite_Assembly_03"),
		FVector(1200.f, 200.f, 0.f), FRotator(0.f, 40.f, 0.f), FVector(0.8f));
	AddLandmark(Dress, TEXT("WaterSheet"), TEXT("/Game/Geometry/WaterPlane_01/SM_WaterPlane_01.SM_WaterPlane_01"),
		FVector(0.f, -2200.f, -180.f), FRotator::ZeroRotator, FVector(6.f, 3.f, 1.f));
}

void ArenaVisualDressDefaults::FillDominionPilot(UArenaVisualDressDefinition& Dress)
{
	ResetDress(Dress, EArenaGrayboxLayout::ShatteredDominion,
		TEXT("Shattered Dominion — Multi-biome landmarks"),
		TEXT("Composite landmarks at biome offsets; pools reuse Quarry/Lava/Iceland/Sandstone/Dolerite."));
	Dress.bHidePlaceholderWallsWhenDressed = false; // keep bridges/floors readable

	// Shared pools for center coliseum ring
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_L_01.SM_Quarry_Cliff_L_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_Cliff_L_01.SM_Lava_Cliff_L_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Iceland_Environment/Static_Meshes/SM_Mountain_02.SM_Mountain_02")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01.SM_Dolerite_Cliff_01")));

	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_RockPile_S_01.SM_Quarry_RockPile_S_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Lava_RockMound_S_01.SM_Lava_RockMound_S_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/WinterForestSet/Meshes/SM_WinterRock_A.SM_WinterRock_A")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_01.SM_RockSandstone_01")));

	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Spire_L_01.SM_Quarry_Spire_L_01")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01.SM_EuropeanBeech_Forest_01")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/StarterContent/Props/SM_PillarFrame300.SM_PillarFrame300")));

	// Center
	AddLandmark(Dress, TEXT("ColiseumSpire"), TEXT("/Game/Geometry/SM_Quarry_Spire_L_01.SM_Quarry_Spire_L_01"),
		FVector(0.f, 1600.f, -40.f), FRotator::ZeroRotator, FVector(0.5f));
	// North stone
	AddLandmark(Dress, TEXT("NorthCliff"), TEXT("/Game/Geometry/SM_Quarry_Cliff_L_02.SM_Quarry_Cliff_L_02"),
		FVector(0.f, 6500.f + 1800.f, -60.f), FRotator::ZeroRotator, FVector(0.35f));
	// East forest
	AddLandmark(Dress, TEXT("EastTree"), TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_04.SM_EuropeanBeech_Forest_04"),
		FVector(6500.f + 900.f, 400.f, 0.f), FRotator(0.f, 20.f, 0.f), FVector(1.2f));
	// West volcanic
	AddLandmark(Dress, TEXT("WestLava"), TEXT("/Game/Geometry/SM_Lava_Cliff_L_03.SM_Lava_Cliff_L_03"),
		FVector(-6500.f - 1800.f, 0.f, -60.f), FRotator(0.f, 90.f, 0.f), FVector(0.35f));
	// South coast
	AddLandmark(Dress, TEXT("SouthDolerite"), TEXT("/Game/Megascans/3D_Assets/Dolerite_Cliff_04/SM_Dolerite_Cliff_04.SM_Dolerite_Cliff_04"),
		FVector(0.f, -6500.f - 1200.f, -80.f), FRotator(0.f, 180.f, 0.f), FVector(0.4f));
	AddLandmark(Dress, TEXT("SouthWater"), TEXT("/Game/Geometry/WaterPlane_01/SM_WaterPlane_01.SM_WaterPlane_01"),
		FVector(0.f, -6500.f - 1600.f, -180.f), FRotator::ZeroRotator, FVector(5.f, 2.5f, 1.f));
	// NE frozen
	AddLandmark(Dress, TEXT("NESnow"), TEXT("/Game/WinterForestSet/Meshes/SM_SnowMountain_A.SM_SnowMountain_A"),
		FVector(5200.f, 5200.f + 1400.f, -40.f), FRotator::ZeroRotator, FVector(0.6f));
	// NW desert
	AddLandmark(Dress, TEXT("NWSandstone"), TEXT("/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_00.SM_RockSandstone_00"),
		FVector(-5200.f - 1200.f, 5200.f, -20.f), FRotator(0.f, 45.f, 0.f), FVector(1.1f));
}

void ArenaVisualDressDefaults::FillFallenSanctuaryPilot(UArenaVisualDressDefinition& Dress)
{
	ResetDress(Dress, EArenaGrayboxLayout::FallenSanctuary,
		TEXT("Fallen Sanctuary — Ruins + Forest fringe"),
		TEXT("Limestone / Quarry ruins + Beech fringe + Starter statue/bush."));
	Dress.bHidePlaceholderWallsWhenDressed = false; // keep Chapter 1 bounds

	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_M_01.SM_Quarry_Cliff_M_01")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_Cliff_M_02.SM_Quarry_Cliff_M_02")));
	Dress.CliffMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Limestone_Slab_02/SM_Limestone_Slab_02.SM_Limestone_Slab_02")));

	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Megascans/3D_Assets/Limestone_Rubble_01/SM_Limestone_Rubble_01.SM_Limestone_Rubble_01")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/Geometry/SM_Quarry_RockPile_S_03.SM_Quarry_RockPile_S_03")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/A_Surface_Footstep/Geometry/SM_Bush.SM_Bush")));
	Dress.DebrisMeshPool.Add(SoftMesh(TEXT("/Game/StarterContent/Props/SM_Bush.SM_Bush")));

	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01.SM_EuropeanBeech_Forest_01")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_06.SM_EuropeanBeech_Forest_06")));
	Dress.PillarMeshPool.Add(SoftMesh(TEXT("/Game/StarterContent/Props/SM_PillarFrame.SM_PillarFrame")));

	AddLandmark(Dress, TEXT("TempleStatue"), TEXT("/Game/StarterContent/Props/SM_Statue.SM_Statue"),
		FVector(0.f, 1100.f, 40.f), FRotator(0.f, 180.f, 0.f), FVector(2.5f));
	AddLandmark(Dress, TEXT("FallenSlab"), TEXT("/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01.SM_Limestone_Slab_01"),
		FVector(-400.f, 1400.f, 40.f), FRotator(15.f, 20.f, 8.f), FVector(1.2f));
	AddLandmark(Dress, TEXT("CampBush"), TEXT("/Game/A_Surface_Footstep/Geometry/SM_Bush.SM_Bush"),
		FVector(-1200.f, -500.f, 0.f), FRotator::ZeroRotator, FVector(1.5f));
	AddLandmark(Dress, TEXT("GateSpire_L"), TEXT("/Game/Geometry/SM_Quarry_Spire_L_01.SM_Quarry_Spire_L_01"),
		FVector(-500.f, -2600.f, -20.f), FRotator::ZeroRotator, FVector(0.25f));
	AddLandmark(Dress, TEXT("GateSpire_R"), TEXT("/Game/Geometry/SM_Quarry_Spire_L_02.SM_Quarry_Spire_L_02"),
		FVector(500.f, -2600.f, -20.f), FRotator(0.f, 180.f, 0.f), FVector(0.25f));
	AddLandmark(Dress, TEXT("ColiseumSilhouette"), TEXT("/Game/Geometry/Custom/SM_Custom_Plateau_XXL_01.SM_Custom_Plateau_XXL_01"),
		FVector(0.f, -9000.f, -40.f), FRotator::ZeroRotator, FVector(0.35f));
}

UArenaVisualDressDefinition* ArenaVisualDressDefaults::ResolveDress(
	EArenaGrayboxLayout Layout,
	const TSoftObjectPtr<UArenaVisualDressDefinition>& Assigned,
	UObject* TransientOuter)
{
	if (!Assigned.IsNull())
	{
		if (UArenaVisualDressDefinition* Loaded = Assigned.LoadSynchronous())
		{
			// Stale VisualDress after gota.BuildArena layout switches must not win over the biome DA.
			if (Loaded->TargetLayout == Layout)
			{
				return Loaded;
			}
			UE_LOG(LogTemp, Warning, TEXT("ArenaVisualDress: ignoring assigned %s (TargetLayout %d != %d); trying canonical"),
				*Loaded->GetName(), static_cast<int32>(Loaded->TargetLayout), static_cast<int32>(Layout));
		}
	}

	const FSoftObjectPath Canonical = GetCanonicalDressPath(Layout);
	if (Canonical.IsValid())
	{
		if (UArenaVisualDressDefinition* FromDisk = Cast<UArenaVisualDressDefinition>(Canonical.TryLoad()))
		{
			return FromDisk;
		}
	}

	if (!TransientOuter)
	{
		return nullptr;
	}

	const FName TransientName = *FString::Printf(TEXT("TransientDress_%d"), static_cast<int32>(Layout));
	UArenaVisualDressDefinition* Transient = NewObject<UArenaVisualDressDefinition>(TransientOuter, TransientName);

	switch (Layout)
	{
	case EArenaGrayboxLayout::TitansQuarry:
		FillTitansQuarryPilot(*Transient);
		break;
	case EArenaGrayboxLayout::AshfallCrucible:
		FillAshfallPilot(*Transient);
		break;
	case EArenaGrayboxLayout::FrostboundCitadel:
		FillFrostboundPilot(*Transient);
		break;
	case EArenaGrayboxLayout::VerdantSanctuary:
		FillVerdantPilot(*Transient);
		break;
	case EArenaGrayboxLayout::SunscarExpanse:
		FillSunscarPilot(*Transient);
		break;
	case EArenaGrayboxLayout::StormbreakCoast:
		FillStormbreakPilot(*Transient);
		break;
	case EArenaGrayboxLayout::ShatteredDominion:
		FillDominionPilot(*Transient);
		break;
	case EArenaGrayboxLayout::FallenSanctuary:
		FillFallenSanctuaryPilot(*Transient);
		break;
	default:
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("ArenaVisualDress: transient dress for layout %d (%s)"),
		static_cast<int32>(Layout), *Transient->DisplayName.ToString());
	return Transient;
}
