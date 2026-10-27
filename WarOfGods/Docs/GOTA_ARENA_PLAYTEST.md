# Arena Graybox — How to Play (4 players)

## Quick PIE (Titan's Quarry)

1. Open `ThirdPersonMap` (or any open level) in UE 5.8.
2. **World Settings → GameMode Override** → `ArenaGameMode`.
3. Set on the GameMode (Details, or subclass BP later):
   - `GrayboxLayout` = `TitansQuarry` (default) or `ShatteredDominionThreeBiome`
   - `SoftMaxPlayers` = 4
   - `bAutoBuildGraybox` = true
4. Play. Graybox geometry + 4 `ArenaPlayerStart`s spawn at runtime.
5. Assign `EncounterDirector → EnemyClass` = `BP_Enemy_Melee` when ready (otherwise waves log only).

### Multiplayer PIE

- Play → Net Mode: **Play As Listen Server**, Number of Players: **4**.
- Confirm each client sees the same stone/forest/volcano placeholders (local deterministic build).

### Shattered Dominion 3-biome stub

Set `GrayboxLayout` = `ShatteredDominionThreeBiome`:

- Center: Pantheon Coliseum  
- North: Stone Ruins (+ bridge)  
- East: Forest placeholders  
- West: Volcanic + lava marker  

Walk the bridges between regions; watch Output Log for `BiomeEnter`.

## Placeholders

Missing Iceland / Nordic / full Megascans dress → colored boxes and pillars until packs are curated. Combat uses existing BP systems.
