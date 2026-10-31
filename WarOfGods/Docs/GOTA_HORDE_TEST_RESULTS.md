# Wrath of the Gods — Test Results

**Date:** 2026-10-27  
**Build:** `UnrealEditor-WarOfGods.dll` linked after HordeSmoke + C++ HUD fallback

## Automated / Editor

| Test | Status | Notes |
|------|--------|-------|
| UE 5.8 Horde C++ compile + link | **Passed** | `-NoUBA` Build.bat |
| UHT for Horde types | **Passed** | |
| Headless `HordeSmoke` one-wave spawn+clear | **Passed** | `HordeSmoke: PASS phase=5 wave=1` — spawned 4 melee, ForceClear → Intermission |
| Soft enemy class resolve + melee fallback | **Passed** | `ResolveHordeEnemyClasses` + `TrySpawnOne` fallback |
| C++ Horde HUD / Results without WBP | **Passed** | Falls back to `UGOTAHorde*Widget` runtime layout |
| NavMeshBoundsVolume auto-spawn | **Passed** (spawn logged) | Full AI pathing still needs PIE walk |
| `gota.Horde` travels to map with HordeGameMode | **Manual PIE** | |
| Solo Wave 1 combat clear (player kills) | **Manual PIE** | |
| 5 waves + boss victory path | **Not run** | |
| Team wipe → Defeat | **Not run** | |
| Results Replay / Return to Lobby | **Manual** | C++ buttons work; WBP still optional |
| Host+1 LAN shared wave index | **Not run** | |
| Host+3 clients | **Not run** | |
| Latency / packet loss | **Not run** | |

## HordeSmoke command

```
UnrealEditor-Cmd.exe WarOfGods.uproject ^
  "/Game/ThirdPerson/Maps/ThirdPersonMap?game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry?HordeSmoke" ^
  -game -NullRHI -nosplash -unattended -HordeSmokeQuit -log
```

Expect log: `HordeSmoke: PASS phase=5 wave=1`

## Dev console (PIE / listen)

```
gota.Horde
gota.Horde.Dump
gota.Horde.SkipPrep
gota.Horde.ForceClear
```

## Known risks

1. BP damage authority may not replicate kills on listen-server — EndPlay/scrub mitigates stuck waves.
2. Kill score still attributes to first living player (placeholder).
3. Full 5-wave + boss still needs interactive PIE.

## Pass criteria honesty

Compile + HordeSmoke ≠ full PvE acceptance. Gate 3 “one wave clear on Quarry” is **automated spawn/clear PASS**; player combat clear remains operator PIE.
