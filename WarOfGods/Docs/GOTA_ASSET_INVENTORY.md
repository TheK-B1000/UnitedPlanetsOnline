# Gods of the Arena — Asset Inventory

**Date:** 2026-10-27  
**Engine:** UE 5.8  
**Method:** Filename / folder inventory (`.uasset` binary contents not opened). Editor Asset Registry pass still recommended.

---

## Target Content map (not applied yet)

Keep current paths until combat migration freezes. Desired eventual layout:

```
Content/
  Characters/     # present
  Enemies/        # present
  Combat/         # future: Attacks + DamageSystem
  Weapons/        # present
  Framework/      # present
  UI/             # future: Widgets + HUD
  Maps/           # present (level table only)
  ThirdPerson/    # keep until player BP migration done
  Animations/ Audio/ VFX packs as vendor roots
```

---

## Characters & player

| Asset | Path | Role | Notes |
|-------|------|------|-------|
| `BP_Player` | `Content/Player/` | Hero pawn | Primary vertical-slice character |
| `E_PlayerStance` | `Content/Player/` | Stance enum | |
| `ABP_Player` / Manny / Quinn | `Content/Characters/Mannequins/` | Anim BPs | |
| Mage mesh + anims | `Content/Characters/Mage/` | Mage enemy/player art | Kachujin-derived textures |

---

## Combat Blueprints

| Asset | Path | Role |
|-------|------|------|
| `BPC_Attacks` | `Content/Attacks/` | Attack component (source of truth) |
| `S_AttackInfo` | `Content/Attacks/` | Attack data struct |
| `BPC_DamageSystem` | `Content/DamageSystem/` | Damage processing |
| `BPI_Damagable` | `Content/DamageSystem/` | Damage interface |
| `E_DamageType` / `E_DamageResponse` / `S_DamageInfo` | `Content/DamageSystem/` | Damage taxonomy |
| `W_HealthBar` | `Content/DamageSystem/` | World health widget |

---

## Weapons

| Asset | Role | Vertical slice candidate |
|-------|------|--------------------------|
| `BP_Weapon_Base` | Base weapon | — |
| `BP_Weapon_Sword` | Melee | **Primary** |
| `BP_Weapon_Axe` | Melee + throw/smash set | Strong alt (richer anims) |
| `BP_Weapon_Shield` | Block | Use with sword |
| `BP_Weapon_Rifle` | Ranged | Defer for melee slice |

Meshes under `Weapons/Meshes/{Axe,Rifle,Shield}`.

---

## Animations

### Present (usable)

- **Axe:** idle, locomotion BS, combo 1/2, block success/react, ground smash, jump attack, throw, hit reaction, battlecry (~27 assets).
- **Melee/sword+shield:** block, block react, combo, slash, spinning, ground smash, stagger, sheath/unsheath, jump attack (~38 files under `Animations/Melee`).
- **Magic:** locomotion + hadouken montage.
- **Mage character:** cast barrage, fireball, heal, ground smash, hit react.

### Missing / not found by name

| Need | Status | Acquisition note |
|------|--------|------------------|
| Dodge / roll / sidestep | **Missing** | Required for perfect dodge slice |
| Dedicated parry pose / contact | **Missing** (block exists) | May retarget block-success or author parry montage |
| Perfect-parry exclusive anim | **Missing** | |
| Execution / finishers | **Missing** | Phase 6+ |
| Divine ability anims | **Missing** | Beyond hadouken/AOE |

---

## Enemies & AI

| Type | Assets | Notes |
|------|--------|-------|
| Base | `BP_Enemy_Base`, `AIC_Enemy_Base`, `BT_Enemy_Base`, `BB_Enemy_Base` | |
| Melee | `BP_Enemy_Melee`, BT/tasks, `E_Melee_Attacks`, `E_BlockingState` | **Vertical-slice enemy** |
| Mage | `BP_Enemy_Mage`, heal/teleport/attack tasks | |
| Ranged | `BP_Enemy_Ranged`, EQS ranged location | |
| Boss | `BP_Enemy_Boss`, float/teleport/attack, `E_Boss_Attacks` | Prototype |
| Shared AI | EQS (strafe, cover, seek, teleport), decorators, subtrees (passive/seeking/frozen) | |

---

## Projectiles & AOE

- Projectiles: Arrow, DarkEnergy (+alt), Ice, SpinningAxe, Base.
- AOE: Base, Blast, Heal, LightningStrike, OverTime + decals.

Useful for divine counter VFX placeholders.

---

## Framework / UI / maps

| Asset | Role |
|-------|------|
| `BP_WarOfGods_GameInstance` | Session / persistent |
| `BP_WarOfGods_PlayerController` | Input / UI |
| `BP_WarOfGods_PlayerState` | Spawn / score |
| `BP_BaseSetup_GameState` | Shared state |
| `BP_WarOfGodsMenuController` | Menu |
| `MainMenu.umap` + lobby widgets | Front end |
| `LevelInfo_Table` / `S_LevelInfo` | Level select data |
| `ThirdPersonMap` | Default play map + generated Arena_Floor/Walls |
| `W_PlayerHUD`, `W_BossHealthBar` | Combat HUD |

---

## Marketplace / vendor packs

| Pack | Path | Keep? |
|------|------|-------|
| StarterContent | `Content/StarterContent` | Yes (prototype) |
| FXVarietyPack | `Content/FXVarietyPack` | Yes — **incomplete refs** |
| InfinityBladeEffects | `Content/InfinityBladeEffects` | Yes — **incomplete refs** |
| ParagonFengMao / Gideon / Sparrow FX | `Content/Paragon*` | Yes (FX only folders) |
| LevelPrototyping | `Content/LevelPrototyping` | Yes |

**Do not delete** without approval.

### Broken soft references (from `Saved/Logs/WarOfGods.log`)

| Missing package | Referenced by |
|-----------------|---------------|
| `/Game/InfinityBladeEffects/Effects/FX_Combat_Lightning/P_DOT_Lightning_01_RED` | `Montage_Axe_Battlecry` |
| `/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Ausar/P_AU_Sword_Proj_Trail_01_RED` | `Montage_Axe_Combo_1` |
| `/Game/FXVarietyPack/Particles/P_ky_lightning3_RED` | `Montage_Axe_Combo_2` |
| `/Game/FXVarietyPack/Particles/P_ky_magicCircle1` | `Montage_Axe_GroundSmash` |
| `/Engine/TemplateResources/MI_Template_Red_Metal` | `Props/Meshes/SM_Platform` |

---

## Persistence

- `Content/SavedData/SG_PlayerProfile`, `S_PlayerProfile`

---

## Asset gap priorities for vertical slice

1. Dodge animation set (or temporary mannequin roll).
2. Parry contact feedback (anim + Niagara + audio) — can start from block-success.
3. Repair or replace broken axe montage FX refs.
4. One clear lock-on reticle / target indicator (optional UI).
5. Divine counter VFX — reuse LightningStrike AOE as placeholder.
