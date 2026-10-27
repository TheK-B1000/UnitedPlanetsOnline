# Asset Inventory & Cleanup — Arena Biomes

**Date:** 2026-10-27  
**Engine:** UE 5.8  
**Disk:** Project on **B:** (~744 GB free). Epic Vault Cache still on **K:** (~1.7 GB free) — move cache to B ASAP.

---

## What you have (approximate)

| Pack / folder | Size | Role | Decision |
|---------------|------|------|----------|
| `Megascans/3D_Assets` | **~27 GB** | Huge rock/cliff/hoodoo kits | **Keep on disk, do not commit yet.** Curate per arena. |
| `Megascans/Rock, Surfaces, ErodedGround, 3D_Plants` | ~0.8 GB | Dressing | **Keep** — Canyon / Ruins / Forest accents |
| `EuropeanBeech` | **~7 GB** | Forest foliage kit | **Keep on disk** for Forest arena; defer git until used |
| `Geometry/Custom` | **~1.2 GB** | Large custom meshes | Review; likely keep for Canyon |
| `Geometry/Succulents` + Quixel BPs | small | Desert/oasis dressing | **Keep** |
| `FightingAnimsetPro/Animations` | ~107 MB | Combat anim library | **Keep + commit** |
| `FightingAnimsetPro/SourceFiles` | was 546 MB | Raw FBX | **Deleted** (anims already imported) |
| `BossyEnemy` | ~32 MB | Boss mesh + parry/stagger anims | **Keep + commit** |
| `MSPresets` | ~29 MB | Megascans materials | **Keep** (needed by Megascans) |
| LavaField maps | small | Volcanic zoo/example | **Keep** for Volcanic arena |
| Paragon / InfinityBlade / FXVariety | existing | VFX | **Keep** (fix broken soft refs later) |
| StarterContent / LevelPrototyping | existing | Blockout | **Keep** for Neutral arena |

---

## Arena biome plan (variety)

Use existing packs; do **not** buy more until these five play.

| Arena ID | Biome fantasy | Primary assets | Status |
|----------|---------------|----------------|--------|
| `Arena_Neutral` | Stone pit / training ground | ThirdPerson arena floor/walls, LevelPrototyping, StarterContent | Scaffold folder ready |
| `Arena_Forest` | Sacred grove | EuropeanBeech + Megascans plants | Folder ready; dress in editor |
| `Arena_Volcanic` | Ash & lava | LavaFieldCollections maps/materials + FXVariety/Paragon fire FX | Folder ready |
| `Arena_Canyon` | Desert cliffs / hoodoo | Megascans Rock, Cliffs, Hoodoo, Geometry Custom, Succulents | Folder ready |
| `Arena_Ruins` | Broken temple | Pedestals/Butte packs + existing Props + InfinityBlade FX | Folder ready |

Content paths created:

```
Content/Arenas/Neutral/
Content/Arenas/Forest/
Content/Arenas/Volcanic/
Content/Arenas/Canyon/
Content/Arenas/Ruins/
```

Maps themselves are authored in-editor (Phase 1 does not mass-resave or invent umaps blindly).

---

## Git policy (stops the 1000+ change flood)

**Commit now (combat-critical, manageable size):**

- `Content/BossyEnemy/`
- `Content/FightingAnimsetPro/` (without SourceFiles)
- Quixel scatter blueprints if used for Forest/Canyon dressing

**Ignore until curated (still on B: for building arenas):**

- `Content/Megascans/`
- `Content/EuropeanBeech/`
- `Content/Maps/Assembly/`
- Megascans/Assembly zoo maps

**Reason:** Committing 30+ GB of Megascans will crush git and backups. Dress arenas from local content; commit only the final arena maps + the specific meshes they reference (or use Git LFS later).

---

## Vault Cache (K: full)

Launcher setting is still `VaultCacheDirectories=K:/Epic/VaultCache/` (~65 GB).

1. Quit Epic Games Launcher completely.
2. Set Vault Cache to `B:/Epic/VaultCache`.
3. Optionally move/copy existing `K:/Epic/VaultCache` → `B:/Epic/VaultCache`, then delete K copy after verifying installs.

Project Content already lands on **B** when you add to the open WarOfGods project; only the **download cache** was on K.

---

## Do not delete without a second confirmation

- Entire `Megascans/3D_Assets` (27 GB) — useful for Canyon cliffs; prune unused assets after first Canyon arena exists.
- `EuropeanBeech` — Forest arena dependency.
