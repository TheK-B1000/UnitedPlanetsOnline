# Arena Graybox — Play Now (4 players)

Default GameMode is **`ArenaGameMode`** on `ThirdPersonMap`.

## PIE

1. Open project in UE 5.8 → `ThirdPersonMap` (startup map).
2. Play — default graybox is **Titan's Quarry**.
3. Melee waves spawn when `BP_Enemy_Melee` resolves.

### 4-player

Play → Net Mode: **Listen Server** → Number of Players: **4**.

## Switch arenas (console `~`)

```
gota.BuildArena Quarry
gota.BuildArena Ashfall
gota.BuildArena Frostbound
gota.BuildArena Verdant
gota.BuildArena Sunscar
gota.BuildArena Stormbreak
gota.BuildArena Dominion
gota.BuildArena Story
```

Or travel:

```
open ThirdPersonMap?Arena=Quarry
open ThirdPersonMap?Arena=Ashfall
open ThirdPersonMap?Arena=Frostbound
open ThirdPersonMap?Arena=Verdant
open ThirdPersonMap?Arena=Sunscar
open ThirdPersonMap?Arena=Stormbreak
open ThirdPersonMap?Arena=Dominion
open ThirdPersonMap?Arena=Story?Mode=Story
```

**Story Mode** spawns in **The Fallen Sanctuary** (camp SW of courtyard), not Titan's Quarry. See `GOTA_STORY_PROLOGUE.md`.

## Standalone arenas (prototype graybox)

| Arena | Feel | Signature |
|-------|------|-----------|
| Titan's Quarry | Stone pit | Center pad, ring pillars, 4 corner spawns |
| Ashfall Crucible | Volcanic | Lava glow, cross bridges, corner islands |
| Frostbound Citadel | Ice keep | Gate walls, courtyard, ramparts |
| Verdant Sanctuary | Forest temple | Tree rings (Beech/Maple if present), flanking alleys |
| Sunscar Expanse | Desert | Sand floor, canyon corridors, dune pads |
| Stormbreak Coast | Cliff coast | Water drop south, towers, choke walls |

## Shattered Dominion (6 biomes)

| Direction | Biome |
|-----------|--------|
| Center | Pantheon Coliseum |
| North | Stone Ruins |
| East | Forest |
| West | Volcanic |
| South | Coast |
| Northeast | Frozen |
| Northwest | Desert |

Watch Output Log for `BiomeEnter` / `ArenaGrayboxBuilder built`.

## Notes

- Main Menu map still uses its own GameMode when you open `MainMenu`.
- Art dress (LavaField / Moab / Iceland) comes after graybox combat feels good.
