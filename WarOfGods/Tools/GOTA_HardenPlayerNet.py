"""Network hardening of BP_Player (see Docs/GOTA_MULTIPLAYER_AUDIT.md B11/B12).

1. Reparent BP_Player from ACharacter to AGOTAPlayerCharacter (server-side validation of its SR_* combat RPCs).
2. Mark its SR_* (client -> server) custom events Reliable; MC_* cosmetic multicasts stay Unreliable.
Optional: GOTA_EXPORT_DIR=<dir> exports the graphs as text before changing anything (audit).
Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
"""
import os

import unreal

EAL = unreal.EditorAssetLibrary
PATH = "/Game/Player/BP_Player"


def main():
    bp = unreal.load_asset(PATH)
    if not bp:
        raise RuntimeError("cannot load " + PATH)
    out = os.environ.get("GOTA_EXPORT_DIR")
    if out:
        n = unreal.GOTAEditorUIBuilderLibrary.export_blueprint_graphs(bp, out)
        unreal.log("GOTA_HARDEN exported %d graphs to %s" % (n, out))
    parent = unreal.load_class(None, "/Script/WarOfGods.GOTAPlayerCharacter")
    if not parent:
        raise RuntimeError("AGOTAPlayerCharacter not found (build the editor first)")
    unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)  # idempotent
    gen = bp.generated_class()
    unreal.log("GOTA_HARDEN reparented BP_Player -> %s (is child: %s)" % (parent.get_name(), unreal.MathLibrary.class_is_child_of(gen, parent)))
    changed = unreal.GOTAEditorUIBuilderLibrary.make_server_events_reliable(bp, "SR_")
    unreal.log("GOTA_HARDEN reliable SR_ events changed: %d" % changed)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    if not EAL.save_asset(PATH, only_if_is_dirty=False):
        raise RuntimeError("could not save " + PATH)
    unreal.log("GOTA_HARDEN saved " + PATH)


main()
