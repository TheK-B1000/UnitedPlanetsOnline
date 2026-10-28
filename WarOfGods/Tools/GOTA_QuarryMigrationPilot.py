# GOTA — Quarry Collections pilot migration helper (UE 5.8 Editor Python)
# Does NOT replace Content Browser → Asset Actions → Migrate.
# Use to print the recommended pilot selection after Quarry is added to a staging project.
#
# Run in Unreal Editor (Python): File → Execute Python Script
# Or: py "B:/Unreal/UnitedPlanetsOnline/WarOfGods/Tools/GOTA_QuarryMigrationPilot.py" from Editor.

PILOT_CONTENT_PATHS = [
    "/Game/Geometry/SM_Quarry_Cliff_L_01",
    "/Game/Geometry/SM_Quarry_Cliff_L_02",
    "/Game/Geometry/SM_Quarry_Cliff_L_03",
    "/Game/Geometry/SM_Quarry_Cliff_M_01",
    "/Game/Geometry/SM_Quarry_Cliff_M_02",
    "/Game/Geometry/SM_Quarry_Spire_L_01",
    "/Game/Geometry/SM_Quarry_RockPile_S_01",
    "/Game/Geometry/SM_Quarry_RockPile_S_02",
    "/Game/Geometry/SM_Quarry_Ridge_S_01",
    "/Game/Megascans/3D_Assets/Limestone_Rocks_01/SM_Limestone_Rocks_01",
    "/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01",
    "/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01",
]

VAULT_SOURCE = r"B:\Epic\VaultCache\QuarryCollections\data\Content"
DEST_PROJECT_CONTENT = r"B:\Unreal\UnitedPlanetsOnline\WarOfGods\Content"

MANUAL_STEPS = """
QUARRY PILOT — STATUS

Quarry Collections is already Add-to-Project in WarOfGods (Geometry/SM_Quarry_* + Megascans).

Next:
1. Compile C++ (ArenaVisualDressDefaults) — transient pilot auto-applies on gota.BuildArena Quarry.
2. Optional persist DA: Tools/GOTA_CreateDress_TitansQuarry.py
3. PIE: gota.BuildArena Quarry — confirm dress, spawns, nav.
4. Tune scales in ArenaVisualDressDefaults / DA if cliffs are oversized.

Do not Explorer-copy .uasset files.
"""


def main():
    print("=== GOTA Quarry Migration Pilot ===")
    print(f"Vault source: {VAULT_SOURCE}")
    print(f"Dest Content: {DEST_PROJECT_CONTENT}")
    print("")
    print("Pilot soft paths (after migrate into WarOfGods):")
    for p in PILOT_CONTENT_PATHS:
        print(f"  {p}")
    print("")
    print(MANUAL_STEPS)

    try:
        import unreal  # noqa: F401 — only present inside Editor

        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        found = 0
        missing = 0
        for path in PILOT_CONTENT_PATHS:
            assets = registry.get_assets_by_package_name(path)
            # Soft path may be object path; also try package
            ok = unreal.EditorAssetLibrary.does_asset_exist(path)
            if ok:
                found += 1
                print(f"[IN PROJECT] {path}")
            else:
                missing += 1
                print(f"[MISSING]    {path}")
        print(f"Summary: {found} present, {missing} missing in current project.")
        if missing:
            print("Complete the Migrate steps above, then re-run this script.")
    except ImportError:
        print("(Outside Editor — printed checklist only.)")


if __name__ == "__main__":
    main()
