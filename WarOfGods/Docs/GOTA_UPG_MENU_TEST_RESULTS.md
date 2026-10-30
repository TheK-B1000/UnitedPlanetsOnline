# UPG Menu — Test Results

**Date:** 2026-10-30
**Context:** Windows 11, UE 5.8, `WarOfGodsEditor` Development
**Runtime:** `UnrealEditor.exe -game` standalone (uncooked) at 1920×1080 windowed
**Method:** Scripted runs (`gota.Test.Run Tools/Tests/*.txt`). Real Slate key events (`gota.Test.Key`), log assertions (`gota.Menu.Dump`, `gota.Duel.Dump`), and screenshots in `Saved/Screenshots/GOTA/`.

## Status legend
Implemented · Compiled · Standalone-verified (uncooked `-game`) · PIE-verified · Packaged-verified · Not tested · Blocked

## Build

| Check | Result |
|---|---|
| `WarOfGodsEditor` Win64 Development | **Compiled** (0 errors) |
| 17 Widget Blueprints compile via builder | **PASS** (`Saved/GOTA_BuildMenuUI.txt`) |
| Packaged build | **Not tested.** Cook rules added; no package run. |

## Frontend (menu_smoke.txt)

| Test | Result |
|---|---|
| Game boots into `L_GOTA_MainMenu`, frontend presented once | **Standalone-verified** (`roots=1`, only `WBP_GOTA_FrontendRoot` in viewport) |
| Legacy `WBP_MainMenu` no longer spawned on top | **Standalone-verified** (inherited level script removed) |
| Arrow keys / D-pad move focus; Enter / (A) activates | **Standalone-verified** (Down×2 from Story → Multiplayer → screen=Multiplayer) |
| Esc, gamepad (B), Backspace go back; modal closes first | **Standalone-verified** |
| Continue disabled with "NO MISSION TO RESUME" when no valid save | **Standalone-verified** (no save on this machine). Resume-from-save **not tested.** |
| Story, Arena, Multiplayer, Settings, Extras screens render | **Standalone-verified** (screenshots) |
| Settings tabs via RB / LB | **Standalone-verified** |
| Quit → confirm modal, Esc cancels, focus restored | **Standalone-verified**. Actual quit in packaged build **not tested.** |
| Cards route to the same actions as nav rows | Implemented (shared `MenuAction`); card clicks **not individually exercised** |
| Mouse hover/click | **Not tested** by script (no synthetic mouse) |
| Resolutions other than 1920×1080 | **Not tested** (anchored layout, unverified) |

## Fight AI (fight_ai.txt, duel_inspect.txt)

| Test | Result |
|---|---|
| Arena → Enter the Arena (real UI) → busy modal → `DuelGameMode ?Opponent=AI` on Titan's Quarry | **Standalone-verified** |
| Warden + AI champion spawn (own `AIC_Enemy_Base` controller, bot PlayerState "Arena Champion") | **Standalone-verified** |
| Countdown locks fighters; AI brain paused, then resumed at FIGHT | **Standalone-verified** |
| Damage → death through the Blueprint `BPC_DamageSystem.TakeDamage` → round scored | **Standalone-verified** (both directions) |
| 90 s timeout with equal health → draw, no point | **Standalone-verified** |
| Best-of-three completes; results show VICTORY 2–1 / 2–0 with real score | **Standalone-verified** |
| Rematch via results UI (Enter) → fresh match at 0–0 | **Standalone-verified** |
| Return to Main Menu via results UI → frontend reset (`roots=1`, screen=MainMenu, no stale busy/modal) → Arena reachable again | **Standalone-verified** |
| Champion detects and approaches the Warden | **Standalone-verified** (Blackboard `State=Attacking`, `AttackTarget=Warden`; it closes to about 600 uu) |
| Champion lands attacks on a passive Warden | **Not achieved.** It holds near its defend radius; probably waiting on the melee attack-token protocol (`ReserveAttackToken` on the target's damage system). **Open.** |
| Human-driven combat (a person fighting) | **Not tested** — no interactive session |

## Bugs found and fixed during validation

1. **Graybox pivot (pre-existing, all arenas).** `ArenaGrayboxBuilder` placed the LevelPrototyping `SM_Cube` as if centre-pivoted. The Quarry floor covered one quadrant only, so the Warden fell through the world at start 0. Fixed by normalizing to mesh bounds. This **changes graybox geometry placement in every arena** (now as designed).
2. **Duel auto-start before BeginPlay.** A local host started the match in PostLogin, then BeginPlay reset it. Fixed.
3. **Menu state not reset after travel.** Widgets are outered to the GameInstance, so the "same world" check was wrong. Fixed with an explicit `PresentedWorld`.
4. **Settings revert baseline captured after edits.** Revert restored the new values. Fixed.
5. **Unsafe session reflection.** BP `JoinMpSession` was called with null params, and the BP `DestroySession` targeted the wrong session name. Disabled.
6. **Results screen had no gamepad/keyboard focus.** Fixed.
7. **Per-respawn `W_PlayerHUD` stacking from `BP_Player`.** Removed before each respawn (local player only).

## Navigation (affects Fight AI and Horde)

Runtime-generated arenas get no navmesh on graybox-only ground. ThirdPersonMap is World Partition; with Dynamic generation enabled, the build runs but adds no tiles for the runtime graybox components.

Mitigation: Fight AI uses arena starts that sit on the map's baked navmesh, and the champion enters the Warden's lane. **The root cause is open.** It needs an Editor investigation of navigation relevance for runtime-registered `UStaticMeshComponent`s in a WP map. Alternatively, bake the arena geometry into a dedicated level.

## Not done / blocked

| Item | Status |
|---|---|
| 3D UPG Earth orbital scene, Warden presentation, fleets, banners | **Not built.** No suitable assets exist; temporary clean-plate backdrop in use. |
| Horde from menu | **Disabled by data flag** (not validated end-to-end) |
| LAN host → join → lobby → match with 2 machines | **Not tested** |
| Multi-resolution / 21:9 / 4K review | **Not tested** |
| Performance capture | **Not measured.** With world rendering suppressed behind the backdrop the menu is UI-only; no profile taken. |
| First-time uncooked load of the arena map | About 8 minutes of shader/DDC work on first run; seconds afterwards. Uncooked-editor artifact only. |
