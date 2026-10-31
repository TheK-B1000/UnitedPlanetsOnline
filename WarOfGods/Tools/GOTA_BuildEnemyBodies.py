"""Enemy roster bodies: real character meshes over the existing enemy combat pawns (BP_Enemy_*).

Each enemy keeps its own AI, attacks, montages and hit windows on its (hidden) mannequin combat frame; the
visible body follows it through the same runtime IK retarget as the hero bodies. Definitions are
UGOTAEnemyDefinition assets (primary asset type "GOTAEnemy", never in hero selection), applied on the server
by UGOTAEnemyBodySubsystem when a matching pawn spawns. The Fight AI champion is dressed explicitly by
ADuelGameMode. Rigs: Tools/GOTA_BuildParagonRigs.py (MinionLane, MinionSuper, FengMao).

Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_BuildEnemyBodies.txt
"""
import os

import unreal

EAL = unreal.EditorAssetLibrary
DATA = "/Game/Enemies/Data"
RIGS = "/Game/Heroes/Paragon/Rigs"
MINIONS = "/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/"
A = unreal.GOTAEnemyArchetype

ENEMIES = {
    # Common enemies: Paragon lane minions (Dawn). Their own weapons show; the frame's sword is hidden.
    "Enemy_MeleeSoldier": dict(name="Dawn Legionary", archetype=A.MELEE_SOLDIER, pawn="/Game/Enemies/MeleeEnemy/BP_Enemy_Melee",
                               mesh=MINIONS + "Minion_Lane_Melee_Dawn", rig="MinionLane", scale=1.0, auto=True,
                               note="Paragon: Lane Melee Minion (Dawn)"),
    "Enemy_RangedSoldier": dict(name="Dawn Gunner", archetype=A.RANGED_SOLDIER, pawn="/Game/Enemies/BP_Enemy_Ranged",
                                mesh=MINIONS + "Minion_Lane_Ranged_Dawn", rig="MinionLane", scale=1.0, auto=True,
                                note="Paragon: Lane Ranged Minion (Dawn)"),
    "Enemy_HeavyBrute": dict(name="Dawn Juggernaut", archetype=A.HEAVY_BRUTE, pawn="/Game/Enemies/BossEnemy/BP_Enemy_Boss",
                             mesh=MINIONS + "Minion_Lane_Super_Dawn", rig="MinionSuper", scale=1.0, auto=True,
                             note="Paragon: Lane Super Minion (Dawn)"),
    # Fight AI champion: an elite with a full hero body. Applied explicitly (BP_Enemy_Melee is also the common soldier).
    "Enemy_Champion": dict(name="Arena Champion", archetype=A.ELITE_CHAMPION, pawn="/Game/Enemies/MeleeEnemy/BP_Enemy_Melee",
                           mesh="/Game/ParagonFengMao/Characters/Heroes/FengMao/Meshes/FengMao_GDC", rig="FengMao",
                           scale=float(os.environ.get("GOTA_CHAMPION_SCALE", "1.0")), auto=False,
                           note="Paragon: Feng Mao (elite champion body)"),
}

log = []
for enemy_id, e in ENEMIES.items():
    path = "%s/DA_%s" % (DATA, enemy_id)
    mesh = unreal.load_asset(e["mesh"])
    rtg = unreal.load_asset("%s/RTG_GOTA_Manny_%s" % (RIGS, e["rig"]))
    pawn = unreal.load_class(None, e["pawn"] + "." + e["pawn"].rsplit("/", 1)[1] + "_C")
    if not mesh or not rtg or not pawn:
        log.append("%s FAIL mesh=%s rtg=%s pawn=%s" % (enemy_id, bool(mesh), bool(rtg), bool(pawn)))
        continue
    da = EAL.load_asset(path)
    if not da:
        da = unreal.AssetToolsHelpers.get_asset_tools().create_asset("DA_" + enemy_id, DATA, unreal.GOTAEnemyDefinition, unreal.DataAssetFactory())
    da.set_editor_property("hero_id", enemy_id)
    da.set_editor_property("display_name", unreal.Text(e["name"]))
    da.set_editor_property("archetype", e["archetype"])
    da.set_editor_property("pawn_class", pawn)
    da.set_editor_property("auto_dress_pawn_class", e["auto"])
    da.set_editor_property("body_mesh", mesh)
    da.set_editor_property("body_retargeter", rtg)
    da.set_editor_property("body_scale", e["scale"])
    da.set_editor_property("body_source_note", unreal.Text(e["note"]))
    da.set_editor_property("hide_attached_weapon_meshes", True)
    EAL.save_asset(path)
    log.append("%s OK %s on %s scale=%.2f auto=%s" % (enemy_id, mesh.get_name(), pawn.get_name(), e["scale"], e["auto"]))
open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_BuildEnemyBodies.txt"), "w").write("\n".join(log))
