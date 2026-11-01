"""Applies prototype bodies (GOTA_HeroContent.BODIES) to the existing hero definitions only.

Same rule as GOTA_BuildMenuUI.apply_hero_body, without rebuilding the menus.
Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_BuildHeroBodies.txt
"""
import importlib
import os
import sys

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
sys.path.insert(0, os.path.join(PROJECT, "Tools"))
import GOTA_HeroContent as HC  # noqa: E402

importlib.reload(HC)
EAL = unreal.EditorAssetLibrary
log = []

HIDDEN_SLOT_MATERIAL = "/Game/Heroes/Materials/M_GOTA_HiddenSlot"


def ensure_hidden_material():
    """Project-owned invisible material for hiding body material slots (masked, opacity mask 0)."""
    if EAL.does_asset_exist(HIDDEN_SLOT_MATERIAL):
        return
    folder, name = HIDDEN_SLOT_MATERIAL.rsplit("/", 1)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("used_with_skeletal_mesh", True)
    zero = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 0)
    zero.set_editor_property("r", 0.0)
    unreal.MaterialEditingLibrary.connect_material_property(zero, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    EAL.save_asset(HIDDEN_SLOT_MATERIAL)
    log.append("created " + HIDDEN_SLOT_MATERIAL)


ensure_hidden_material()

for hero_id, b in HC.BODIES.items():
    path = "/Game/Heroes/Data/DA_Hero_" + hero_id
    da = EAL.load_asset(path)
    mesh = unreal.load_asset(b["mesh"])
    rtg = unreal.load_asset(b["retargeter"])
    if not da or not mesh or not rtg:
        log.append("%s FAIL def=%s mesh=%s rtg=%s" % (hero_id, bool(da), bool(mesh), bool(rtg)))
        continue
    da.set_editor_property("body_mesh", mesh)
    da.set_editor_property("body_retargeter", rtg)
    da.set_editor_property("body_scale", float(b["scale"]))
    da.set_editor_property("body_source_note", unreal.Text(b["note"]))
    da.set_editor_property("hide_attached_weapon_meshes", bool(b.get("hide_weapons", True)))
    da.set_editor_property("body_extra_meshes", [unreal.load_asset(x) for x in b.get("extras", [])])
    da.set_editor_property("body_hidden_material_slots", list(b.get("hidden_slots", [])))
    if b.get("hidden_material"):
        da.set_editor_property("body_hidden_material", unreal.load_asset(b["hidden_material"]))
    da.set_editor_property("gameplay_pawn_class", unreal.load_class(None, "/Game/Player/BP_Player.BP_Player_C"))
    da.set_editor_property("availability", getattr(unreal.GOTAHeroAvailability, b["status"]))
    da.set_editor_property("unavailable_reason", unreal.Text("" if b["status"] == "PLAYABLE" else HC.IN_DEVELOPMENT_REASON))
    EAL.save_asset(path)
    log.append("%s OK %s status=%s scale=%.2f" % (hero_id, mesh.get_name(), b["status"], b["scale"]))

open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_BuildHeroBodies.txt"), "w").write("\n".join(log))
