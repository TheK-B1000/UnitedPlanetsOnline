# Gods of the Arena — Technical Debt

**Date:** 2026-10-27  
**Engine:** UE 5.8 (upgraded from 5.3)

---

## Resolved in Phase 0

| Item | Resolution |
|------|------------|
| `.cpp` files under `Public/` | Moved to `Private/{Characters,Combat}` |
| `LockonComponent` at `Public/` root / empty `Combat` stub | Now `Public/Combat/LockonComponent.h` + Private cpp |
| Empty Lockon / Stats stubs | Implemented usable Lockon, Stats, Combat event hub |
| Engine stuck on 5.3 | `EngineAssociation` → `5.8`; Target `V7` + `Unreal5_8` |
| Stale 5.3 Intermediate/Binaries | Deleted before 5.8 rebuild |

---

## Open debt

### Architecture

| Debt | Impact | Severity | Suggested fix |
|------|--------|----------|---------------|
| Combat rules live in Blueprint (`BPC_Attacks`, `BP_Player`) | Hard to test/network-harden | High | Capture behavior → migrate incrementally into `UCombatComponent` |
| Dual damage paths (`BPC_DamageSystem` vs `UStatsComponent`) | Double-application risk if both used | High | Single authority path; BP wraps C++ or vice versa |
| Observer events not yet wired from BP | Event architecture unused | Medium | Call `Notify*` from existing attack/parry nodes |
| No GAS | Custom attributes/effects duplication risk | Medium | Evaluate after combat contracts stable |
| No Online Subsystem enabled | Sessions may use incomplete OSS | Medium | Choose Steam/EOS after lobby audit |
| Template ThirdPerson GameMode still default | Mode split unclear | Medium | Introduce mode-specific GMs sharing combat |
| Project/repo names diverge (`UnitedPlanetsOnline` / `WarOfGods` / GOTA) | Confusion only | Low | Rename later when branding locked |

### Content / assets

| Debt | Impact | Severity | Suggested fix |
|------|--------|----------|---------------|
| Missing InfinityBlade / FXVariety particle refs | Montage warnings, missing trails | Medium | Retarget to existing FX or reimport packs |
| Missing `/Engine/TemplateResources/MI_Template_Red_Metal` | `SM_Platform` material break | Low | Assign local material |
| No dodge / dedicated parry animations by name | Vertical slice incomplete | High | Acquire or placeholder montages |
| Marketplace packs mixed at Content root | Clutter, hard budgets | Low | Defer move; document as vendor roots |
| `Attacks` + `DamageSystem` at Content root | Desired future `Content/Combat/` | Low | Move with redirects after combat freeze |
| World Partition streaming logs under Saved | Noise | Low | Ignore / clean Saved (gitignored) |

### Networking

| Debt | Impact | Severity | Suggested fix |
|------|--------|----------|---------------|
| Authority of damage/parry Unverified | Exploits / desync | High | Server validate hits & defensive windows |
| Animation replication “messing around” history | Fragile montages | Medium | Replicate combat state, not ad-hoc anim RPCs |
| No dedicated server target | Co-op scale limits | Medium | Add server target when ready |
| 5-player never profiled | Perf unknown | Medium | PIE multi-client after combat MP |

### Process

| Debt | Impact | Severity | Suggested fix |
|------|--------|----------|---------------|
| No Docs historically | Knowledge in git messages only | Medium | Keep Docs/ updated per milestone |
| Blueprint graphs not dumped this pass | Audit incomplete | High | Editor capture next session |
| Crashes under Saved/Crashes | Past instability | Medium | Triage after 5.8 open |

---

## Upgrade notes (5.3 → 5.8)

- Binary engine at `K:\Program Files\Epic Games\UE_5.8` (changelist 55116800).
- First editor open should run fixup redirects and resave assets.
- Do **not** assume Iris / Replication Graph / Mass AI are in use — none enabled.
- Plugin set remains minimal (`ModelingToolsEditorMode`, `EnhancedInput`).
- Expect possible deprecation warnings in Blueprints using old nodes; fix opportunistically.

---

## Non-goals until debt above is addressed

- Mass Content folder moves  
- Full GAS adoption  
- Story / PvE / PvP mode scaffolding  
- Deleting marketplace packs  
- Paid backend  
