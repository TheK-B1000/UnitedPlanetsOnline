# Gods of the Arena — Project Audit

**Date:** 2026-10-27  
**Branch:** `cpp-branch`  
**Project path:** `WarOfGods/WarOfGods.uproject`  
**Engine:** Unreal Engine **5.8.0** (upgraded from 5.3)  
**Install used:** `K:\Program Files\Epic Games\UE_5.8`  
**Module:** `WarOfGods` (single runtime module)  
**Inspection method:** Static source/config/asset-name inventory + UE 5.8 editor module compile. Blueprint graph behavior marked **Unverified (static only)** unless noted.

---

## Baseline verification

| Check | Result |
|-------|--------|
| Engine association | `5.8` in `WarOfGods.uproject` |
| Target build settings | `BuildSettingsVersion.V7`, `IncludeOrderVersion.Unreal5_8` |
| `WarOfGodsEditor` Win64 Development | **Succeeded** (UE 5.8 UBT, ~91s) |
| Editor launch / PIE | Not executed this pass |
| Listen-server / 5-player | Not executed — lobby assets exist; runtime unproven |
| Content resave on 5.8 | Pending first editor open (expect redirect/resave prompts) |

---

## System inventory

Status key: **Working** | **Partial** | **Broken** | **Missing** | **Unverified**

### Foundation

| System | Status | Paths | Notes | MP readiness | Priority |
|--------|--------|-------|-------|--------------|----------|
| Engine / project | Working | `WarOfGods.uproject` | Upgraded 5.3 → 5.8; compiles | N/A | — |
| C++ module layout | Working | `Source/WarOfGods/Public|Private/{Characters,Combat,Events}` | Flat domain folders; no deep nesting | N/A | — |
| Startup map | Partial | `/Game/ThirdPerson/Maps/ThirdPersonMap` | Template map + generated arena floor/walls | Local only | P2 |
| Default GameMode | Partial | `BP_ThirdPersonGameMode` | Still ThirdPerson template GM | Unverified | P1 |
| GameInstance | Partial | `Content/Framework/BP_WarOfGods_GameInstance` | Exists; session logic Unverified | Partial (lobby commits) | P1 |

### Combat

| System | Status | Paths | Notes | MP readiness | Priority |
|--------|--------|-------|-------|--------------|----------|
| Attacks / combos | Partial | `Content/Attacks/BPC_Attacks`, `S_AttackInfo` | Git: combos, jump/ground smash; **BP graphs Unverified** | Replication commits exist | P0 |
| Damage | Partial | `Content/DamageSystem/*` | `BPC_DamageSystem`, `BPI_Damagable`, damage enums/structs | Unverified authority | P0 |
| Block / parry | Partial | Implied in `BP_Player`, `BPC_Attacks`; enemy `BTT_MeleeBlock`, `E_BlockingState` | Git: `6f13cb9`, `1398f9f`; **no dedicated parry anim assets by name** | Unverified lag fairness | P0 |
| Dodge / perfect dodge | Missing | — | No dodge/roll assets found by name | Missing | P0 |
| Lock-on | Partial | `ULockonComponent` (C++), `IA_Lockon` | Real candidate select + soft follow implemented Phase 0 | Local presentation | P1 |
| Combat events (C++) | Partial | `UCombatComponent`, `Events/CombatEvents.h` | Observer hub; BP must call Notify* until migration | Local events ≠ replication | P1 |
| Stats (C++) | Partial | `UStatsComponent` | Health / Stamina / DivineMomentum + RepNotify hooks | Replicated props ready; not wired to BP damage yet | P1 |
| Weapons | Partial | `BP_Weapon_{Base,Sword,Axe,Shield,Rifle}` | Sword/axe primary for vertical slice | Unverified | P1 |
| Divine counter / momentum | Partial | `UStatsComponent` DivineMomentum | Design hook only; no ability yet | Missing | P0 |

### Movement / input

| System | Status | Paths | Notes |
|--------|--------|-------|-------|
| Enhanced Input | Working | `Content/ThirdPerson/Input/*` | Move, Look, Jump, Lockon |
| Character movement | Partial | `BP_Player`, ThirdPerson character | Template + custom stances (`E_PlayerStance`) Unverified |

### AI / enemies

| System | Status | Paths | Notes |
|--------|--------|-------|-------|
| Enemy base + types | Partial | `Enemies/BP_Enemy_*`, Boss/Mage/Melee | Substantial BT/EQS content |
| Behavior Trees / EQS | Partial | `Enemies/AI/**` | Perception, strafe, cover, boss teleport tasks |
| Group combat director | Missing | — | No attack-token / director asset |
| Boss | Partial | `BP_Enemy_Boss`, `BT_Enemy_Boss`, HUD bar | Single boss prototype |

### Multiplayer / sessions

| System | Status | Paths | Notes |
|--------|--------|-------|-------|
| Lobby / sessions | Partial | `MainMenu/*`, `Framework/*` | Commits: Create/Join Session, Lobby Menu, PlayerState spawn |
| OnlineSubsystem | Missing | Build.cs previously commented | Not enabled for 5.8 yet |
| Server-authoritative combat | Partial | Replication commits for magic/melee/anim | Authority boundaries Unverified |
| Dedicated server target | Missing | Only Game + Editor targets | — |

### Game modes

| Mode | Status | Notes |
|------|--------|-------|
| Story campaign | Missing | No quest system; MainMenu level table only |
| PvE arena waves | Missing | Arena meshes under ThirdPerson map only |
| PvP duel | Missing | No match lifecycle |

### UI / audio / VFX

| System | Status | Paths | Notes |
|--------|--------|-------|-------|
| Player HUD | Partial | `Widgets/W_PlayerHUD`, boss bar | Unverified |
| Main menu / lobby UI | Partial | `WBP_MainMenu`, `WBP_LobbyMenu`, `WBP_ServerRow` | |
| Combat SFX | Partial | e.g. `Audio/axe-block` | Sparse |
| Niagara / cascade FX | Broken refs | See asset inventory | Missing IB/FXVariety particles |

### Persistence

| System | Status | Paths | Notes |
|--------|--------|-------|-------|
| SaveGame profile | Partial | `SavedData/SG_PlayerProfile`, `S_PlayerProfile` | Local only |

---

## C++ surface (post Phase 0)

```
Source/WarOfGods/
  WarOfGods.Build.cs / .h / .cpp
  Public/
    Characters/StatsComponent.h
    Combat/CombatComponent.h
    Combat/LockonComponent.h
    Events/CombatEvents.h
  Private/
    Characters/StatsComponent.cpp
    Combat/CombatComponent.cpp
    Combat/LockonComponent.cpp
    Events/
```

---

## Highest risks

1. **5.3 → 5.8 content migration** — first editor open may need asset resaves; test packs carefully.
2. **Blueprint combat is the real game** — C++ shells must not replace BP until behavior is captured.
3. **Broken FX soft-references** — montages reference missing particles.
4. **No dodge content** — vertical slice needs dodge anims or temporary placeholders.
5. **Multiplayer unproven on 5.8** — retest lobby + combat replication after upgrade.

---

## Blueprint behavior capture (Phase 0)

**Status: Unverified (static only)** — UE editor graph inspection was not available during this pass. Inferences below come from asset names + git history only. Revisit in Phase 1 with the 5.8 editor open.

| Blueprint | Inferred behavior (unverified) | Evidence |
|-----------|--------------------------------|----------|
| `BP_Player` | Stances; block, parry, combo; owns combat input | `E_PlayerStance`; commit `6f13cb9` |
| `BPC_Attacks` + `S_AttackInfo` | Data-driven attacks; combos; interrupt rules | `fc3df1c`, `b7713ab` |
| `BPC_DamageSystem` + `BPI_Damagable` | Shared damage interface for player/enemies | Asset set under `DamageSystem/` |
| `BP_Weapon_*` | Sword/axe/shield/rifle actors from `BP_Weapon_Base` | Content listing |
| Framework GI/PC/PS | Lobby, spawn positions, PostLogin / controller swap | `89cb98c`, `d636e85`, `3865d33` |
| Enemy melee / boss | Block task; boss stagger on *some* parries; teleport | `278176e`, `1398f9f`, `2a99140` |
| Magic/melee replication | Anim/combat replication iterated | `65e9fd7`, `24b5169`, `a1b4125` |

Runtime confirmation of parry windows, damage authority, and session flow is **not** claimed.

---

## Recommended next task

Open project in UE 5.8 editor, resave critical Blueprints, document `BPC_Attacks` / `BP_Player` block-parry-combo flow, then wire `UCombatComponent` Notify calls from those Blueprints without moving rules into C++ yet.
