# White Tiger Studios — create real L_GOTA_Story_Prologue.umap via Editor Python.
# Run:
#   UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript="B:/Unreal/UnitedPlanetsOnline/WarOfGods/Tools/CreateStoryPrologueMap.py" -unattended -nosplash -NullRHI
#
# Do NOT load_map after duplicate under -ExecutePythonScript — Python keeps a world
# reference and EditorServer fatals on GC leak during shutdown.

import unreal

SOURCE = "/Game/ThirdPerson/Maps/ThirdPersonMap"
DEST_DIR = "/Game/Story/Maps"
DEST = DEST_DIR + "/L_GOTA_Story_Prologue"


def ensure_dir(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def main() -> None:
    ensure_dir("/Game/Story")
    ensure_dir(DEST_DIR)

    if unreal.EditorAssetLibrary.does_asset_exist(DEST):
        unreal.log("L_GOTA_Story_Prologue already exists — leaving intact.")
        return

    if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE):
        unreal.log_error("Source map missing: " + SOURCE)
        return

    duplicated = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, DEST)
    if not duplicated:
        unreal.log_error("Failed to duplicate ThirdPersonMap -> L_GOTA_Story_Prologue")
        return

    unreal.EditorAssetLibrary.save_asset(DEST)
    unreal.log("Created and saved " + DEST)
    unreal.log("Set DefaultGameMode to GOTAStoryGameMode in Editor World Settings (optional).")


if __name__ == "__main__":
    main()
