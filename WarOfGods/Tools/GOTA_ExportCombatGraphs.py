# Audit helper: export combat Blueprint graphs as node text into Saved/GraphAudit/.
import os
import unreal

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
out = os.path.join(proj, "Saved", "GraphAudit")
os.makedirs(out, exist_ok=True)
lines = []
for path in ["/Game/Player/BP_Player", "/Game/Attacks/BPC_Attacks", "/Game/DamageSystem/BPC_DamageSystem",
             "/Game/Enemies/BP_Enemy_Base", "/Game/Enemies/MeleeEnemy/BP_Enemy_Melee", "/Game/Enemies/AI/AIC_Enemy_Base",
             "/Game/Weapons/BP_Weapon_Base", "/Game/Weapons/BP_Weapon_Sword"]:
    bp = unreal.EditorAssetLibrary.load_asset(path)
    n = unreal.GOTAEditorUIBuilderLibrary.export_blueprint_graphs(bp, out) if bp else 0
    lines.append("%s: %d graphs" % (path, n))
with open(os.path.join(out, "_index.txt"), "w") as f:
    f.write("\n".join(lines))
if os.environ.get("GOTA_QUIT_AFTER_BUILD") == "1":
    unreal.SystemLibrary.quit_editor()
