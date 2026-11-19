"""Status overlay materials for statuses added after Divine Combat Pass 1 (GAS milestone A: Stunned, Frozen).

Same recipe as GOTA_BuildHeroAbilities.overlay_material (unlit translucent fresnel rim on the body's overlay slot), so
every status reads the same way. Only creates missing materials; never touches ability sets.

Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
"""
import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAT = "/Game/Heroes/Abilities/Materials"

OVERLAYS = [
    # name, colour, strength, rim exponent, opacity
    ("M_GOTA_Overlay_Frozen", (0.55, 0.85, 1.0), 7.0, 1.4, 0.95),   # ice: wide pale-cyan rim, nearly opaque
    ("M_GOTA_Overlay_Stunned", (1.0, 0.82, 0.18), 6.0, 2.2, 0.85),  # dazed: gold rim
    # milestone B: buffs bright and warm or clear, debuffs dark or sickly
    ("M_GOTA_Overlay_Shielded", (0.75, 0.95, 1.0), 5.0, 1.6, 0.9),  # bright white-cyan bubble
    ("M_GOTA_Overlay_Empowered", (1.0, 0.35, 0.1), 5.0, 2.4, 0.8),  # ember red
    ("M_GOTA_Overlay_Fortified", (0.7, 0.75, 0.85), 4.0, 2.6, 0.8), # steel
    ("M_GOTA_Overlay_Vulnerable", (0.85, 0.1, 0.55), 5.0, 2.0, 0.85),  # bruised magenta
    ("M_GOTA_Overlay_Hasted", (0.45, 1.0, 0.75), 4.0, 2.8, 0.75),  # quick mint
    ("M_GOTA_Overlay_Weakened", (0.45, 0.4, 0.35), 3.0, 2.0, 0.8),  # dull ash
]


def overlay_material(name, color, strength, exponent, opacity):
    path = MAT + "/" + name
    if EAL.does_asset_exist(path):
        unreal.log("overlay %s exists" % name)
        return
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("used_with_skeletal_mesh", True)
    fres = MEL.create_material_expression(mat, unreal.MaterialExpressionFresnel, -600, 0)
    fres.set_editor_property("exponent", exponent)
    col = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -600, -200)
    col.set_editor_property("constant", unreal.LinearColor(color[0] * strength, color[1] * strength, color[2] * strength, 1))
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, -100)
    MEL.connect_material_expressions(col, "", mul, "A")
    MEL.connect_material_expressions(fres, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    op = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 150)
    k = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 250)
    k.set_editor_property("r", opacity)
    MEL.connect_material_expressions(fres, "", op, "A")
    MEL.connect_material_expressions(k, "", op, "B")
    MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_asset(path)
    unreal.log("overlay %s created" % name)


for entry in OVERLAYS:
    overlay_material(*entry)
