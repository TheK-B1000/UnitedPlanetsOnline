"""Ayla (Desert Storm) as a MetaHuman, built with UE 5.8's in-editor MetaHuman Creator (plugin MetaHumanCharacter,
enabled for the Editor target only). Re-runnable: every step is skipped, with the reason logged, when its
prerequisite is missing, so the same script finishes the character once the prerequisites exist.

Card identity (Tools/GOTA_HeroContent.py, Docs/GOTA_CHARACTER_ASSET_MATRIX.md): silver-white hair, cybernetic arms,
powerful but athletic physique, machine components, blue energy. MetaHuman covers the human base (body, face,
skin, hair). Cybernetic arms and machine parts are NOT something MetaHuman can make: they need a separate
modelled/kitbashed attachment (tracked in the matrix as a remaining requirement).

Prerequisites (user, one-time):
  1. MetaHuman optional content: Epic Games Launcher > UE 5.8 > Options > enable "MetaHuman" optional content
     (installs Engine/Plugins/MetaHuman/MetaHumanCharacter/Content/Optional: hair grooms, clothing, texture synthesis).
  2. Epic account login in the editor (auto-rigging and source textures are Epic cloud services).
Without (2) the character cannot be rigged, and an unrigged MetaHuman cannot be assembled into meshes.

Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
     (rigging/texture steps need an editor session that is logged in to Epic)
Report: Saved/GOTA_BuildAylaMetaHuman.txt
Output: /Game/Heroes/Ayla/MetaHuman/MH_Ayla (character), .../Build (assembled meshes). Not committed to the public
        repo (MetaHuman-derived content, same policy as other Epic content - see Docs/GOTA_THIRD_PARTY_ASSETS.md).
"""
import os
import traceback

import unreal

ROOT = "/Game/Heroes/Ayla/MetaHuman"
CHAR = ROOT + "/MH_Ayla"
OPTIONAL = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_plugins_dir()),
                        "MetaHuman", "MetaHumanCharacter", "Content", "Optional")
# Body: powerful but athletic (card). Measurements in cm; sliders in MetaHuman units.
BODY = {"Height": 176.0, "Masculine/Feminine": 1.4, "Muscularity": 1.3, "Fat": -0.9, "Across Shoulder": 37.0, "Bicep": 31.0}
# Hair: silver-white. Candidates are tried in order (wardrobe items shipped in the optional content).
HAIR_ITEMS = ["/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_StraightBangs.WI_Hair_L_StraightBangs"]
log = []


def P(*a):
    log.append(" ".join(str(x) for x in a))
    unreal.log("GOTA_Ayla: " + log[-1])


def editing(sub, ch, fn):
    if not sub.try_add_object_to_edit(ch):
        P("cannot open for edit (already open?)")
        return
    try:
        fn()
    finally:
        if sub.is_object_added_for_editing(ch):
            sub.remove_object_to_edit(ch)


def main():
    sub = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    ch = unreal.load_asset(CHAR) if unreal.EditorAssetLibrary.does_asset_exist(CHAR) else None
    if not ch:
        ch = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MH_Ayla", ROOT, unreal.MetaHumanCharacter, unreal.new_object(type=unreal.MetaHumanCharacterFactoryNew))
        P("created", CHAR)
    has_optional = os.path.isdir(OPTIONAL)
    P("optional content:", "installed" if has_optional else "MISSING (no hair/clothing/skin synthesis)")

    def body():
        cons = {str(c.name): c for c in sub.get_body_constraints(ch)}
        for k, v in BODY.items():
            cons[k].is_active = True
            cons[k].target_measurement = v
        sub.set_body_constraints(ch, list(cons.values()))
        sub.commit_body_state(ch)
        P("body sculpted:", {str(c.name): round(c.target_measurement, 1) for c in sub.get_body_constraints(ch) if str(c.name) in BODY or str(c.name) in ("Chest", "Waist", "Hip")})
    editing(sub, ch, body)

    def rig_and_textures():
        req = unreal.MetaHumanCharacterAutoRiggingRequestParams()
        req.blocking = True
        req.report_progress = False
        req.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY  # the enum in 5.8 python exposes JOINTS_ONLY
        sub.request_auto_rigging(ch, req)
        tex = unreal.MetaHumanCharacterTextureRequestParams()
        tex.blocking = True
        tex.report_progress = False
        sub.request_texture_sources(ch, tex)
    editing(sub, ch, rig_and_textures)
    rigged = sub.can_build_meta_human(ch)
    P("rigged / buildable:", rigged, "" if rigged else "(auto-rig needs an Epic login in the editor)")

    if has_optional:
        for item_path in HAIR_ITEMS:
            item = unreal.load_asset(item_path)
            if not item:
                continue
            key = ch.internal_collection.try_add_item_from_wardrobe_item("Hair", item)
            ch.internal_collection.default_instance.try_add_slot_selection(
                unreal.MetaHumanPipelineSlotSelection(slot_name="Hair", selected_item=key))

            def hair():
                sub.assemble_for_preview(character=ch)
                params = ch.internal_collection.default_instance.get_instance_parameters(
                    item_path=unreal.MetaHumanPaletteItemPath(item_key=key))
                P("hair params:", [str(p.name) for p in params])
                for p in params:
                    if str(p.name) == "Melanin":
                        p.set_float(value=0.0)      # silver-white
                    elif str(p.name) == "Redness":
                        p.set_float(value=0.0)
            editing(sub, ch, hair)
            P("hair:", item_path.split(".")[-1], "set silver-white")
            break
    else:
        P("hair: skipped (optional content missing)")

    if rigged:
        def build():
            bp = unreal.MetaHumanCharacterEditorBuildParameters()
            bp.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
            bp.pipeline_quality = unreal.MetaHumanQualityLevel.MEDIUM
            bp.absolute_build_path = ROOT + "/Build"
            bp.common_folder_path = ROOT + "/Common"
            bp.enable_wardrobe_item_validation = False
            sub.build_meta_human(character=ch, params=bp)
        editing(sub, ch, build)
        ar = unreal.AssetRegistryHelpers.get_asset_registry()
        for a in ar.get_assets_by_path(ROOT + "/Build", recursive=True):
            if str(a.asset_class_path.asset_name) in ("SkeletalMesh", "Skeleton", "Blueprint"):
                P("built", a.asset_class_path.asset_name, a.package_name)
    else:
        P("build: skipped (character not rigged)")
    unreal.EditorAssetLibrary.save_asset(CHAR, only_if_is_dirty=False)


try:
    main()
except Exception:
    P("FAIL", traceback.format_exc())
open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_BuildAylaMetaHuman.txt"), "w").write("\n".join(log))
