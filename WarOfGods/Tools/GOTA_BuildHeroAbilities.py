"""Divine Combat Pass 1: authors each hero's ability set (all balancing values live here) and the
status overlay materials, then links the sets to the hero definitions.

Values are conservative PROTOTYPE numbers (Docs/GOTA_HERO_ABILITIES.md). Context: Veteran challenger
health 130 (scaled per hero), champion 260; Divine Momentum max 100.

Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_BuildHeroAbilities.txt
"""
import os

import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
OUT = "/Game/Heroes/Abilities"
MAT = OUT + "/Materials"
log = []

G = "/Game/ParagonGreystone"
GA = G + "/Characters/Heroes/Greystone/Animations/"
GFX = G + "/FX/Particles/Greystone/"
C = "/Game/ParagonCountess"
CA = C + "/Characters/Heroes/Countess/Animations/"
S = "/Game/ParagonSevarog"
SA = S + "/Characters/Heroes/Sevarog/Animations/"

KW = "/Game/ParagonKwang"
KWA = KW + "/Characters/Heroes/Kwang/Animations/"
KWFX = KW + "/FX/Particles/Abilities/"
AU = "/Game/ParagonAurora"
AUA = AU + "/Characters/Heroes/Aurora/Animations/"
AUFX = AU + "/FX/Particles/Abilities/"
GD = "/Game/ParagonGideon"
GDA = GD + "/Characters/Heroes/Gideon/Animations/"
GDFX = GD + "/FX/Particles/Gideon/Abilities/"
STL = "/Game/ParagonSteel"
STLA = STL + "/Characters/Heroes/Steel/Animations/"

K = unreal.GOTAAbilityKind
ST = unreal.GOTAStatus
LOOK = unreal.GOTAAbilityLook

# id, name, short, kind, dict(values)
HEROES = {
    # Card-aligned sets (Docs/GOTA_HERO_ABILITIES_PLAN.md, "Card alignment"). Names and descriptions follow the hero
    # cards (Tools/GOTA_HeroContent.py); mechanics map onto existing kinds. Unimplemented card effects are noted per entry.
    "Drakon": dict(health=1.15, gap=4.0, look=LOOK.TITAN, abilities=[
        # Card: "Burst damage. Ignites enemies." (ignite / burning DoT not implemented yet)
        dict(id="MoltenStrike", name="Molten Strike", short="Molten Strike", kind=K.MELEE_ARC,
             desc="Burst damage. A heavy molten swing across a wide arc in front of Drakon.",
             damage=22, cooldown=6, cast=0.55, recovery=0.6, range=300, arc=150,
             anim=GA + "Ability_Q", cast_fx=None, impact_fx=GFX + "P_Greystone_ClearAPath_Impact", fx_scale=1.0,
             cast_sfx="/Game/ParagonGreystone/Audio/Cues/Greystone_Effort_Ability_Q", kick=1.3,
             ai=dict(priority=1.0, min=0, max=320)),
        # Card: "High movement. Breaks guard." Signature ability. Unblockable (breaks guard).
        dict(id="TitanCharge", name="Titan Charge", short="Titan Charge", kind=K.CHARGE,
             desc="High movement. Drakon charges forward and breaks guard: the first enemy struck cannot block it "
                  "and is knocked back. Sidestep the line to avoid it.",
             damage=14, cooldown=10, cast=0.3, recovery=0.5, range=750, interval=0.45, magnitude=900, blockable=False,
             anim=GA + "Attack_RMB", cast_fx=GFX + "P_Greystone_Trail_OnlyWind", impact_fx=GFX + "P_Greystone_Primary_Impact", fx_scale=1.2,
             cast_sfx="/Game/ParagonGreystone/Audio/Cues/Greystone_Effort_Ability_RMB", kick=1.5,
             ai=dict(priority=1.2, min=380, max=760)),
        # Card: "Area control. Knock back enemies." (replaces the non-card Unbreakable guard)
        dict(id="DragonRoar", name="Dragon Roar", short="Dragon Roar", kind=K.AREA_BURST,
             desc="Area control. Drakon roars: enemies within 4 m are knocked back.",
             damage=6, cooldown=12, cast=0.4, recovery=0.5, radius=400, magnitude=850,
             anim=GA + "Ability_E", cast_fx=GFX + "P_Greystone_Deflect_Looping", impact_fx=GFX + "P_Greystone_LeapAOE_GroundSmash", fx_scale=0.9,
             cast_sfx="/Game/ParagonGreystone/Audio/Cues/Greystone_Effort_Block", kick=1.2,
             ai=dict(priority=1.1, min=0, max=380)),
        # Card: "Ultimate. Massive area damage."
        dict(id="UnchainedFury", name="Unchained Fury", short="Unchained Fury", kind=K.AREA_BURST, ult=True,
             desc="Ultimate. Massive area damage: after a long, visible wind-up Drakon slams the ground and throws back "
                  "everything nearby. Leave the radius or dodge through it.",
             damage=45, cooldown=40, momentum=100, cast=1.0, recovery=1.0, radius=450, magnitude=700, blockable=False, interruptible=False,
             anim=GA + "Ability_Ultimate", cast_fx=GFX + "P_Greystone_Ultimate_Glow_Activate", impact_fx=GFX + "P_Greystone_LeapAOE_GroundSmash", fx_scale=1.3, fx_life=2.0,
             cast_sfx="/Game/ParagonGreystone/Audio/Cues/Greystone_Effort_Ability_Ultimate_Rebirth", kick=2.4,
             ai=dict(priority=2.0, min=0, max=380)),
    ]),
    "Nyx": dict(health=0.85, gap=3.0, look=LOOK.SHADOW, abilities=[
        # Card: "Short-range teleportation." Signature ability.
        dict(id="ShadowStep", name="Shadow Step", short="Shadow Step", kind=K.TELEPORT,
             desc="Short-range teleportation: Nyx blinks behind the enemy (or forward). The destination is checked "
                  "for navmesh, sight and collision.",
             cooldown=7, cast=0.15, recovery=0.25, range=650, interruptible=False,
             anim=CA + "Ability_RMB", rate=1.2, cast_fx=C + "/FX/Particles/Abilities/Blink/FX/P_Countess_TeleportBegin", impact_fx=C + "/FX/Particles/Abilities/Blink/FX/P_Countess_TeleportArrive", fx_scale=1.0, kick=0.0,
             ai=dict(priority=1.0, min=300, max=650)),
        # Card: "Stuns and disrupts." (a true stun status is not implemented yet: the strikes stagger)
        dict(id="MindSpike", name="Mind Spike", short="Mind Spike", kind=K.MULTI_HIT,
             desc="Disrupts: three rapid psychic strikes that stagger the target in front of Nyx.",
             damage=9, hits=3, interval=0.22, cooldown=7, cast=0.25, recovery=0.45, range=260, arc=110,
             anim=CA + "Ability_Q", rate=1.15, impact_fx=C + "/FX/Particles/Abilities/BlinkStrike/FX/P_Countess_BlinkStrike_HitFX", fx_scale=0.8, kick=0.5,
             ai=dict(priority=1.3, min=0, max=280)),
        # Card: "Absorbs damage and negates control."
        dict(id="VoidShield", name="Void Shield", short="Void Shield", kind=K.SELF_GUARD,
             desc="Absorbs damage and negates control: for 1.3 s Nyx is wrapped in void, untargetable and immune to hits.",
             cooldown=14, cast=0.05, recovery=0.0, status=ST.VEILED, status_duration=1.3, interruptible=False,
             anim=CA + "Ability_E", rate=1.3, cast_fx=C + "/FX/Particles/Abilities/ShadowClone/FX/p_Countess_ShadowCloneBurst", fx_scale=1.0, kick=0.0,
             ai=dict(priority=1.5, min=0, max=400, max_hp=0.7)),
        # Card: "Steals energy and weakens." Ultimate here. (energy steal not implemented yet: drains health + weakens)
        dict(id="SoulDrain", name="Soul Drain", short="Soul Drain", kind=K.AURA, ult=True,
             desc="Ultimate. Nyx drains the souls around her: enemies within 4.5 m take damage and are weakened "
                  "(Dread: slower, softer divine abilities) for 4 s.",
             damage=18, cooldown=40, momentum=100, cast=0.6, recovery=0.6, radius=450, status=ST.DREAD, status_duration=4.0, magnitude=0.35,
             blockable=False, interruptible=False,
             anim=CA + "Ability_Ultimate", cast_fx=C + "/FX/Particles/Abilities/Ultimate/FX/p_CountessUlt_CastFX", impact_fx=C + "/FX/Particles/Abilities/Ultimate/FX/p_CountessUlt_SlashFX", fx_scale=0.9, kick=0.9,
             ai=dict(priority=2.0, min=0, max=430)),
    ]),
    "Varoth": dict(health=1.05, gap=4.0, look=LOOK.VOID, abilities=[
        # Card: "Area damage. Launch enemies." Signature ability.
        dict(id="InfernoSlam", name="Inferno Slam", short="Inferno Slam", kind=K.AREA_BURST,
             desc="Area damage. Varoth slams the ground: enemies within 3.5 m are hit and launched away.",
             damage=22, cooldown=8, cast=0.6, recovery=0.6, radius=350, magnitude=650,
             anim=SA + "Swing3_Slow", cast_fx=None, impact_fx=S + "/FX/Particles/Abilities/Subjugate/FX/P_Sevarog_Subjugate_Blast", fx_scale=0.9, kick=1.6,
             ai=dict(priority=1.0, min=0, max=330)),
        # Card: "High-impact rush attack." (replaces the non-card Grasp of the Void pull)
        dict(id="AshenCharge", name="Ashen Charge", short="Ashen Charge", kind=K.CHARGE,
             desc="High-impact rush: Varoth bursts forward; the first enemy struck is knocked back. Sidestep the line.",
             damage=16, cooldown=11, cast=0.3, recovery=0.5, range=700, interval=0.4, magnitude=950,
             anim=SA + "Speedburst", cast_fx=S + "/FX/Particles/Abilities/SoulSiphon/FX/P_SiphonCasting", impact_fx=S + "/FX/Particles/Abilities/Primary/FX/P_Sevarog_Melee_SucessfulImpact", fx_scale=1.0, kick=1.5,
             ai=dict(priority=1.2, min=400, max=700)),
        # Card: "Burning area. Damage over time." (DoT not implemented yet: one burn tick + Dread)
        dict(id="WorldBurn", name="World Burn", short="World Burn", kind=K.AURA,
             desc="Burning area: enemies within 5 m are scorched and gripped by dread for 4 s - 35% slower and "
                  "their divine abilities hit softer.",
             damage=5, cooldown=16, cast=0.5, recovery=0.4, radius=500, status=ST.DREAD, status_duration=4.0, magnitude=0.35,
             anim=SA + "Cast", cast_fx=S + "/FX/Particles/Abilities/Subjugate/FX/P_SubjugateSwirls", impact_fx=S + "/FX/Particles/Abilities/Subjugate/FX/P_Sub_Cast", fx_scale=1.2, kick=0.6,
             ai=dict(priority=1.1, min=0, max=480)),
        # Card: "Increased power, defense, and area damage." (defense part: Unbreakable; power/area bonus not implemented yet)
        dict(id="ColossusForm", name="Colossus Form", short="Colossus Form", kind=K.SELF_GUARD, ult=True,
             desc="Ultimate. Varoth becomes the colossus for 6 s: half of all damage is shrugged off and he cannot "
                  "be staggered.",
             cooldown=40, momentum=100, cast=0.4, recovery=0.3, status=ST.UNBREAKABLE, status_duration=6.0, magnitude=0.5, interruptible=False,
             anim=SA + "Subjugation", cast_fx=S + "/FX/Particles/Abilities/Ultimate/FX/P_UltActivate", fx_scale=1.4, fx_life=6.0, kick=1.0,
             ai=dict(priority=2.0, min=0, max=500, max_hp=0.8)),
    ]),
    # Signature-ability milestone (Docs/GOTA_HERO_ABILITIES_PLAN.md): one card ability per new god, slot 0.
    "Zephyr": dict(health=0.95, gap=3.5, look=LOOK.SHADOW, abilities=[
        dict(id="GaleRush", name="Gale Rush", short="Gale Rush", kind=K.CHARGE,
             desc="High-speed dash attack. Zephyr rides the wind straight at the enemy; the first one struck is "
                  "knocked away. Sidestep the line to avoid it.",
             damage=16, cooldown=8, cast=0.15, recovery=0.35, range=700, interval=0.3, magnitude=750,
             anim=KWA + "Ability_RMB", cast_fx=KWFX + "Ultimate/FX/P_KwangAfterImage",
             impact_fx=KWFX + "Primary/FX/P_Kwang_Primary_Impact", fx_scale=1.0, kick=1.2,
             ai=dict(priority=1.2, min=350, max=700)),
    ]),
    "Lysara": dict(health=1.0, gap=3.5, look=LOOK.VOID, abilities=[
        dict(id="GravityWell", name="Gravity Well", short="Gravity Well", kind=K.PULL,
             desc="Lysara bends gravity around a visible enemy and drags them to her, staggering them. Needs line "
                  "of sight; breaking sight or leaving range during the cast makes it fizzle.",
             damage=10, cooldown=12, cast=0.5, recovery=0.4, range=850, magnitude=1600,
             anim=AUA + "Ability_R_InPlace", cast_fx=AUFX + "Ultimate/FX/P_Aurora_Ultimate_Warmup",
             impact_fx=AUFX + "Freeze/FX/P_Aurora_Freeze_Whrilwind", fx_scale=1.0, kick=0.8,
             ai=dict(priority=1.3, min=380, max=840)),
    ]),
    "Kairos": dict(health=1.0, gap=3.5, look=LOOK.VOID, abilities=[
        dict(id="TimeLash", name="Time Lash", short="Time Lash", kind=K.AURA,
             desc="Area control. Kairos lashes time around him: enemies within 4.5 m take light damage and are "
                  "Slowed (40% movement speed) for 3 seconds. Their attacks and abilities are unaffected.",
             damage=6, cooldown=12, cast=0.5, recovery=0.4, radius=450, status=ST.SLOWED, status_duration=3.0,
             magnitude=0.4, anim=GDA + "Burden", cast_fx=GDFX + "Portal/FX/P_Portal_Cast",
             impact_fx=GDFX + "Portal/FX/P_Portal_Entrance", fx_scale=1.2, kick=0.6,
             ai=dict(priority=1.2, min=0, max=430)),
    ]),
    # Gravity Slam: close-range, telegraphed area crush (not a second pull). Steel's GroundSmash_End lands at 0.16 s;
    # played at 0.229x the slam lands exactly at the end of the 0.7 s wind-up (the server's damage moment).
    "Orion": dict(health=1.1, gap=3.5, look=LOOK.TITAN, abilities=[
        dict(id="GravitySlam", name="Gravity Slam", short="Gravity Slam", kind=K.AREA_BURST,
             desc="Orion charges gravity overhead, then crushes the ground around him: everything within 3.3 m is "
                  "staggered. Clearly telegraphed and unblockable - get out of range or dodge through the impact.",
             damage=20, cooldown=10, cast=0.7, recovery=0.6, radius=330, magnitude=0, blockable=False,
             interruptible=True, anim=STLA + "Steel_Ability_GroundSmash_End", rate=0.229,
             cast_fx=STL + "/FX/Particles/Steel/Abilities/Melee/FX/P_Steel_Melee_Charge_A",
             impact_fx=STL + "/FX/Particles/Steel/Abilities/Ultimate/FX/P_Steel_Ultimate_Impact", fx_scale=0.8, kick=1.8,
             ai=dict(priority=1.3, min=0, max=300)),
    ]),
}


def P(*a):
    line = " ".join(str(x) for x in a)
    log.append(line)
    unreal.log("GOTA_HeroAbilities: " + line)


def find_asset(path_hint, cls_name):
    """Exact path if it exists, else search the pack by asset name (folder layouts differ per pack)."""
    if not path_hint:
        return None
    if EAL.does_asset_exist(path_hint):
        return unreal.load_asset(path_hint)
    name = path_hint.split("/")[-1]
    root = "/".join(path_hint.split("/")[:3])
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    for a in ar.get_assets_by_path(root, recursive=True):
        if str(a.asset_name) == name:
            return unreal.load_asset(str(a.package_name))
    P("  MISSING", cls_name, path_hint)
    return None


def overlay_material(name, color, strength):
    path = MAT + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("used_with_skeletal_mesh", True)
    fres = MEL.create_material_expression(mat, unreal.MaterialExpressionFresnel, -600, 0)
    fres.set_editor_property("exponent", 2.2)
    col = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -600, -200)
    col.set_editor_property("constant", unreal.LinearColor(color[0] * strength, color[1] * strength, color[2] * strength, 1))
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -100)
    MEL.connect_material_expressions(col, "", mul, "A")
    MEL.connect_material_expressions(fres, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    op = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 150)
    k = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 250)
    k.set_editor_property("r", 0.85)
    MEL.connect_material_expressions(fres, "", op, "A")
    MEL.connect_material_expressions(k, "", op, "B")
    MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_asset(path)
    P("overlay", name)


def build_set(hero, cfg):
    name = "DA_AbilitySet_" + hero
    path = OUT + "/" + name
    # Update in place: DA_Hero_<Hero> references the set, so deleting it fails and create_asset returns None.
    da = EAL.load_asset(path) if EAL.does_asset_exist(path) else None
    if not da:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.GOTAAbilitySet)
        da = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, OUT, unreal.GOTAAbilitySet, factory)
    defs = []
    for a in cfg["abilities"]:
        d = unreal.GOTAAbilityDef()
        d.set_editor_property("ability_id", a["id"])
        d.set_editor_property("display_name", unreal.Text(a["name"]))
        d.set_editor_property("short_label", unreal.Text(a["short"]))
        d.set_editor_property("description", unreal.Text(a["desc"]))
        d.set_editor_property("kind", a["kind"])
        d.set_editor_property("ultimate", bool(a.get("ult", False)))
        d.set_editor_property("damage", float(a.get("damage", 0)))
        d.set_editor_property("cooldown", float(a["cooldown"]))
        d.set_editor_property("momentum_cost", float(a.get("momentum", 0)))
        d.set_editor_property("cast_time", float(a["cast"]))
        d.set_editor_property("recovery_time", float(a["recovery"]))
        d.set_editor_property("range", float(a.get("range", 300)))
        d.set_editor_property("radius", float(a.get("radius", 0)))
        d.set_editor_property("arc_degrees", float(a.get("arc", 120)))
        d.set_editor_property("hit_count", int(a.get("hits", 1)))
        d.set_editor_property("hit_interval", float(a.get("interval", 0.15)))
        d.set_editor_property("status", a.get("status", ST.NONE))
        d.set_editor_property("status_duration", float(a.get("status_duration", 0)))
        d.set_editor_property("magnitude", float(a.get("magnitude", 0)))
        d.set_editor_property("can_be_blocked", bool(a.get("blockable", True)))
        d.set_editor_property("interruptible", bool(a.get("interruptible", True)))
        d.set_editor_property("look", cfg["look"])
        d.set_editor_property("impact_kick", float(a.get("kick", 1.0)))
        d.set_editor_property("anim_play_rate", float(a.get("rate", 1.0)))
        d.set_editor_property("fx_scale", float(a.get("fx_scale", 1.0)))
        d.set_editor_property("fx_lifetime", float(a.get("fx_life", 1.5)))
        anim = find_asset(a.get("anim"), "anim")
        if anim:
            d.set_editor_property("animation", anim)
            # Fit the hero animation into the ability window (cast + recovery; the server blends it out at the
            # end). Explicit "rate" in the table wins; otherwise speed up long sequences, never slow them down.
            if "rate" not in a or a.get("auto_rate"):
                window = float(a["cast"]) + float(a["recovery"]) + float(a.get("hits", 1) - 1) * float(a.get("interval", 0.15))
                rate = min(max(anim.get_play_length() / max(window, 0.2), 1.0), 2.2)
                d.set_editor_property("anim_play_rate", rate)
        for key, prop, cls in (("cast_fx", "cast_fx", "fx"), ("impact_fx", "impact_fx", "fx"), ("cast_sfx", "cast_sound", "sound"), ("impact_sfx", "impact_sound", "sound")):
            asset = find_asset(a.get(key), cls)
            if asset:
                ok_type = isinstance(asset, unreal.ParticleSystem) if cls == "fx" else isinstance(asset, unreal.SoundBase)
                if ok_type:
                    d.set_editor_property(prop, asset)
                else:
                    P("  wrong type for", prop, asset.get_class().get_name())
        ai = a.get("ai", {})
        d.set_editor_property("ai_priority", float(ai.get("priority", 1.0)))
        d.set_editor_property("ai_min_distance", float(ai.get("min", 0)))
        d.set_editor_property("ai_max_distance", float(ai.get("max", 400)))
        d.set_editor_property("ai_max_own_health_fraction", float(ai.get("max_hp", 1.0)))
        defs.append(d)
        P(" ", hero, a["id"], "anim=%s castfx=%s impactfx=%s" % (bool(anim), bool(d.get_editor_property("cast_fx")), bool(d.get_editor_property("impact_fx"))))
    da.set_editor_property("abilities", defs)
    da.set_editor_property("max_health_scale", float(cfg["health"]))
    da.set_editor_property("ai_global_gap", float(cfg["gap"]))
    # A running game/editor holding the package makes the save fail silently: report it.
    if not EAL.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError("could not save %s (is a game or editor holding it open?)" % path)
    hero_def = EAL.load_asset("/Game/Heroes/Data/DA_Hero_" + hero)
    hero_def.set_editor_property("ability_set", da)
    EAL.save_asset("/Game/Heroes/Data/DA_Hero_" + hero, only_if_is_dirty=False)
    P(hero, "set saved with", len(defs), "abilities; linked to DA_Hero_" + hero)


if not EAL.does_asset_exist(MAT + "/M_GOTA_Overlay_Slowed"):
    EAL.make_directory(MAT)
    overlay_material("M_GOTA_Overlay_Slowed", (0.25, 0.55, 1.0), 4.0)  # time-blue, Kairos' Time Lash
# GOTA_HEROES=Zephyr builds only those sets (and leaves the status overlay materials alone).
ONLY = [h.strip() for h in os.environ.get("GOTA_HEROES", "").split(",") if h.strip()]
EAL.make_directory(MAT)
if not ONLY:
    overlay_material("M_GOTA_Overlay_Unbreakable", (1.0, 0.72, 0.25), 6.0)
    overlay_material("M_GOTA_Overlay_Veiled", (0.35, 0.08, 0.55), 4.0)
    overlay_material("M_GOTA_Overlay_Dread", (0.25, 1.0, 0.35), 4.0)
for hero, cfg in HEROES.items():
    if ONLY and hero not in ONLY:
        continue
    try:
        build_set(hero, cfg)
    except Exception as e:
        P(hero, "FAIL", e)
open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_BuildHeroAbilities.txt"), "w").write("\n".join(log))
