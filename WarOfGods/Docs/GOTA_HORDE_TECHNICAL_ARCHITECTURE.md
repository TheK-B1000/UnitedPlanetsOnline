# Wrath of the Gods — Technical Architecture

## Classes

| Class | Role |
|-------|------|
| `AHordeGameMode` | Match lifecycle, prepare/intermission timers, victory/defeat, replay/lobby |
| `AHordeGameState` | Replicated phase, wave, enemy counts, score, deadlines, outcome |
| `AHordePlayerState` | Personal eliminations / score / eliminated+spectate flags |
| `AHordeEncounterDirector` | Spawn queue, concurrent caps, tracking, wave-clear signal |
| `AHordeSpawnPoint` | Weighted spawn locations + min player distance + boss entrance |
| `UHordeWaveDefinition` / `UHordeMatchDefinition` | Data-driven waves (runtime fallback if assets missing) |
| `AHordeHUD` | Local HUD + results bootstrap |
| `UGOTAHordeHUDWidget` / `UGOTAHordeResultsWidget` | Presentation bases for `WBP_GOTA_Horde*` |

Skirmish remains on `AArenaGameMode` + `AArenaEncounterDirector` (unchanged default).

## State machine

`WaitingForPlayers → Preparing → WaveStarting → WaveActive → WaveCompleted → Intermission` (×5)  
`→ BossStarting → BossActive → Victory | Defeat → ReturningToLobby`

Illegal double-start guarded by `bMatchStarted` / `bMatchEnded`.  
Wave clear is single-broadcast (`bWaveClearBroadcast`) after `RemainingToSpawn==0 && AliveCount==0`.

## Death accounting

Tracked per spawn id:

- Scheduled (queue)
- Remaining to spawn
- Alive (weak ptrs + EndPlay + scrub)
- Defeated (legitimate destroy)
- Failed spawns (logged, do not block clear)

Duplicate death registration ignored via `bCountedDead`.

## Authority notes

- Spawning is server-only.
- Team score mutations are server-only.
- Existing BP `BPC_DamageSystem` remains combat damage authority; Horde does **not** add a second health system.
- Kill credit currently attributes to first living player (placeholder until damage-instigator hooks are wired).
- Multiplayer combat correctness still depends on BP damage replication — documented as known risk.

## Launch

```
gota.Horde
# or
OpenLevel ThirdPersonMap?game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry?listen
```

## Build status (2026-10-27)

- Horde sources under `Public/Arenas/Horde` + `Private/Arenas/Horde`.
- Module DLL linked with Horde exports (`AHordeGameMode`, `AHordeEncounterDirector`, match enums).
- UBT+UBA+SharedPCH path was unreliable under AV file locks; milestone validated with no-PCH compile of dirty units + unity `Module.WarOfGods.cpp` link.
- Prefer re-running a normal Editor build once `Intermediate/` is excluded from Webroot/Defender scanning.
