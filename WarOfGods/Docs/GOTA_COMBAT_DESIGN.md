# Gods of the Arena — Combat Design Spec

**Date:** 2026-10-27  
**Engine:** UE 5.8  
**Status:** Phase 1 — contracts from existing BP behavior + target timing (tunable, not final)

---

## Ownership (Phase 1)

```mermaid
flowchart TB
  Input[Player Input] --> BPPlayer[BP_Player]
  BPPlayer --> BPCAttacks[BPC_Attacks]
  BPCAttacks --> Weapons[BP_Weapon_*]
  BPCAttacks --> DamageBP[BPC_DamageSystem]
  DamageBP --> HealthBP[BP Health Authority]
  BPCAttacks -->|"Notify* adapters"| CombatCxx[UCombatComponent Events]
  HealthBP -->|"ObserveHealth optional"| StatsCxx[UStatsComponent]
  StatsCxx -->|DivineMomentum only| Divine[Divine resource]
  Lockon[ULockonComponent] --> Camera[Soft look / target]
```

| Value | Authority now | Observer |
|-------|---------------|----------|
| Health / death | `BPC_DamageSystem` / BP | `UStatsComponent::ObserveHealth` (optional) |
| Attack outcomes | `BPC_Attacks` | `UCombatComponent` Notify* |
| Block / parry | `BP_Player` + `BPC_Attacks` | NotifyBlock / NotifyParry |
| Stamina | Unverified in BP | C++ ready; gated by `bAuthoritativeMutations` |
| DivineMomentum | C++ `UStatsComponent` | HUD / abilities later |
| Lock-on target | `ULockonComponent` | Camera / combat orient |

**Rule:** No duplicate health writers. C++ Stats mutations stay **off** (`bAuthoritativeMutations=false`) until migration.

---

## Attacks

| Phase | Meaning | Source today | Future contract |
|-------|---------|--------------|-----------------|
| Startup | Montage begin → first hit frame | `BPC_Attacks` + montages | DataAsset: `StartupSeconds` |
| Active | Damage traces valid | Weapon traces / notifies | `ActiveSeconds` + AttackId dedupe |
| Recovery | End active → idle | Montage end / anim notify | `RecoverySeconds` |
| Combo window | Buffer next input | Existing combo commits | `ComboBufferSeconds` |
| Cancel | Dodge/parry interrupt | Unverified | Explicit cancel tags |

**Damage authority:** Server (listen) should apply via `BPC_DamageSystem`; clients play montages. Phase 3 hardens this.

**Event sequence (normal hit):**  
`NotifyAttackStarted` → `NotifyAttackCommitted` → `NotifyHitConfirmed` → `NotifyDamageApplied` (BP calls Notify* at those points).

---

## Blocking & parrying

| State | Behavior | Notes |
|-------|----------|-------|
| Block | Reduces/prevents eligible damage | `RequestBlock` + sustain after timed window |
| Timed parry | Window after `RequestParry` | `UCombatTimingComponent` + `UCombatTimingConfig` |
| Perfect parry | Leading sub-window → DivineMomentum | Defaults in `GOTA_IMPL_04_06_COMBAT_TIMING.md` |
| Guard break | Punish turtle | Not verified — Phase 2 |
| Unparryable | Distinct telegraph | Pass `bUnparryable=true` into `ResolveIncomingAttack` |

**Event sequence (parry):**  
`RequestParry` → incoming hit → `ResolveIncomingAttack` → if cancel, skip BP damage → `NotifyParrySucceeded` / `OnPerfectParry` → momentum grant on authority.

**PvP:** Server validates window with latency tolerance; clients never authoritatively declare parry success.

---

## Dodging

| Item | Status |
|------|--------|
| Dedicated dodge input/anims | C++ `RequestDodge` ready; **BP anim bind pending** |
| Invulnerability frames | `DodgeIFrameStart/End` on timing config |
| Perfect dodge | Leading i-frame sub-window → momentum |
| FightingAnimsetPro | Prefer retarget rolls — verify in editor |

**Contract:** Before damage, `ResolveIncomingAttack` cancels hits inside i-frames and fires `NotifyPerfectDodge` when perfect.

---

## Divine Momentum

| Rule | Spec |
|------|------|
| Gain | Perfect parry, perfect dodge, clean combo end, interrupt (tunable weights) |
| Spend | Divine counterattack / ultimate |
| Cap | `MaxDivineMomentum` (default 100) |
| Reset | On death; optional drain out of combat |
| Modes | Campaign full; PvP normalized / restricted |

C++ API: `AddDivineMomentum` / `ConsumeDivineMomentum` (authority-only).  
Counter: `UCombatTimingComponent::RequestDivineCounter` consumes cost and opens an auto-perfect window.

---

## Playtest plan (timing)

1. Record montage notify times for sword light combo in PIE.  
2. Expose windows on a DataAsset; scrub until parry feels fair vs `BP_Enemy_Melee`.  
3. Only then lock “perfect” sub-window (~3–6 frames at 60 fps as starting guess — **not final**).

---

## Multiplayer notes

- Local multicast delegates do **not** replicate.  
- Authority resolves hit/parry; clients receive RepNotify / RPC / GameplayCue for FX.  
- Lock-on is local presentation; do not replicate soft camera.
