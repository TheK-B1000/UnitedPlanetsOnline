# Create / refresh all DA_Dress_* ArenaVisualDressDefinition assets under /Game/Arenas/Data.
# Run:
#   UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript=.../GOTA_CreateAllDressDAs.py -unattended -nosplash -NullRHI
#
# Soft paths mirror ArenaVisualDressDefaults.cpp transient pilots.

import unreal

FOLDER = "/Game/Arenas/Data"

# layout_key -> (asset_name, display_name, notes, hide_walls, layout_attr, cliffs, debris, pillars, landmarks)
# landmark: (slot_id, mesh_path, loc, rot, scale)
# layout_attr: unreal.ArenaGrayboxLayout member name or int fallback

def _paths(*items):
    return list(items)


DRESS_SPECS = [
    {
        "asset": "DA_Dress_TitansQuarry",
        "layout": "TITANS_QUARRY",
        "layout_i": 0,
        "name": "Titan's Quarry — Quarry Collections",
        "notes": "Quarry Collections + Limestone Megascans. Persisted from C++ pilot.",
        "hide_walls": True,
        "cliffs": _paths(
            "/Game/Geometry/SM_Quarry_Cliff_L_01",
            "/Game/Geometry/SM_Quarry_Cliff_L_02",
            "/Game/Geometry/SM_Quarry_Cliff_L_03",
            "/Game/Geometry/SM_Quarry_Cliff_M_01",
            "/Game/Geometry/SM_Quarry_Cliff_M_02",
            "/Game/Geometry/SM_Quarry_Cliff_M_03",
        ),
        "debris": _paths(
            "/Game/Geometry/SM_Quarry_RockPile_S_01",
            "/Game/Geometry/SM_Quarry_RockPile_S_02",
            "/Game/Geometry/SM_Quarry_Ridge_S_01",
            "/Game/Megascans/3D_Assets/Limestone_Rocks_01/SM_Limestone_Rocks_01",
            "/Game/Megascans/3D_Assets/Limestone_Rubble_01/SM_Limestone_Rubble_01",
            "/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01",
        ),
        "pillars": _paths(
            "/Game/Geometry/SM_Quarry_Spire_L_01",
            "/Game/Geometry/SM_Quarry_Spire_L_02",
            "/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01",
        ),
        "landmarks": [
            ("Spire_N", "/Game/Geometry/SM_Quarry_Spire_L_01", (0, 2100, -40), (0, 0, 0), (0.45, 0.45, 0.45)),
            ("Spire_S", "/Game/Geometry/SM_Quarry_Spire_L_02", (0, -2100, -40), (0, 180, 0), (0.4, 0.4, 0.4)),
            ("Ridge_E", "/Game/Geometry/SM_Quarry_Ridge_S_01", (2050, 0, -20), (0, 90, 0), (0.55, 0.55, 0.55)),
            ("Outcrop_Cover", "/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01", (650, 450, 0), (0, 35, 0), (1, 1, 1)),
            ("GroundPatch", "/Game/Geometry/Custom/SM_RockGround_01", (0, 0, -20), (0, 0, 0), (8, 8, 1)),
        ],
    },
    {
        "asset": "DA_Dress_AshfallCrucible",
        "layout": "ASHFALL_CRUCIBLE",
        "layout_i": 1,
        "name": "Ashfall Crucible — Lava Field",
        "notes": "Lava Field Collections + custom scree.",
        "hide_walls": True,
        "cliffs": _paths(
            "/Game/Geometry/SM_Lava_Cliff_L_01",
            "/Game/Geometry/SM_Lava_Cliff_L_02",
            "/Game/Geometry/SM_Lava_Cliff_L_03",
            "/Game/Geometry/SM_Lava_Cliff_L_04",
            "/Game/Geometry/SM_Lava_Cliff_M_01",
            "/Game/Geometry/SM_Lava_Cliff_M_02",
        ),
        "debris": _paths(
            "/Game/Geometry/SM_Lava_RockMound_S_01",
            "/Game/Geometry/SM_Lava_RockMound_S_02",
            "/Game/Geometry/SM_Lava_RockMound_S_03",
            "/Game/Geometry/SM_Lava_RockMound_S_04",
            "/Game/Geometry/Custom/SM_Custom_Scree_L_04",
        ),
        "pillars": _paths(
            "/Game/Geometry/SM_Lava_TreeMound_M_01",
            "/Game/Geometry/SM_Lava_TreeMound_M_02",
            "/Game/Geometry/SM_Lava_TreeMound_M_03",
        ),
        "landmarks": [
            ("LavaCliff_N", "/Game/Geometry/SM_Lava_Cliff_L_01", (0, 2300, -60), (0, 0, 0), (0.35, 0.35, 0.35)),
            ("LavaCliff_S", "/Game/Geometry/SM_Lava_Cliff_L_02", (0, -2300, -60), (0, 180, 0), (0.35, 0.35, 0.35)),
            ("BossMound", "/Game/Geometry/SM_Lava_TreeMound_M_04", (0, 0, 40), (0, 0, 0), (0.55, 0.55, 0.55)),
            ("Scree_E", "/Game/Geometry/Custom/SM_Custom_Scree_XXL_02", (2000, 400, -30), (0, 90, 0), (0.25, 0.25, 0.25)),
        ],
    },
    {
        "asset": "DA_Dress_FrostboundCitadel",
        "layout": "FROSTBOUND_CITADEL",
        "layout_i": 2,
        "name": "Frostbound Citadel — Iceland + Winter Forest",
        "notes": "Iceland_Environment + WinterForestSet.",
        "hide_walls": True,
        "cliffs": _paths(
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_01",
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_02",
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_03",
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_04",
            "/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Eroded_Mountain",
            "/Game/WinterForestSet/Meshes/SM_SnowMountain_A",
        ),
        "debris": _paths(
            "/Game/WinterForestSet/Meshes/SM_SnowPile_A",
            "/Game/WinterForestSet/Meshes/SM_SnowPile_B",
            "/Game/WinterForestSet/Meshes/SM_WinterRock_A",
            "/Game/WinterForestSet/Meshes/SM_WinterRock_C",
            "/Game/WinterForestSet/Meshes/SM_RockCluster_A",
        ),
        "pillars": _paths(
            "/Game/WinterForestSet/Meshes/SM_WinterTree_A",
            "/Game/WinterForestSet/Meshes/SM_WinterTree_C",
            "/Game/WinterForestSet/Meshes/SM_WinterTree_E",
            "/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Crest",
        ),
        "landmarks": [
            ("KeepMountain", "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_Plateu_01", (0, 2400, -80), (0, 0, 0), (0.2, 0.2, 0.2)),
            ("IceMonolith", "/Game/WinterForestSet/Meshes/SM_WinterRock_F", (0, 0, 20), (0, 0, 0), (2.5, 2.5, 2.5)),
            ("SnowYard", "/Game/WinterForestSet/Meshes/SM_SnowPile_C", (500, -400, 0), (0, 0, 0), (1.5, 1.5, 1.5)),
        ],
    },
    {
        "asset": "DA_Dress_VerdantSanctuary",
        "layout": "VERDANT_SANCTUARY",
        "layout_i": 3,
        "name": "Verdant Sanctuary — Beech / Maple / Moss",
        "notes": "EuropeanBeech + NorwayMaple SimpleWind.",
        "hide_walls": True,
        "cliffs": _paths(
            "/Game/Geometry/Custom/SM_Custom_Cliff_XL_06",
            "/Game/Geometry/Custom/SM_Custom_Cliff_XL_08",
            "/Game/Geometry/Custom/SM_Custom_Plateau_XXL_01",
        ),
        "debris": _paths(
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Sapling_01",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Sapling_02",
            "/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Field_01",
            "/Game/A_Surface_Footstep/Geometry/SM_Bush",
            "/Game/Geometry/Custom/SM_MossGround_01",
        ),
        "pillars": _paths(
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_03",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_05",
            "/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_01",
            "/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_02",
        ),
        "landmarks": [
            ("TempleTree_A", "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_02", (-900, 700, 0), (0, 40, 0), (1.1, 1.1, 1.1)),
            ("TempleTree_B", "/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_03", (950, -650, 0), (0, -25, 0), (1, 1, 1)),
            ("MossFloor", "/Game/Geometry/Custom/SM_MossGround_02", (0, 0, -15), (0, 0, 0), (10, 10, 1)),
            ("Statue", "/Game/StarterContent/Props/SM_Statue", (0, 200, 40), (0, 180, 0), (2, 2, 2)),
        ],
    },
    {
        "asset": "DA_Dress_SunscarExpanse",
        "layout": "SUNSCAR_EXPANSE",
        "layout_i": 4,
        "name": "Sunscar Expanse — Sandstone / Moab",
        "notes": "RockSandstone Megascans + sand/succulents.",
        "hide_walls": True,
        "cliffs": _paths(
            "/Game/Megascans/3D_Assets/CanyonSandstone/SM_CanyonSandstone",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_00",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_01",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_02",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_03",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_05",
            "/Game/Geometry/Custom/SM_Custom_Cliff_XL_08",
        ),
        "debris": _paths(
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_07",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_08",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_09",
            "/Game/Geometry/Succulents/SM_Succulent_Green_Short_01",
            "/Game/Geometry/Succulents/SM_Succulent_Dead_Short_01",
            "/Game/Geometry/Succulents/SM_Succulent_Red_Short_01",
        ),
        "pillars": _paths(
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_010",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_011",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_012",
        ),
        "landmarks": [
            ("Hoodoo_N", "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_013", (0, 2300, -20), (0, 0, 0), (1.2, 1.2, 1.2)),
            ("CanyonBlock", "/Game/Megascans/3D_Assets/CanyonSandstone/SM_CanyonSandstone", (2400, 0, -40), (0, 90, 0), (0.5, 0.5, 0.5)),
            ("SandFloor", "/Game/Geometry/Custom/SM_Sand_01", (0, 0, -18), (0, 0, 0), (12, 12, 1)),
            ("SucculentPatch", "/Game/Geometry/Succulents/SM_Succulent_Green_Long_01", (700, -600, 0), (0, 0, 0), (1.4, 1.4, 1.4)),
        ],
    },
    {
        "asset": "DA_Dress_StormbreakCoast",
        "layout": "STORMBREAK_COAST",
        "layout_i": 5,
        "name": "Stormbreak Coast — Nordic Dolerite + Water",
        "notes": "Dolerite cliffs/assemblies + WaterPlane.",
        "hide_walls": True,
        "cliffs": _paths(
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_03/SM_Dolerite_Cliff_03",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_04/SM_Dolerite_Cliff_04",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_05/SM_Dolerite_Cliff_05",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_06/SM_Dolerite_Cliff_06",
            "/Game/Geometry/Custom/SM_Custom_Cliff_XL_06",
        ),
        "debris": _paths(
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_01/SM_Dolerite_Assembly_01",
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_03/SM_Dolerite_Assembly_03",
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_04/SM_Dolerite_Assembly_04",
            "/Game/Megascans/3D_Assets/Thai_Beach_Branch_01/SM_Thai_Beach_Branch_01",
        ),
        "pillars": _paths(
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_05/SM_Dolerite_Assembly_05",
            "/Game/Megascans/3D_Assets/Dolerite_Assembly_06/SM_Dolerite_Assembly_06",
            "/Game/StarterContent/Props/SM_PillarFrame300",
        ),
        "landmarks": [
            ("SeawardCliff", "/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01", (0, -1400, -80), (0, 180, 0), (0.45, 0.45, 0.45)),
            ("TowerRock_L", "/Game/Megascans/3D_Assets/Dolerite_Assembly_01/SM_Dolerite_Assembly_01", (-1200, 200, 0), (0, 0, 0), (0.8, 0.8, 0.8)),
            ("TowerRock_R", "/Game/Megascans/3D_Assets/Dolerite_Assembly_03/SM_Dolerite_Assembly_03", (1200, 200, 0), (0, 40, 0), (0.8, 0.8, 0.8)),
            ("WaterSheet", "/Game/Geometry/WaterPlane_01/SM_WaterPlane_01", (0, -2200, -180), (0, 0, 0), (6, 3, 1)),
        ],
    },
    {
        "asset": "DA_Dress_ShatteredDominion",
        "layout": "SHATTERED_DOMINION",
        "layout_i": 6,
        "name": "Shattered Dominion — Multi-biome landmarks",
        "notes": "Composite landmarks at biome offsets.",
        "hide_walls": False,
        "cliffs": _paths(
            "/Game/Geometry/SM_Quarry_Cliff_L_01",
            "/Game/Geometry/SM_Lava_Cliff_L_01",
            "/Game/Iceland_Environment/Static_Meshes/SM_Mountain_02",
            "/Game/Megascans/3D_Assets/Dolerite_Cliff_01/SM_Dolerite_Cliff_01",
        ),
        "debris": _paths(
            "/Game/Geometry/SM_Quarry_RockPile_S_01",
            "/Game/Geometry/SM_Lava_RockMound_S_01",
            "/Game/WinterForestSet/Meshes/SM_WinterRock_A",
            "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_01",
        ),
        "pillars": _paths(
            "/Game/Geometry/SM_Quarry_Spire_L_01",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01",
            "/Game/StarterContent/Props/SM_PillarFrame300",
        ),
        "landmarks": [
            ("ColiseumSpire", "/Game/Geometry/SM_Quarry_Spire_L_01", (0, 1600, -40), (0, 0, 0), (0.5, 0.5, 0.5)),
            ("NorthCliff", "/Game/Geometry/SM_Quarry_Cliff_L_02", (0, 8300, -60), (0, 0, 0), (0.35, 0.35, 0.35)),
            ("EastTree", "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_04", (7400, 400, 0), (0, 20, 0), (1.2, 1.2, 1.2)),
            ("WestLava", "/Game/Geometry/SM_Lava_Cliff_L_03", (-8300, 0, -60), (0, 90, 0), (0.35, 0.35, 0.35)),
            ("SouthDolerite", "/Game/Megascans/3D_Assets/Dolerite_Cliff_04/SM_Dolerite_Cliff_04", (0, -7700, -80), (0, 180, 0), (0.4, 0.4, 0.4)),
            ("SouthWater", "/Game/Geometry/WaterPlane_01/SM_WaterPlane_01", (0, -8100, -180), (0, 0, 0), (5, 2.5, 1)),
            ("NESnow", "/Game/WinterForestSet/Meshes/SM_SnowMountain_A", (5200, 6600, -40), (0, 0, 0), (0.6, 0.6, 0.6)),
            ("NWSandstone", "/Game/Megascans/3D_Assets/RockSandstone/SM_RockSandstone_00", (-6400, 5200, -20), (0, 45, 0), (1.1, 1.1, 1.1)),
        ],
    },
    {
        "asset": "DA_Dress_FallenSanctuary",
        "layout": "FALLEN_SANCTUARY",
        "layout_i": 7,
        "name": "Fallen Sanctuary — Ruins + Forest fringe",
        "notes": "Limestone ruins + Beech fringe + gate spires.",
        "hide_walls": False,
        "cliffs": _paths(
            "/Game/Geometry/SM_Quarry_Cliff_M_01",
            "/Game/Geometry/SM_Quarry_Cliff_M_02",
            "/Game/Megascans/3D_Assets/Limestone_Slab_02/SM_Limestone_Slab_02",
        ),
        "debris": _paths(
            "/Game/Megascans/3D_Assets/Limestone_Rubble_01/SM_Limestone_Rubble_01",
            "/Game/Geometry/SM_Quarry_RockPile_S_03",
            "/Game/A_Surface_Footstep/Geometry/SM_Bush",
            "/Game/StarterContent/Props/SM_Bush",
        ),
        "pillars": _paths(
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_01",
            "/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_06",
            "/Game/StarterContent/Props/SM_PillarFrame",
        ),
        "landmarks": [
            ("TempleStatue", "/Game/StarterContent/Props/SM_Statue", (0, 1100, 40), (0, 180, 0), (2.5, 2.5, 2.5)),
            ("FallenSlab", "/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01", (-400, 1400, 40), (15, 20, 8), (1.2, 1.2, 1.2)),
            ("CampBush", "/Game/A_Surface_Footstep/Geometry/SM_Bush", (-1200, -500, 0), (0, 0, 0), (1.5, 1.5, 1.5)),
            ("GateSpire_L", "/Game/Geometry/SM_Quarry_Spire_L_01", (-500, -2600, -20), (0, 0, 0), (0.25, 0.25, 0.25)),
            ("GateSpire_R", "/Game/Geometry/SM_Quarry_Spire_L_02", (500, -2600, -20), (0, 180, 0), (0.25, 0.25, 0.25)),
            ("ColiseumSilhouette", "/Game/Geometry/Custom/SM_Custom_Plateau_XXL_01", (0, -9000, -40), (0, 0, 0), (0.35, 0.35, 0.35)),
        ],
    },
]


def load_mesh(package_path: str):
    """Load StaticMesh so SoftObjectPtr properties accept Object refs (UE Python)."""
    if not unreal.EditorAssetLibrary.does_asset_exist(package_path):
        return None
    asset = unreal.EditorAssetLibrary.load_asset(package_path)
    return asset if isinstance(asset, unreal.StaticMesh) else None


def mesh_list(package_paths):
    meshes = []
    for p in package_paths:
        m = load_mesh(p)
        if m:
            meshes.append(m)
    return meshes


def resolve_layout(spec):
    try:
        enum = unreal.ArenaGrayboxLayout
        return getattr(enum, spec["layout"])
    except Exception:
        return spec["layout_i"]


def ensure_folder():
    if not unreal.EditorAssetLibrary.does_directory_exist("/Game/Arenas"):
        unreal.EditorAssetLibrary.make_directory("/Game/Arenas")
    if not unreal.EditorAssetLibrary.does_directory_exist(FOLDER):
        unreal.EditorAssetLibrary.make_directory(FOLDER)


def get_or_create(asset_name: str):
    path = f"{FOLDER}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log(f"Refreshing {path}")
        return unreal.EditorAssetLibrary.load_asset(path), path

    factory = unreal.DataAssetFactory()
    factory.set_editor_property("DataAssetClass", unreal.ArenaVisualDressDefinition)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(asset_name, FOLDER, unreal.ArenaVisualDressDefinition, factory)
    unreal.log(f"Created {path}")
    return asset, path


def make_landmark_slot(slot_id, mesh_path, loc, rot, scale):
    slot = unreal.ArenaDressMeshSlot()
    slot.set_editor_property("SlotId", unreal.Name(slot_id))
    mesh = load_mesh(mesh_path)
    if mesh:
        slot.set_editor_property("Mesh", mesh)
    slot.set_editor_property("RelativeLocation", unreal.Vector(*loc))
    slot.set_editor_property("RelativeRotation", unreal.Rotator(rot[0], rot[1], rot[2]))
    slot.set_editor_property("Scale", unreal.Vector(*scale))
    slot.set_editor_property("bVisualOnly", True)
    return slot


def apply_spec(dress, spec):
    dress.set_editor_property("TargetLayout", resolve_layout(spec))
    dress.set_editor_property("DisplayName", unreal.Text(spec["name"]))
    dress.set_editor_property("SourcePackNotes", spec["notes"])
    dress.set_editor_property("bHidePlaceholderWallsWhenDressed", spec["hide_walls"])
    dress.set_editor_property("CliffMeshPool", mesh_list(spec["cliffs"]))
    dress.set_editor_property("DebrisMeshPool", mesh_list(spec["debris"]))
    dress.set_editor_property("PillarMeshPool", mesh_list(spec["pillars"]))

    landmarks = []
    for lm in spec.get("landmarks", []):
        landmarks.append(make_landmark_slot(*lm))
    dress.set_editor_property("DressSlots", landmarks)


def count_existing(paths):
    ok = [p for p in paths if unreal.EditorAssetLibrary.does_asset_exist(p)]
    return len(ok), len(paths)


def main():
    ensure_folder()
    created = []
    warnings = []

    for spec in DRESS_SPECS:
        all_paths = spec["cliffs"] + spec["debris"] + spec["pillars"] + [lm[1] for lm in spec.get("landmarks", [])]
        ok, total = count_existing(all_paths)
        if ok < total:
            missing = [p for p in all_paths if not unreal.EditorAssetLibrary.does_asset_exist(p)]
            unreal.log_warning(f"{spec['asset']}: {ok}/{total} meshes present; missing {len(missing)}")
            for p in missing[:8]:
                unreal.log_warning(f"  missing {p}")
            warnings.append(spec["asset"])

        dress, path = get_or_create(spec["asset"])
        if not dress:
            unreal.log_error(f"Failed to create {spec['asset']}")
            continue
        apply_spec(dress, spec)
        unreal.EditorAssetLibrary.save_asset(path)
        created.append(path)
        unreal.log(f"Saved {path} ({ok}/{total} soft refs on disk)")

    unreal.log(f"GOTA dress DAs complete: {len(created)} assets")
    if warnings:
        unreal.log_warning(f"Partial mesh coverage on: {', '.join(warnings)} (soft refs still saved; runtime skips missing)")


if __name__ == "__main__":
    main()
