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
