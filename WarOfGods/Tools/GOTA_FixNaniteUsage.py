"""Sets 'Used with Nanite' on the arena master materials (and any other base material used by
arena Nanite meshes) so cooked builds don't fall back to the default material.

Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_FixNaniteUsage.txt
"""
import os

import unreal

MASTERS = [
    "/Game/Arenas/Shared/Materials/M_GOTA_ArenaSurface",
    "/Game/Arenas/Shared/Materials/M_GOTA_ArenaSurfaceVT",
    "/Game/Arenas/Shared/Materials/M_GOTA_ArenaEmissive",
    "/Game/StarterContent/Materials/M_Water_Ocean",
]
log = []
for path in MASTERS:
    mat = unreal.load_asset(path)
    if not mat:
        log.append("MISSING " + path)
        continue
    before = mat.get_editor_property("used_with_nanite")
    if not before:
        mat.set_editor_property("used_with_nanite", True)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    log.append("%s used_with_nanite %s -> %s" % (path, before, mat.get_editor_property("used_with_nanite")))
open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_FixNaniteUsage.txt"), "w").write("\n".join(log))
