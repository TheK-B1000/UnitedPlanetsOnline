# Arena Graybox — Play Now (4 players)

Default GameMode is now **`ArenaGameMode`** on `ThirdPersonMap`.

## PIE

1. Open project in UE 5.8 → `ThirdPersonMap` (startup map).
2. Play.
3. You should spawn in **Titan's Quarry** graybox with `BP_Player` (if it loads).
4. Melee enemies wave-spawn when `BP_Enemy_Melee` resolves.

### 4-player

Play → Net Mode: **Listen Server** → Number of Players: **4**.

### Switch to Shattered Dominion (3 biomes)

In console (`~`):

```
gota.BuildArena Dominion
```

Or travel with:

```
open ThirdPersonMap?Arena=Dominion
```

Back to Quarry:

```
gota.BuildArena Quarry
```

### Regions (Dominion)

| Direction | Biome |
|-----------|--------|
| Center | Pantheon Coliseum |
| North | Stone Ruins |
| East | Forest (European Beech meshes if pack present, else green pillars) |
| West | Volcanic + lava marker |

Watch Output Log for `BiomeEnter`.

## Notes

- Main Menu map still uses its own GameMode when you open `MainMenu`.
- Art dress (LavaField / Moab / Iceland) comes after graybox combat feels good.
