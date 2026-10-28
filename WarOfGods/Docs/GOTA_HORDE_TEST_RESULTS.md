# Wrath of the Gods — Test Results

**Date:** 2026-10-27  
**Build:** `UnrealEditor-WarOfGods.dll` linked 2026-10-27 20:52 (Horde symbols verified via dumpbin)

## Automated / Editor

| Test | Status | Notes |
|------|--------|-------|
| UE 5.8 Horde C++ compile + link | **Passed** | UBA/SharedPCH hung under Webroot; validated via no-PCH compile of dirty units + link. `AHordeGameMode`, director, GS/PS exports present (1366 module exports). |
| UHT for Horde types | **Passed** | `EHordeMatchPhase`, wave/match DAs, spawn point reflected |
| `gota.Horde` travels to map with HordeGameMode | **Not run** | Manual PIE required after Editor reload |
| Solo Wave 1 spawn using BP_Enemy_Melee | **Not run** | Requires PIE + enemy class soft-path load |
| Wave clear advances | **Not run** | |
| 5 waves + boss victory path | **Not run** | |
| Team wipe → Defeat | **Not run** | |
| Results Replay / Return to Lobby | **Not run** | `WBP_GOTA_HordeHUD` / `WBP_GOTA_HordeResults` not authored yet |
| Host+1 LAN shared wave index | **Not run** | Must use multi-client PIE |
| Host+3 clients | **Not run** | |
| Latency / packet loss | **Not run** | |
| NavMesh reachability on graybox | **Not run** | Validate before raising spawn budgets |

## Build notes

- Standard `Build.bat` / UBA frequently stalled on the ~2.3GB SharedPCH while Webroot (`WRSkyClient`) held exclusive locks with parallel `cl.exe`.
- Workaround used for this milestone: no-PCH `cl` of adaptive-excluded units + `Module.WarOfGods.cpp` unity obj, then link `UnrealEditor-WarOfGods.dll`.
- Compile fixes applied for: `AActor::Role` parameter shadowing, missing `Engine/World.h` in graybox, ArenaSelection header/cpp name mismatch, UHT-unsafe includes of `TimerHandle.generated.h` / heavy camera headers before local `.generated.h`.

## Known blockers / risks

1. **WBP_GOTA_HordeHUD / Results** not authored — C++ soft class paths will no-op until WBPs exist.
2. **BP damage authority** may not replicate kills correctly on listen-server — wave clear uses actor destroy/EndPlay; ragdoll-without-Destroy can stall waves (scrub timer mitigates).
3. **Boss BP** `/Game/Enemies/BossEnemy/BP_Enemy_Boss` — verify loads; runtime wave def falls back to melee if soft class fails at spawn time.
4. **Kill score attribution** currently credits first living player (placeholder until damage-instigator hooks).
5. Full UBT-with-PCH Editor rebuild should be re-run once antivirus exclusions cover `Intermediate/` (or Editor Live Coding mutex is clear).

## Pass criteria honesty

Do **not** mark multiplayer or wave gameplay complete until host+clients are actually tested in PIE/standalone. Compile/link success ≠ acceptance criteria 1–12.
