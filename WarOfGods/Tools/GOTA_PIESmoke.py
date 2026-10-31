"""PIE smoke test: opens an authored arena, plays in editor, checks the duel and hero body, then quits.

Run with the full editor (not -Cmd) so PIE can tick:
  UnrealEditor.exe <uproject> -ExecutePythonScript=<this> -nosplash
Env: GOTA_PIE_MAP (default /Game/Arenas/Maps/L_Arena_TitansQuarry), GOTA_PIE_SECONDS (default 25)
Report: Saved/GOTA_PIESmoke.txt
"""
import os
import time

import unreal

MAP = os.environ.get("GOTA_PIE_MAP", "/Game/Arenas/Maps/L_Arena_TitansQuarry")
SECONDS = float(os.environ.get("GOTA_PIE_SECONDS", "25"))
REPORT = os.path.join(unreal.Paths.project_saved_dir(), "GOTA_PIESmoke.txt")
log = []
state = {"phase": "load", "t0": 0.0, "handle": None}


def P(*a):
    line = " ".join(str(x) for x in a)
    log.append(line)
    unreal.log("GOTA_PIESmoke: " + line)
    open(REPORT, "w").write("\n".join(log))


def pie_world():
    for w in unreal.EditorLevelLibrary.get_pie_worlds(False) if hasattr(unreal.EditorLevelLibrary, "get_pie_worlds") else []:
        return w
    sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    return sub.get_game_world() if sub else None


def inspect(world):
    gm = unreal.GameplayStatics.get_game_mode(world)
    P("game mode:", gm.get_class().get_name() if gm else None)
    pawns = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Pawn)
    for p in pawns:
        comps = [c.get_name() for c in p.get_components_by_class(unreal.ActorComponent)]
        body = [c for c in comps if c.startswith("GOTAHeroBody")]
        P("pawn:", p.get_name(), "body comps:", body)


def tick(dt):
    le = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    now = time.time()
    if state["phase"] == "load":
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
        # Same URL options the menu uses; DevHeroes lets an In Development hero spawn for testing.
        unreal.SystemLibrary.execute_console_command(None, "gota.Heroes.AllowInDevelopment 1")
        P("loaded", MAP)
        state["phase"] = "play"
        state["t0"] = now
    elif state["phase"] == "play" and now - state["t0"] > 3:
        le.editor_request_begin_play()
        P("PIE requested")
        state["phase"] = "running"
        state["t0"] = now
    elif state["phase"] == "running" and now - state["t0"] > SECONDS:
        world = pie_world()
        P("PIE world:", world.get_name() if world else None, "is playing:", le.is_in_play_in_editor())
        if world:
            inspect(world)
        le.editor_request_end_play()
        state["phase"] = "quit"
        state["t0"] = now
    elif state["phase"] == "quit" and now - state["t0"] > 3:
        P("PIE ended; quitting")
        unreal.unregister_slate_post_tick_callback(state["handle"])
        unreal.SystemLibrary.quit_editor()


open(REPORT, "w").write("")
state["handle"] = unreal.register_slate_post_tick_callback(tick)
