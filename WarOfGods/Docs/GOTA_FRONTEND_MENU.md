# Gods of the Arena — Frontend (index)

**Branch:** `feature/gota-cinematic-frontend` · **Engine:** UE 5.8
**Visual reference:** `Docs/References/GOTA_UPG_MainMenu_Visual_Reference.png` (supersedes `Docs/Reference/GOTA_MainMenu_Concept.jpg`)

| Topic | Document |
|---|---|
| Architecture, screen model, travel, Fight AI, settings, sessions | `GOTA_UPG_FRONTEND_ARCHITECTURE.md` |
| Layout, tokens, states, motion, audio, backdrop status | `GOTA_UPG_MAIN_MENU_DESIGN.md` |
| Every asset created / edited | `GOTA_UPG_MENU_ASSET_INVENTORY.md` |
| What was tested, how, and what is still open | `GOTA_UPG_MENU_TEST_RESULTS.md` |

## Recovery

- Git tag `recovery/pre-upg-frontend` = HEAD before this work (`d988889`).
- `MainMenu.umap` and the legacy `WBP_MainMenu` / `WBP_LobbyMenu` are untouched.
- To roll back the boot target, set `GameDefaultMap` back to `/Game/MainMenu/MainMenu`.

## Rebuild everything

1. Offline art and audio (any CPython with Pillow, numpy, opencv-python-headless):
   - `python Tools/GOTA_BuildMenuArt.py Tools/MenuFonts`
   - `python Tools/GOTA_BuildMenuAudio.py`
2. Compile `WarOfGodsEditor`.
3. Run the editor builder:
   `set GOTA_QUIT_AFTER_BUILD=1` then
   `UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript="<abs>\Tools\GOTA_BuildMenuUI.py" -unattended -stdout`
   The result is written to `Saved/GOTA_BuildMenuUI.txt`.

## Scripted runtime validation (non-shipping builds)

```
UnrealEditor.exe WarOfGods.uproject -game -windowed -ResX=1920 -ResY=1080 -ExecCmds="gota.Test.Run Tools/Tests/menu_smoke.txt"
UnrealEditor.exe WarOfGods.uproject -game -windowed -ResX=1920 -ResY=1080 -ExecCmds="gota.Test.Run Tools/Tests/fight_ai.txt"
```

Screenshots are saved to `Saved/Screenshots/GOTA/`.
