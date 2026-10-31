# GOTA Playable Integration 01 — Create/refresh DA_Dress_TitansQuarry
# Matches ArenaVisualDressDefaults::FillTitansQuarryPilot soft refs.
# SoftObjectPtr arrays: pass loaded UStaticMesh objects (Editor converts to soft).

import unreal

ASSET_PATH = "/Game/Arenas/Data/DA_Dress_TitansQuarry"
ASSET_NAME = "DA_Dress_TitansQuarry"
FOLDER = "/Game/Arenas/Data"

CLIFF = [
    "/Game/Geometry/SM_Quarry_Cliff_L_01",
    "/Game/Geometry/SM_Quarry_Cliff_L_02",
    "/Game/Geometry/SM_Quarry_Cliff_L_03",
    "/Game/Geometry/SM_Quarry_Cliff_M_01",
    "/Game/Geometry/SM_Quarry_Cliff_M_02",
    "/Game/Geometry/SM_Quarry_Cliff_M_03",
]
DEBRIS = [
    "/Game/Geometry/SM_Quarry_RockPile_S_01",
    "/Game/Geometry/SM_Quarry_RockPile_S_02",
    "/Game/Geometry/SM_Quarry_Ridge_S_01",
    "/Game/Megascans/3D_Assets/Limestone_Rocks_01/SM_Limestone_Rocks_01",
    "/Game/Megascans/3D_Assets/Limestone_Rubble_01/SM_Limestone_Rubble_01",
    "/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01",
]
PILLARS = [
    "/Game/Geometry/SM_Quarry_Spire_L_01",
    "/Game/Geometry/SM_Quarry_Spire_L_02",
    "/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01",
]

LANDMARKS = [
    ("Spire_N", "/Game/Geometry/SM_Quarry_Spire_L_01", 0.0, 2100.0, -40.0, 0.0, 0.0, 0.0, 0.45, 0.45, 0.45),
    ("Spire_S", "/Game/Geometry/SM_Quarry_Spire_L_02", 0.0, -2100.0, -40.0, 0.0, 180.0, 0.0, 0.4, 0.4, 0.4),
    ("Ridge_E", "/Game/Geometry/SM_Quarry_Ridge_S_01", 2050.0, 0.0, -20.0, 0.0, 90.0, 0.0, 0.55, 0.55, 0.55),
    ("Outcrop_Cover", "/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01", 650.0, 450.0, 0.0, 0.0, 35.0, 0.0, 1.0, 1.0, 1.0),
    ("GroundPatch", "/Game/Geometry/Custom/SM_RockGround_01", 0.0, 0.0, -20.0, 0.0, 0.0, 0.0, 8.0, 8.0, 1.0),
]


def load_mesh(path: str):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        return None
    return unreal.EditorAssetLibrary.load_asset(path)


def soft_paths_as_meshes(paths):
    out = []
    for p in paths:
        m = load_mesh(p)
        if m is None:
            unreal.log_error(f"Missing mesh: {p}")
        else:
            out.append(m)
    return out


def ensure_folder():
    if not unreal.EditorAssetLibrary.does_directory_exist(FOLDER):
        unreal.EditorAssetLibrary.make_directory(FOLDER)


def get_or_create_dress():
    ensure_folder()
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        unreal.log(f"Refreshing existing {ASSET_PATH}")
        return unreal.EditorAssetLibrary.load_asset(ASSET_PATH)

    factory = unreal.DataAssetFactory()
    try:
        factory.set_editor_property("DataAssetClass", unreal.ArenaVisualDressDefinition.static_class())
    except Exception:
        factory.set_editor_property("DataAssetClass", unreal.ArenaVisualDressDefinition)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(ASSET_NAME, FOLDER, unreal.ArenaVisualDressDefinition, factory)
    unreal.log(f"Created {ASSET_PATH}")
    return asset


def make_slot(slot_id, mesh_path, x, y, z, pitch, yaw, roll, sx, sy, sz):
    mesh = load_mesh(mesh_path)
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


def main():
    required = CLIFF + DEBRIS + PILLARS + [lm[1] for lm in LANDMARKS]
    missing = [p for p in required if not unreal.EditorAssetLibrary.does_asset_exist(p)]
    if missing:
        unreal.log_error("Missing Quarry dress meshes — aborting:")
        for p in missing:
            unreal.log_error(f"  {p}")
        return False

    dress = get_or_create_dress()
    if not dress:
        unreal.log_error("Failed to create/load ArenaVisualDressDefinition")
        return False

    try:
        layout = unreal.ArenaGrayboxLayout.TITANS_QUARRY
    except AttributeError:
        layout = 0

    dress.set_editor_property("TargetLayout", layout)
    dress.set_editor_property("DisplayName", unreal.Text("Titan's Quarry — Quarry Collections"))
    dress.set_editor_property(
        "SourcePackNotes",
        "Quarry Collections + Limestone Megascans. Dress meshes visual-only; graybox collision authoritative.",
    )
    dress.set_editor_property("bHidePlaceholderWallsWhenDressed", True)

    dress.set_editor_property("CliffMeshPool", soft_paths_as_meshes(CLIFF))
    dress.set_editor_property("DebrisMeshPool", soft_paths_as_meshes(DEBRIS))
    dress.set_editor_property("PillarMeshPool", soft_paths_as_meshes(PILLARS))

    slots = []
    for lm in LANDMARKS:
        s = make_slot(*lm)
        if s is not None:
            slots.append(s)
    dress.set_editor_property("DressSlots", slots)

    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
    unreal.log(
        f"OK Saved DA_Dress_TitansQuarry cliffs={len(CLIFF)} debris={len(DEBRIS)} "
        f"pillars={len(PILLARS)} slots={len(slots)} (bVisualOnly)."
    )
    unreal.log("PIE: open ThirdPersonMap?Arena=Quarry  OR  gota.BuildArena Quarry")
    return True


if __name__ == "__main__":
    main()
