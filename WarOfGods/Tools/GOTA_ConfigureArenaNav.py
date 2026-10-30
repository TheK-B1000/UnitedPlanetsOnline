# Gods of the Arena - make the arena host map's navmesh regenerate at runtime.
# Arenas (Quarry, Horde, Duel...) are generated at runtime by ArenaGrayboxBuilder, so the
# RecastNavMesh saved in ThirdPersonMap must use Dynamic runtime generation; otherwise the
# navigation system never gathers geometry and AI cannot path inside the arena.
# Run: set GOTA_QUIT_AFTER_BUILD=1 & UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript="<abs>/Tools/GOTA_ConfigureArenaNav.py"
import os
import unreal

MAP = "/Game/ThirdPerson/Maps/ThirdPersonMap"
out = []
unreal.EditorLoadingAndSavingUtils.load_map(MAP)
changed = 0
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if isinstance(actor, unreal.RecastNavMesh):
        before = actor.get_editor_property("runtime_generation")
        actor.modify()
        actor.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
        out.append("%s runtime_generation %s -> DYNAMIC" % (actor.get_name(), before))
        changed += 1
if changed:
    unreal.EditorLoadingAndSavingUtils.save_current_level()
out.append("RESULT: %d navmesh actor(s) updated" % changed)
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(proj, "Saved", "GOTA_ConfigureArenaNav.txt"), "w") as f:
    f.write("\n".join(out) + "\n")
if os.environ.get("GOTA_QUIT_AFTER_BUILD") == "1":
    unreal.SystemLibrary.quit_editor()
