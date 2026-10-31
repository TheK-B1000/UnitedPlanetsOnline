# Arena overhaul — material families (run in the full editor; idempotent).
#   M_GOTA_ArenaSurface : world-aligned (triplanar) PBR surface with macro anti-tiling. Every
#                         kitbashed structure and gameplay floor uses an instance of it, so
#                         scaled primitives never show stretched UVs.
#   M_GOTA_ArenaEmissive: panning emissive for lava / energy conduits / UPG light strips.
#   MI_GOTA_<Family>     : biome material instances (stone, sandstone, obsidian, snow, ice, moss,
#                          wet rock, metal, lava, energy …) from Megascans / Iceland surfaces.
import os

import unreal

MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
ROOT = "/Game/Arenas/Shared/Materials"
WAT = "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture"
WAN = "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedNormal.WorldAlignedNormal"
LOG = []


def log(msg):
    LOG.append(msg)
    unreal.log("[GOTA mats] " + msg)


def recreate_material(name):
    # Always rebuilt from scratch: clearing expressions in place left stale nodes behind.
    path = ROOT + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    return TOOLS.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())


def node(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def param_scalar(mat, name, value, x, y):
    n = node(mat, unreal.MaterialExpressionScalarParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", value)
    return n


def param_vector(mat, name, value, x, y):
    n = node(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", value)
    return n


def param_texobj(mat, name, default_path, x, y, normal=False):
    n = node(mat, unreal.MaterialExpressionTextureObjectParameter, x, y)
    n.set_editor_property("parameter_name", name)
    tex = EAL.load_asset(default_path)
    if tex:
        n.set_editor_property("texture", tex)
    if normal:
        n.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    return n


def func(mat, path, x, y):
    n = node(mat, unreal.MaterialExpressionMaterialFunctionCall, x, y)
    f = unreal.load_asset(path)
    n.set_editor_property("material_function", f)
    return n


def c(a, out, b, inp):
    ok = MEL.connect_material_expressions(a, out, b, inp)
    if not ok:
        log("WARN connect failed: %s.%s -> %s.%s" % (a.get_name(), out, b.get_name(), inp))
    return ok


def vec3_from_scalar(mat, s, x, y):
    # Function Vector3 inputs accept a scalar (the compiler replicates it).
    return s


def build_surface_master():
    mat = recreate_material("M_GOTA_ArenaSurface")
    albedo = param_texobj(mat, "Albedo", "/Game/Megascans/Surfaces/Quarry_Rock_Cliff_01/T_Quarry_Rock_Cliff_01_A", -1600, -300)
    normal = param_texobj(mat, "Normal", "/Game/Megascans/Surfaces/Quarry_Rock_Cliff_01/T_Quarry_Rock_Cliff_01_N", -1600, 200, normal=True)
    rough = param_texobj(mat, "Roughness", "/Game/Megascans/Surfaces/Quarry_Rock_Cliff_01/T_Quarry_Rock_Cliff_01_R", -1600, 600)
    tile = param_scalar(mat, "TileSize", 300.0, -1600, 900)
    macro = param_scalar(mat, "MacroScale", 7.0, -1600, 1000)
    macro_k = param_scalar(mat, "MacroStrength", 0.55, -1600, 1100)
    tint = param_vector(mat, "Tint", unreal.LinearColor(1, 1, 1, 1), -600, -500)
    rchan = param_vector(mat, "RoughnessChannel", unreal.LinearColor(1, 0, 0, 0), -600, 700)
    rscale = param_scalar(mat, "RoughnessScale", 1.0, -600, 850)
    rbias = param_scalar(mat, "RoughnessBias", 0.0, -600, 950)
    metal = param_scalar(mat, "Metallic", 0.0, -300, 400)
    contrast = param_scalar(mat, "ProjectionContrast", 0.6, -1600, 1200)

    size3 = vec3_from_scalar(mat, tile, -1350, 900)
    mul_macro = node(mat, unreal.MaterialExpressionMultiply, -1350, 1000)
    c(tile, "", mul_macro, "A")
    c(macro, "", mul_macro, "B")
    macro3 = vec3_from_scalar(mat, mul_macro, -1200, 1000)

    wa = func(mat, WAT, -1000, -300)
    c(albedo, "", wa, "TextureObject")
    c(size3, "", wa, "TextureSize")
    c(contrast, "", wa, "ProjectionTransitionContrast")
    wm = func(mat, WAT, -1000, -100)
    c(albedo, "", wm, "TextureObject")
    c(macro3, "", wm, "TextureSize")
    wn = func(mat, WAN, -1000, 200)
    c(normal, "", wn, "TextureObject")
    c(size3, "", wn, "TextureSize")
    c(contrast, "", wn, "ProjectionTransitionContrast")
    wr = func(mat, WAT, -1000, 600)
    c(rough, "", wr, "TextureObject")
    c(size3, "", wr, "TextureSize")
    c(contrast, "", wr, "ProjectionTransitionContrast")

    # Base colour = tint * albedo * lerp(1, macro luminance * 2.2, MacroStrength)
    desat = node(mat, unreal.MaterialExpressionDesaturation, -750, -100)
    c(wm, "XYZ Texture", desat, "")
    mk = node(mat, unreal.MaterialExpressionMultiply, -600, -100)
    c(desat, "", mk, "A")
    mk.set_editor_property("const_b", 2.2)
    one = node(mat, unreal.MaterialExpressionConstant, -600, 0)
    one.set_editor_property("r", 1.0)
    lerp = node(mat, unreal.MaterialExpressionLinearInterpolate, -450, -100)
    c(one, "", lerp, "A")
    c(mk, "", lerp, "B")
    c(macro_k, "", lerp, "Alpha")
    m1 = node(mat, unreal.MaterialExpressionMultiply, -450, -300)
    c(wa, "XYZ Texture", m1, "A")
    c(tint, "", m1, "B")
    m2 = node(mat, unreal.MaterialExpressionMultiply, -250, -250)
    c(m1, "", m2, "A")
    c(lerp, "", m2, "B")
    MEL.connect_material_property(m2, "", unreal.MaterialProperty.MP_BASE_COLOR)

    MEL.connect_material_property(wn, "XYZ Texture", unreal.MaterialProperty.MP_NORMAL)

    dot = node(mat, unreal.MaterialExpressionDotProduct, -450, 650)
    c(wr, "XYZ Texture", dot, "A")
    c(rchan, "", dot, "B")
    rm = node(mat, unreal.MaterialExpressionMultiply, -300, 650)
    c(dot, "", rm, "A")
    c(rscale, "", rm, "B")
    ra = node(mat, unreal.MaterialExpressionAdd, -150, 650)
    c(rm, "", ra, "A")
    c(rbias, "", ra, "B")
    rc = node(mat, unreal.MaterialExpressionSaturate, -50, 650)
    c(ra, "", rc, "")
    MEL.connect_material_property(rc, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

    MEL.recompile_material(mat)
    EAL.save_asset(ROOT + "/M_GOTA_ArenaSurface")
    log("M_GOTA_ArenaSurface built (%d expressions)" % MEL.get_num_material_expressions(mat))
    return mat


def build_emissive_master():
    mat = recreate_material("M_GOTA_ArenaEmissive")
    color = param_vector(mat, "EmissiveColor", unreal.LinearColor(1.0, 0.35, 0.05, 1), -900, -200)
    strength = param_scalar(mat, "EmissiveStrength", 20.0, -900, -50)
    pattern = param_texobj(mat, "Pattern", "/Game/Megascans/Surfaces/Icelandic_Lava_Rocks_smvmacab/T_Icelandic_Lava_Rocks_smvmacab_8K_D", -1200, 150)
    tile = param_scalar(mat, "TileSize", 400.0, -1200, 400)
    speed = param_scalar(mat, "PanSpeed", 0.02, -1200, 500)
    power = param_scalar(mat, "PatternPower", 2.5, -1200, 600)
    base = param_vector(mat, "BaseColor", unreal.LinearColor(0.02, 0.012, 0.01, 1), -300, -350)
    rough = param_scalar(mat, "Roughness", 0.6, -300, 450)

    # World-space XY UVs, slowly panning — lava flow / energy current.
    wp = node(mat, unreal.MaterialExpressionWorldPosition, -1200, 250)
    mask = node(mat, unreal.MaterialExpressionComponentMask, -1050, 250)
    mask.set_editor_property("r", True)
    mask.set_editor_property("g", True)
    c(wp, "", mask, "")
    div = node(mat, unreal.MaterialExpressionDivide, -900, 250)
    c(mask, "", div, "A")
    c(tile, "", div, "B")
    t = node(mat, unreal.MaterialExpressionTime, -1050, 500)
    tm = node(mat, unreal.MaterialExpressionMultiply, -900, 500)
    c(t, "", tm, "A")
    c(speed, "", tm, "B")
    add = node(mat, unreal.MaterialExpressionAdd, -750, 300)
    c(div, "", add, "A")
    c(tm, "", add, "B")
    samp = node(mat, unreal.MaterialExpressionTextureSample, -600, 200)
    c(pattern, "", samp, "Tex")
    c(add, "", samp, "UVs")
    inv = node(mat, unreal.MaterialExpressionOneMinus, -450, 200)
    c(samp, "R", inv, "")
    pw = node(mat, unreal.MaterialExpressionPower, -350, 200)
    c(inv, "", pw, "Base")
    c(power, "", pw, "Exp")
    e1 = node(mat, unreal.MaterialExpressionMultiply, -200, 0)
    c(color, "", e1, "A")
    c(strength, "", e1, "B")
    e2 = node(mat, unreal.MaterialExpressionMultiply, -50, 100)
    c(e1, "", e2, "A")
    c(pw, "", e2, "B")
    MEL.connect_material_property(e2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(mat)
    EAL.save_asset(ROOT + "/M_GOTA_ArenaEmissive")
    log("M_GOTA_ArenaEmissive built")
    return mat


S = "/Game/Megascans/Surfaces/"
I = "/Game/Iceland_Environment/Textures/"
R_ONLY = unreal.LinearColor(1, 0, 0, 0)
ORD_G = unreal.LinearColor(0, 1, 0, 0)

# name: (albedo, normal, roughness, roughness channel, tile cm, tint, rough scale, rough bias, metallic, macro strength)
SC = "/Game/StarterContent/Textures/"
ZERO = unreal.LinearColor(0, 0, 0, 0)
# NOTE: only NON-virtual textures may feed these samplers (virtual textures render black here).
# Megascans Icelandic 8K sets and Megascans/Rock are virtual-texture assets, so they are not used.
# name: (albedo, normal, roughness, roughness channel, tile cm, tint, rough scale, rough bias, metallic, macro strength)
FAMILIES = {
    "QuarryStone":   (S + "Quarry_Rock_Cliff_01/T_Quarry_Rock_Cliff_01_A", S + "Quarry_Rock_Cliff_01/T_Quarry_Rock_Cliff_01_N", S + "Quarry_Rock_Cliff_01/T_Quarry_Rock_Cliff_01_R", R_ONLY, 420, (1.0, 0.93, 0.84), 1.0, 0.0, 0.0, 0.5),
    "QuarryBlock":   (SC + "T_Brick_Cut_Stone_D", SC + "T_Brick_Cut_Stone_N", SC + "T_Brick_Cut_Stone_D", ZERO, 320, (1.15, 0.98, 0.8), 0.0, 0.78, 0.0, 0.45),
    "QuarryGravel":  (S + "Quarry_Ground_Gravel_02/T_Quarry_Ground_Gravel_02_A", S + "Quarry_Ground_Gravel_02/T_Quarry_Ground_Gravel_02_N", S + "Quarry_Ground_Gravel_02/T_Quarry_Ground_Gravel_02_R", R_ONLY, 260, (1.0, 0.94, 0.86), 1.0, 0.0, 0.0, 0.5),
    "Paving":        (SC + "T_Rock_Sandstone_D", SC + "T_Rock_Sandstone_N", SC + "T_Rock_Sandstone_D", ZERO, 260, (0.92, 0.8, 0.66), 0.0, 0.72, 0.0, 0.5),
    "Sandstone":     (S + "Sandy_Rock_01/T_Sand_Rocky_01_A", S + "Sandy_Rock_01/T_Sand_Rocky_01_N", S + "Sandy_Rock_01/T_Sand_Rocky_01_R", R_ONLY, 380, (1.05, 0.9, 0.72), 1.0, 0.0, 0.0, 0.5),
    "Sand":          (S + "Rippled_Sand_01/T_Rippled_Sand_01_A", S + "Rippled_Sand_01/T_Rippled_Sand_01_N", S + "Rippled_Sand_01/T_Rippled_Sand_01_R", R_ONLY, 320, (1.05, 0.92, 0.75), 1.0, 0.0, 0.0, 0.6),
    "Obsidian":      (SC + "T_Rock_Basalt_D", SC + "T_Rock_Basalt_N", SC + "T_Rock_Basalt_D", ZERO, 300, (0.2, 0.19, 0.21), 0.0, 0.5, 0.0, 0.4),
    "Basalt":        (SC + "T_Rock_Basalt_D", SC + "T_Rock_Basalt_N", SC + "T_Rock_Basalt_D", ZERO, 340, (0.72, 0.66, 0.62), 0.0, 0.8, 0.0, 0.5),
    "Snow":          (I + "Snow/T_snow_02_diff_1k", I + "Snow/T_snow_02_nor_1k", I + "Snow/T_snow_02_rough_1k", R_ONLY, 280, (0.92, 0.95, 1.0), 1.0, 0.0, 0.0, 0.35),
    "Ice":           (I + "T_Snow/T_Snow_BaseColor", I + "T_Snow/T_Snow_Normal", I + "Snow/T_snow_02_rough_1k", R_ONLY, 600, (0.55, 0.72, 0.9), 0.15, 0.05, 0.0, 0.3),
    "FrostStone":    (SC + "T_Rock_Slate_D", SC + "T_Rock_Slate_N", SC + "T_Rock_Slate_D", ZERO, 340, (0.86, 0.92, 1.02), 0.0, 0.72, 0.0, 0.5),
    "MossStone":     (I + "T_MossyCreekStones/T_MossyCreekStones_A", I + "T_MossyCreekStones/T_MossyCreekStones_N", I + "T_MossyCreekStones/T_MossyCreekStones_R", R_ONLY, 380, (0.95, 1.0, 0.9), 1.0, 0.0, 0.0, 0.5),
    "ForestGround":  (I + "T_ForestGround/T_ForestGround_A", I + "T_ForestGround/T_ForestGround_N", I + "T_ForestGround/T_ForestGround_R", R_ONLY, 300, (1.0, 1.0, 1.0), 1.0, 0.0, 0.0, 0.5),
    "TempleStone":   (SC + "T_Rock_Sandstone_D", SC + "T_Rock_Sandstone_N", SC + "T_Rock_Sandstone_D", ZERO, 300, (0.82, 0.88, 0.76), 0.0, 0.75, 0.0, 0.55),
    "WetRock":       (SC + "T_Rock_Slate_D", SC + "T_Rock_Slate_N", SC + "T_Rock_Slate_D", ZERO, 360, (0.4, 0.42, 0.44), 0.0, 0.22, 0.0, 0.45),
    "CoastStone":    (SC + "T_Rock_Smooth_Granite_D", SC + "T_Rock_Slate_N", SC + "T_Rock_Smooth_Granite_D", ZERO, 400, (0.8, 0.82, 0.85), 0.0, 0.62, 0.0, 0.5),
    "Iron":          (SC + "T_Metal_Steel_D", SC + "T_Metal_Steel_N", SC + "T_Metal_Steel_D", ZERO, 220, (0.32, 0.31, 0.3), 0.0, 0.45, 0.9, 0.35),
    "RustIron":      (SC + "T_Metal_Rust_D", SC + "T_Metal_Rust_N", SC + "T_Metal_Rust_D", ZERO, 180, (1.0, 1.0, 1.0), 0.0, 0.72, 0.45, 0.4),
    "UPGAlloy":      (SC + "T_Metal_Aluminum_D", SC + "T_Tech_Panel_N", SC + "T_Metal_Aluminum_D", ZERO, 500, (0.88, 0.9, 0.94), 0.0, 0.3, 0.95, 0.15),
}

EMISSIVE = {
    # name: (color, strength, tile, pan speed, power, base color)
    "Lava":       ((1.0, 0.28, 0.03), 6.0, 520.0, 0.012, 2.2, (0.03, 0.01, 0.005)),
    "LavaCrack":  ((1.0, 0.22, 0.02), 3.0, 260.0, 0.004, 3.5, (0.02, 0.01, 0.01)),
    "UPGEnergy":  ((0.25, 0.65, 1.0), 4.0, 300.0, 0.05, 1.5, (0.02, 0.03, 0.05)),
    "GoldEnergy": ((1.0, 0.72, 0.32), 2.5, 300.0, 0.03, 1.6, (0.05, 0.04, 0.02)),
}


def make_mi(name, parent):
    path = ROOT + "/MI_GOTA_" + name
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = TOOLS.create_asset("MI_GOTA_" + name, ROOT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    return mi, path


def build_instances(surface, emissive):
    for name, (a, n, r, ch, tile, tint, rs, rb, metal, macro) in FAMILIES.items():
        mi, path = make_mi(name, surface)
        for pname, tex in (("Albedo", a), ("Normal", n), ("Roughness", r)):
            t = EAL.load_asset(tex)
            if not t:
                log("WARN %s: missing texture %s" % (name, tex))
                continue
            MEL.set_material_instance_texture_parameter_value(mi, pname, t)
        MEL.set_material_instance_vector_parameter_value(mi, "RoughnessChannel", ch)
        MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(tint[0], tint[1], tint[2], 1))
        MEL.set_material_instance_scalar_parameter_value(mi, "TileSize", tile)
        MEL.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", rs)
        MEL.set_material_instance_scalar_parameter_value(mi, "RoughnessBias", rb)
        MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", metal)
        MEL.set_material_instance_scalar_parameter_value(mi, "MacroStrength", macro)
        EAL.save_asset(path)
    for name, (col, k, tile, speed, power, base) in EMISSIVE.items():
        mi, path = make_mi(name, emissive)
        MEL.set_material_instance_vector_parameter_value(mi, "EmissiveColor", unreal.LinearColor(col[0], col[1], col[2], 1))
        MEL.set_material_instance_vector_parameter_value(mi, "BaseColor", unreal.LinearColor(base[0], base[1], base[2], 1))
        MEL.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", k)
        MEL.set_material_instance_scalar_parameter_value(mi, "TileSize", tile)
        MEL.set_material_instance_scalar_parameter_value(mi, "PanSpeed", speed)
        MEL.set_material_instance_scalar_parameter_value(mi, "PatternPower", power)
        EAL.save_asset(path)
    log("instances: %d surface + %d emissive" % (len(FAMILIES), len(EMISSIVE)))


def main():
    if not EAL.does_directory_exist(ROOT):
        EAL.make_directory(ROOT)
    rebuild = os.environ.get("GOTA_REBUILD_MASTERS") == "1"
    surface = build_surface_master() if rebuild or not EAL.does_asset_exist(ROOT + "/M_GOTA_ArenaSurface") else EAL.load_asset(ROOT + "/M_GOTA_ArenaSurface")
    emissive = build_emissive_master() if rebuild or not EAL.does_asset_exist(ROOT + "/M_GOTA_ArenaEmissive") else EAL.load_asset(ROOT + "/M_GOTA_ArenaEmissive")
    build_instances(surface, emissive)


if __name__ == "__main__" or True:
    try:
        main()
        RESULT = "PASS"
    except Exception as exc:  # noqa
        import traceback
        log("ERROR " + str(exc))
        log(traceback.format_exc())
        RESULT = "FAIL"
    proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    with open(os.path.join(proj, "Saved", "GOTA_ArenaMaterials.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(LOG) + "\nRESULT: " + RESULT + "\n")
    if os.environ.get("GOTA_QUIT_AFTER_BUILD") == "1":
        unreal.SystemLibrary.quit_editor()
