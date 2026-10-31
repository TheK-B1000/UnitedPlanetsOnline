# GOTA — Refresh all arena DA_Dress_* assets to match ArenaVisualDressDefaults Fill*Pilot.
# Run via UnrealEditor-Cmd -ExecutePythonScript or Editor Execute Python Script.

import unreal

FOLDER = "/Game/Arenas/Data"

# layout enum int: TitansQuarry=0 ... FallenSanctuary=7
ARENAS = []


def soft_mesh_load(path: str):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_error(f"MISSING {path}")
        return None
    return unreal.EditorAssetLibrary.load_asset(path)


def load_pool(paths):
    return [m for m in (soft_mesh_load(p) for p in paths) if m is not None]


def make_slot(slot_id, mesh_path, x, y, z, pitch, yaw, roll, sx, sy, sz):
    mesh = soft_mesh_load(mesh_path)
    if mesh is None:
        return None
    slot = unreal.ArenaDressMeshSlot()
    slot.set_editor_property("SlotId", unreal.Name(slot_id))
    slot.set_editor_property("Mesh", mesh)
    slot.set_editor_property("RelativeLocation", unreal.Vector(x, y, z))
    slot.set_editor_property("RelativeRotation", unreal.Rotator(pitch, yaw, roll))
    slot.set_editor_property("Scale", unreal.Vector(sx, sy, sz))
    slot.set_editor_property("bVisualOnly", True)
    return slot


def layout_value(name: str):
    try:
        enum = unreal.ArenaGrayboxLayout
        key = name.upper()
        # UE python often exposes TITANS_QUARRY
        mapping = {
            "TitansQuarry": "TITANS_QUARRY",
            "AshfallCrucible": "ASHFALL_CRUCIBLE",
            "FrostboundCitadel": "FROSTBOUND_CITADEL",
            "VerdantSanctuary": "VERDANT_SANCTUARY",
            "SunscarExpanse": "SUNSCAR_EXPANSE",
            "StormbreakCoast": "STORMBREAK_COAST",
            "ShatteredDominion": "SHATTERED_DOMINION",
            "FallenSanctuary": "FALLEN_SANCTUARY",
        }
        return getattr(enum, mapping[name])
    except Exception:
        order = [
            "TitansQuarry",
            "AshfallCrucible",
            "FrostboundCitadel",
            "VerdantSanctuary",
            "SunscarExpanse",
            "StormbreakCoast",
            "ShatteredDominion",
            "FallenSanctuary",
        ]
        return order.index(name)


def get_or_create(asset_name: str):
    path = f"{FOLDER}/{asset_name}"
    if not unreal.EditorAssetLibrary.does_directory_exist(FOLDER):
        unreal.EditorAssetLibrary.make_directory(FOLDER)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path), path
    factory = unreal.DataAssetFactory()
    try:
        factory.set_editor_property("DataAssetClass", unreal.ArenaVisualDressDefinition.static_class())
    except Exception:
        factory.set_editor_property("DataAssetClass", unreal.ArenaVisualDressDefinition)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = tools.create_asset(asset_name, FOLDER, unreal.ArenaVisualDressDefinition, factory)
    return asset, path


def write_dress(asset_name, layout_name, display, notes, hide_walls, cliffs, debris, pillars, landmarks):
    dress, path = get_or_create(asset_name)
    if not dress:
        unreal.log_error(f"Failed {asset_name}")
        return False

    dress.set_editor_property("TargetLayout", layout_value(layout_name))
    dress.set_editor_property("DisplayName", unreal.Text(display))
    dress.set_editor_property("SourcePackNotes", notes)
    dress.set_editor_property("bHidePlaceholderWallsWhenDressed", hide_walls)
    dress.set_editor_property("CliffMeshPool", load_pool(cliffs))
    dress.set_editor_property("DebrisMeshPool", load_pool(debris))
    dress.set_editor_property("PillarMeshPool", load_pool(pillars))
    slots = [s for s in (make_slot(*lm) for lm in landmarks) if s is not None]
    dress.set_editor_property("DressSlots", slots)
    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log(
        f"OK {asset_name}: cliffs={len(cliffs)} debris={len(debris)} pillars={len(pillars)} slots={len(slots)}"
    )
    return True


def main():
    ok = True

    ok &= write_dress(
        "DA_Dress_TitansQuarry",
        "TitansQuarry",
        "Titan's Quarry — Quarry Collections",
        "Quarry Collections + Limestone Megascans. Visual-only dress.",
        True,
        [
            "/Game/Geometry/SM_Quarry_Cliff_L_01",
            "/Game/Geometry/SM_Quarry_Cliff_L_02",
            "/Game/Geometry/SM_Quarry_Cliff_L_03",
            "/Game/Geometry/SM_Quarry_Cliff_M_01",
            "/Game/Geometry/SM_Quarry_Cliff_M_02",
            "/Game/Geometry/SM_Quarry_Cliff_M_03",
        ],
        [
            "/Game/Geometry/SM_Quarry_RockPile_S_01",
            "/Game/Geometry/SM_Quarry_RockPile_S_02",
            "/Game/Geometry/SM_Quarry_Ridge_S_01",
            "/Game/Megascans/3D_Assets/Limestone_Rocks_01/SM_Limestone_Rocks_01",
            "/Game/Megascans/3D_Assets/Limestone_Rubble_01/SM_Limestone_Rubble_01",
            "/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01",
        ],
        [
            "/Game/Geometry/SM_Quarry_Spire_L_01",
            "/Game/Geometry/SM_Quarry_Spire_L_02",
            "/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01",
        ],
        [
            ("Spire_N", "/Game/Geometry/SM_Quarry_Spire_L_01", 0, 2100, -40, 0, 0, 0, 0.45, 0.45, 0.45),
            ("Spire_S", "/Game/Geometry/SM_Quarry_Spire_L_02", 0, -2100, -40, 0, 180, 0, 0.4, 0.4, 0.4),
            ("Ridge_E", "/Game/Geometry/SM_Quarry_Ridge_S_01", 2050, 0, -20, 0, 90, 0, 0.55, 0.55, 0.55),
            ("Outcrop_Cover", "/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01", 650, 450, 0, 0, 35, 0, 1, 1, 1),
            ("GroundPatch", "/Game/Geometry/Custom/SM_RockGround_01", 0, 0, -20, 0, 0, 0, 8, 8, 1),
        ],
    )

    ok &= write_dress(
        "DA_Dress_AshfallCrucible",
        "AshfallCrucible",
        "Ashfall Crucible — Lava Field",
        "Lava Field Collections + custom scree.",
        True,
        [
            "/Game/Geometry/SM_Lava_Cliff_L_01",
            "/Game/Geometry/SM_Lava_Cliff_L_02",
            "/Game/Geometry/SM_Lava_Cliff_L_03",
            "/Game/Geometry/SM_Lava_Cliff_L_04",
            "/Game/Geometry/SM_Lava_Cliff_M_01",
            "/Game/Geometry/SM_Lava_Cliff_M_02",
        ],
        [
            "/Game/Geometry/SM_Lava_RockMound_S_01",
            "/Game/Geometry/SM_Lava_RockMound_S_02",
            "/Game/Geometry/SM_Lava_RockMound_S_03",
            "/Game/Geometry/SM_Lava_RockMound_S_04",
            "/Game/Geometry/Custom/SM_Custom_Scree_L_04",
        ],
        [
            "/Game/Geometry/SM_Lava_TreeMound_M_01",
            "/Game/Geometry/SM_Lava_TreeMound_M_02",
            "/Game/Geometry/SM_Lava_TreeMound_M_03",
        ],
        [
            ("LavaCliff_N", "/Game/Geometry/SM_Lava_Cliff_L_01", 0, 2300, -60, 0, 0, 0, 0.35, 0.35, 0.35),
            ("LavaCliff_S", "/Game/Geometry/SM_Lava_Cliff_L_02", 0, -2300, -60, 0, 180, 0, 0.35, 0.35, 0.35),
            ("BossMound", "/Game/Geometry/SM_Lava_TreeMound_M_04", 0, 0, 40, 0, 0, 0, 0.55, 0.55, 0.55),
            ("Scree_E", "/Game/Geometry/Custom/SM_Custom_Scree_XXL_02", 2000, 400, -30, 0, 90, 0, 0.25, 0.25, 0.25),
        ],
    )

    ok &= write_dress(
        "DA_Dress_FrostboundCitadel",
        "FrostboundCitadel",
        "Frostbound Citadel — Iceland + Winter Forest",
        "Iceland_Environment mountains + WinterForestSet props.",
        True,
        [
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_01",
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_02",
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_03",
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_04",
            "/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Eroded_Mountain",
            "/Game/WinterForestSet/Meshes/SM_SnowMountain_A",
        ],
        [
            "/Game/WinterForestSet/Meshes/SM_SnowPile_A",
            "/Game/WinterForestSet/Meshes/SM_SnowPile_B",
            "/Game/WinterForestSet/Meshes/SM_WinterRock_A",
            "/Game/WinterForestSet/Meshes/SM_WinterRock_C",
            "/Game/WinterForestSet/Meshes/SM_RockCluster_A",
        ],
        [
            "/Game/WinterForestSet/Meshes/SM_WinterTree_A",
            "/Game/WinterForestSet/Meshes/SM_WinterTree_C",
            "/Game/WinterForestSet/Meshes/SM_WinterTree_E",
            "/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Crest",
        ],
        [
            ("KeepMountain", "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_Plateu_01", 0, 2400, -80, 0, 0, 0, 0.2, 0.2, 0.2),
            ("IceMonolith", "/Game/WinterForestSet/Meshes/SM_WinterRock_F", 0, 0, 20, 0, 0, 0, 2.5, 2.5, 2.5),
            ("SnowYard", "/Game/WinterForestSet/Meshes/SM_SnowPile_C", 500, -400, 0, 0, 0, 0, 1.5, 1.5, 1.5),
        ],
    )

    ok &= write_dress(
        "DA_Dress_VerdantSanctuary",
        "VerdantSanctuary",
        "Verdant Sanctuary — Beech / Maple / Moss",
        "EuropeanBeech + NorwayMaple SimpleWind + moss ground.",
        True,
        [
            "/Game/Geometry/Custom/SM_Custom_Cliff_XL_06",
            "/Game/Geometry/Custom/SM_Custom_Cliff_XL_08",
            "/Game/Geometry/Custom/SM_Custom_Plateau_XXL_01",
        ],
        [
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Sapling_01",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Sapling_02",
            "/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Field_01",
            "/Game/A_Surface_Footstep/Geometry/SM_Bush",
            "/Game/Geometry/Custom/SM_MossGround_01",
        ],
        [
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_03",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_05",
            "/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_01",
            "/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_02",
        ],
        [
            ("TempleTree_A", "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_02", -900, 700, 0, 0, 40, 0, 1.1, 1.1, 1.1),
            ("TempleTree_B", "/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_03", 950, -650, 0, 0, -25, 0, 1, 1, 1),
            ("MossFloor", "/Game/Geometry/Custom/SM_MossGround_02", 0, 0, -15, 0, 0, 0, 10, 10, 1),
            ("Statue", "/Game/StarterContent/Props/SM_Statue", 0, 200, 40, 0, 180, 0, 2, 2, 2),
        ],
    )

    ok &= write_dress(
        "DA_Dress_SunscarExpanse",
        "SunscarExpanse",
        "Sunscar Expanse — Sandstone / Moab",
        "RockSandstone Megascans + Geometry sand/succulents.",
        True,
        [
            "/Game/Megascans/3D_Assets/CanyonSandstone/SM_CanyonSandstone",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_00",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_01",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_02",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_03",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_05",
            "/Game/Geometry/Custom/SM_Custom_Cliff_XL_08",
        ],
        [
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_07",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_08",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_09",
            "/Game/Geometry/Succulents/SM_Succulent_Green_Short_01",
            "/Game/Geometry/Succulents/SM_Succulent_Dead_Short_01",
            "/Game/Geometry/Succulents/SM_Succulent_Red_Short_01",
        ],
        [
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_010",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_011",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_012",
        ],
        [
            ("Hoodoo_N", "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_013", 0, 2300, -20, 0, 0, 0, 1.2, 1.2, 1.2),
            ("CanyonBlock", "/Game/Megascans/3D_Assets/CanyonSandstone/SM_CanyonSandstone", 2400, 0, -40, 0, 90, 0, 0.5, 0.5, 0.5),
            ("SandFloor", "/Game/Geometry/Custom/SM_Sand_01", 0, 0, -18, 0, 0, 0, 12, 12, 1),
            ("SucculentPatch", "/Game/Geometry/Succulents/SM_Succulent_Green_Long_01", 700, -600, 0, 0, 0, 0, 1.4, 1.4, 1.4),
        ],
    )

    ok &= write_dress(
        "DA_Dress_StormbreakCoast",
        "StormbreakCoast",
        "Stormbreak Coast — Nordic Dolerite + Water",
        "Dolerite cliffs/assemblies + WaterPlane.",
        True,
        [
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_03/SM_Dolerite_Cliff_03",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_04/SM_Dolerite_Cliff_04",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_05/SM_Dolerite_Cliff_05",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_06/SM_Dolerite_Cliff_06",
            "/Game/Geometry/Custom/SM_Custom_Cliff_XL_06",
        ],
        [
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_01/SM_Dolerite_Assembly_01",
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_03/SM_Dolerite_Assembly_03",
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_04/SM_Dolerite_Assembly_04",
            "/Game/Megascans/3D_Assets/Thai_Beach_Branch_01/SM_Thai_Beach_Branch_01",
        ],
        [
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_05/SM_Dolerite_Assembly_05",
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_06/SM_Dolerite_Assembly_06",
            "/Game/StarterContent/Props/SM_PillarFrame300",
        ],
        [
            ("SeawardCliff", "/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01", 0, -1400, -80, 0, 180, 0, 0.45, 0.45, 0.45),
            ("TowerRock_L", "/Game/Megascans/3D_Assets/Dolerite_Assembly_01/SM_Dolerite_Assembly_01", -1200, 200, 0, 0, 0, 0, 0.8, 0.8, 0.8),
            ("TowerRock_R", "/Game/Megascans/3D_Assets/Dolerite_Assembly_03/SM_Dolerite_Assembly_03", 1200, 200, 0, 0, 40, 0, 0.8, 0.8, 0.8),
            ("WaterSheet", "/Game/Geometry/WaterPlane_01/SM_WaterPlane_01", 0, -2200, -180, 0, 0, 0, 6, 3, 1),
        ],
    )

    ok &= write_dress(
        "DA_Dress_ShatteredDominion",
        "ShatteredDominion",
        "Shattered Dominion — Multi-biome landmarks",
        "Composite landmarks; pools reuse Quarry/Lava/Iceland/Sandstone/Dolerite.",
        False,
        [
            "/Game/Geometry/SM_Quarry_Cliff_L_01",
            "/Game/Geometry/SM_Lava_Cliff_L_01",
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_02",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01",
        ],
        [
            "/Game/Geometry/SM_Quarry_RockPile_S_01",
            "/Game/Geometry/SM_Lava_RockMound_S_01",
            "/Game/WinterForestSet/Meshes/SM_WinterRock_A",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_01",
        ],
        [
            "/Game/Geometry/SM_Quarry_Spire_L_01",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01",
            "/Game/StarterContent/Props/SM_PillarFrame300",
        ],
        [
            ("ColiseumSpire", "/Game/Geometry/SM_Quarry_Spire_L_01", 0, 1600, -40, 0, 0, 0, 0.5, 0.5, 0.5),
            ("NorthCliff", "/Game/Geometry/SM_Quarry_Cliff_L_02", 0, 8300, -60, 0, 0, 0, 0.35, 0.35, 0.35),
            ("EastTree", "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_04", 7400, 400, 0, 0, 20, 0, 1.2, 1.2, 1.2),
            ("WestLava", "/Game/Geometry/SM_Lava_Cliff_L_03", -8300, 0, -60, 0, 90, 0, 0.35, 0.35, 0.35),
            ("SouthDolerite", "/Game/Megascans/3D_Assets/Dolerite_Cliff_04/SM_Dolerite_Cliff_04", 0, -7700, -80, 0, 180, 0, 0.4, 0.4, 0.4),
            ("SouthWater", "/Game/Geometry/WaterPlane_01/SM_WaterPlane_01", 0, -8100, -180, 0, 0, 0, 5, 2.5, 1),
            ("NESnow", "/Game/WinterForestSet/Meshes/SM_SnowMountain_A", 5200, 6600, -40, 0, 0, 0, 0.6, 0.6, 0.6),
            ("NWSandstone", "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_00", -6400, 5200, -20, 0, 45, 0, 1.1, 1.1, 1.1),
        ],
    )

    ok &= write_dress(
        "DA_Dress_FallenSanctuary",
        "FallenSanctuary",
        "Fallen Sanctuary — Ruins + Forest fringe",
        "Limestone / Quarry ruins + Beech fringe + Starter statue/bush.",
        False,
        [
            "/Game/Geometry/SM_Quarry_Cliff_M_01",
            "/Game/Geometry/SM_Quarry_Cliff_M_02",
            "/Game/Megascans/3D_Assets/Limestone_Slab_02/SM_Limestone_Slab_02",
        ],
        [
            "/Game/Megascans/3D_Assets/Limestone_Rubble_01/SM_Limestone_Rubble_01",
            "/Game/Geometry/SM_Quarry_RockPile_S_03",
            "/Game/A_Surface_Footstep/Geometry/SM_Bush",
            "/Game/StarterContent/Props/SM_Bush",
        ],
        [
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_06",
            "/Game/StarterContent/Props/SM_PillarFrame",
        ],
        [
            ("TempleStatue", "/Game/StarterContent/Props/SM_Statue", 0, 1100, 40, 0, 180, 0, 2.5, 2.5, 2.5),
            ("FallenSlab", "/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01", -400, 1400, 40, 15, 20, 8, 1.2, 1.2, 1.2),
            ("CampBush", "/Game/A_Surface_Footstep/Geometry/SM_Bush", -1200, -500, 0, 0, 0, 0, 1.5, 1.5, 1.5),
            ("GateSpire_L", "/Game/Geometry/SM_Quarry_Spire_L_01", -500, -2600, -20, 0, 0, 0, 0.25, 0.25, 0.25),
            ("GateSpire_R", "/Game/Geometry/SM_Quarry_Spire_L_02", 500, -2600, -20, 0, 180, 0, 0.25, 0.25, 0.25),
            ("ColiseumSilhouette", "/Game/Geometry/Custom/SM_Custom_Plateau_XXL_01", 0, -9000, -40, 0, 0, 0, 0.35, 0.35, 0.35),
        ],
    )

    unreal.log("ALL ARENA DRESS " + ("PASS" if ok else "FAIL"))
    return ok


if __name__ == "__main__":
    main()
