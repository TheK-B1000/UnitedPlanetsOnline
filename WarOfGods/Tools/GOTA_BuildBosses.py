"""Boss 01: Rampage - The Quarry Behemoth.

Creates /Game/Bosses/BP_Boss_QuarryBehemoth (child of BP_Enemy_Base: keeps BPC_DamageSystem, the
BPI_Damagable interface and death events; a plain AIController replaces the melee behaviour tree)
and /Game/Bosses/DA_Boss_QuarryBehemoth (every boss balance/timing value; UGOTABossComponent reads it).

Hit windows are in seconds on the attack's animation timeline at PlayRate 1 (see Docs/GOTA_BOSS_QUARRY_BEHEMOTH.md).
Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_BuildBosses.txt
"""
import os

import unreal

EAL = unreal.EditorAssetLibrary
OUT = "/Game/Bosses"
R = "/Game/ParagonRampage/Characters/Heroes/Rampage/"
RA = R + "Animations/"
RS = R + "Sounds/SoundCues/"
VFX = "/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/"
GFX = "/Game/ParagonGreystone/FX/Particles/Greystone/"
K = unreal.GOTABossAttackKind
log = []


def P(*a):
    line = " ".join(str(x) for x in a)
    log.append(line)
    unreal.log("GOTA_BuildBosses: " + line)


def asset(path):
    if not path:
        return None
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    name = path.split("/")[-1]
    root = "/".join(path.split("/")[:3])
    for a in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(root, recursive=True):
        if str(a.asset_name) == name:
            return unreal.load_asset(str(a.package_name))
    P("  MISSING", path)
    return None


def window(start, end, damage, bone="hand_r", radius=120.0):
    w = unreal.GOTABossHitWindow()
    w.set_editor_property("start", start)
    w.set_editor_property("end", end)
    w.set_editor_property("damage", float(damage))
    w.set_editor_property("bone", bone)
    w.set_editor_property("radius", float(radius))
    return w


# Prototype balance (player Veteran health 130). Damage is conservative; every attack is blockable or dodgeable.
ATTACKS = [
    dict(id="ClawCombo", name="Claw Combo", kind=K.MELEE, anims=["Attack_Melee_A", "Attack_Melee_B", "Attack_Melee_C"], rate=1.0,
         windows=[(0.36, 0.52, 9, "hand_r"), (1.33, 1.49, 9, "hand_l"), (2.30, 2.48, 11, "hand_r")],
         min=0, max=300, cooldown=3.0, w1=1.0, w2=1.0, track=0.30, recovery=1.0, rec2=0.7, block=True,
         impact_fx=VFX + "Hit/P_Concrete", fx_scale=1.4, cast_sfx=RS + "Rampage_Effort_Attack", impact_sfx="/Game/Audio/axe-block", kick=1.0),
    dict(id="BehemothCharge", name="Behemoth Charge", kind=K.CHARGE, anims=["Ability_Energize"], rate=1.15,
         windows=[(0.75, 0.75, 18, "root")], speed=1650, seconds=1.0, knock=950,
         min=650, max=1800, cooldown=9.0, w1=0.9, w2=1.2, track=0.7, recovery=1.6, rec2=0.75, block=False,
         telegraph="BEHEMOTH CHARGE  -  SIDESTEP!",
         impact_fx=VFX + "Destruction/P_Destruction_Concrete", fx_scale=1.2, cast_sfx=RS + "Rampage_Effort_Ability_RMB", impact_sfx="/Game/Audio/axe-block", kick=1.8),
    dict(id="Earthshaker", name="Earthshaker", kind=K.SLAM, anims=["Ability_GroundSmash_Start", "Ability_GroundSmash_End"], rate=1.0,
         windows=[(0.80, 0.86, 22, "root")], radius=450, knock=600, chain2=3,
         min=0, max=520, cooldown=8.0, w1=0.8, w2=1.3, track=0.45, recovery=1.3, rec2=0.6, block=False,
         telegraph="EARTHSHAKER  -  GET CLEAR!",
         impact_fx=GFX + "P_Greystone_LeapAOE_GroundSmash", fx_scale=1.4, cast_sfx=RS + "Rampage_Effort_Ability_Q", impact_sfx="/Game/Audio/sci-fi-explosion", kick=2.2),
    dict(id="CrushingGrasp", name="Crushing Grasp", kind=K.GRAB, anims=["Ability_RipNToss_Rip", "Ability_RipNToss_Toss"], rate=1.0,
         windows=[(0.62, 0.66, 20, "hand_r", 130)], knock=550,
         min=0, max=260, cooldown=11.0, w1=0.6, w2=0.8, track=0.35, recovery=1.2, rec2=0.8, block=False,
         telegraph="CRUSHING GRASP  -  BACK OFF!",
         impact_fx=VFX + "Hit/P_Concrete", fx_scale=1.6, cast_sfx=RS + "Rampage_Effort_Ability_E", impact_sfx="/Game/Audio/axe-block", kick=1.6),
]


def build_definition():
    name = "DA_Boss_QuarryBehemoth"
    path = OUT + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    f = unreal.DataAssetFactory()
    f.set_editor_property("data_asset_class", unreal.GOTABossDefinition)
    da = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, OUT, unreal.GOTABossDefinition, f)
    da.set_editor_property("boss_id", "QuarryBehemoth")
    da.set_editor_property("display_name", unreal.Text("The Quarry Behemoth"))
    da.set_editor_property("mesh", asset(R + "Meshes/Rampage"))
    abp = asset(R + "Rampage_AnimBlueprint")
    if abp:
        da.set_editor_property("anim_class", abp.generated_class())
    da.set_editor_property("montage_slot", "UpperBody")
    for k, v in dict(capsule_radius=105.0, capsule_half_height=112.0, mesh_yaw=-90.0, mesh_scale=1.0, max_health=900.0,
                     walk_speed=430.0, walk_speed_phase2=540.0, turn_rate_degrees=220.0, engage_distance=230.0,
                     attack_gap=1.5, attack_gap_phase2=0.9, stagger_threshold=70.0, stagger_window=3.0,
                     phase_two_health_fraction=0.5).items():
        da.set_editor_property(k, v)
    for k, a in dict(intro_anim="Emote_Master_Roar_T3", phase_two_anim="Ability_Enrage_Start", death_anim="Death_A",
                     hit_react_anim="HitReact_Front", charge_loop_anim="Sprint_Quad_Fwd").items():
        da.set_editor_property(k, asset(RA + a))
    da.set_editor_property("roar_sound", asset(RS + "Rampage_Effort_Ability_Ultimate_Grow"))
    da.set_editor_property("phase_two_fx", asset(VFX + "Explosion/P_Explosion_Smoke"))
    attacks = []
    for a in ATTACKS:
        at = unreal.GOTABossAttack()
        at.set_editor_property("attack_id", a["id"])
        at.set_editor_property("display_name", unreal.Text(a["name"]))
        at.set_editor_property("kind", a["kind"])
        at.set_editor_property("animations", [asset(RA + n) for n in a["anims"]])
        at.set_editor_property("play_rate", float(a["rate"]))
        at.set_editor_property("hit_windows", [window(*w) for w in a["windows"]])
        at.set_editor_property("min_range", float(a["min"]))
        at.set_editor_property("max_range", float(a["max"]))
        at.set_editor_property("cooldown", float(a["cooldown"]))
        at.set_editor_property("weight_phase1", float(a["w1"]))
        at.set_editor_property("weight_phase2", float(a["w2"]))
        at.set_editor_property("track_seconds", float(a["track"]))
        at.set_editor_property("recovery", float(a["recovery"]))
        at.set_editor_property("recovery_scale_phase2", float(a["rec2"]))
        at.set_editor_property("can_be_blocked", bool(a["block"]))
        at.set_editor_property("charge_speed", float(a.get("speed", 1500)))
        at.set_editor_property("charge_seconds", float(a.get("seconds", 1.0)))
        at.set_editor_property("slam_radius", float(a.get("radius", 450)))
        at.set_editor_property("knockback", float(a.get("knock", 0)))
        at.set_editor_property("chain_count_phase2", int(a.get("chain2", 1)))
        at.set_editor_property("telegraph", unreal.Text(a.get("telegraph", "")))
        at.set_editor_property("impact_fx", asset(a.get("impact_fx")))
        at.set_editor_property("fx_scale", float(a.get("fx_scale", 1.0)))
        at.set_editor_property("cast_sound", asset(a.get("cast_sfx")))
        at.set_editor_property("impact_sound", asset(a.get("impact_sfx")))
        at.set_editor_property("impact_kick", float(a.get("kick", 1.0)))
        attacks.append(at)
        P(" attack", a["id"], "anims", [bool(x) for x in at.get_editor_property("animations")], "fx", bool(at.get_editor_property("impact_fx")))
    da.set_editor_property("attacks", attacks)
    EAL.save_asset(path)
    P("definition saved", path)


def build_blueprint():
    name = "BP_Boss_QuarryBehemoth"
    path = OUT + "/" + name
    parent = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Enemies/BP_Enemy_Base")
    if not EAL.does_asset_exist(path):
        f = unreal.BlueprintFactory()
        f.set_editor_property("parent_class", parent)
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, OUT, unreal.Blueprint, f)
    else:
        bp = unreal.load_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    # Boss brain is UGOTABossComponent: a plain AIController (no BT_Enemy_Base / token melee logic).
    cdo.set_editor_property("ai_controller_class", unreal.AIController.static_class())
    cdo.set_editor_property("auto_possess_ai", unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
    for var, val in (("BehaviorTree", None), ("IsWieldingWeapon", False)):
        try:
            cdo.set_editor_property(var, val)
        except Exception as e:
            P("  (cdo var %s not set: %s)" % (var, e))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_asset(path)
    P("blueprint saved", path, "parent", parent.get_name())


EAL.make_directory(OUT)
build_blueprint()
build_definition()
open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_BuildBosses.txt"), "w").write("\n".join(log))
