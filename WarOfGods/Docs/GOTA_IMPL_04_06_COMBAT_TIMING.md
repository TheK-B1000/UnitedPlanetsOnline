# Implementations 04–06 — Combat Timing

**Engine:** UE 5.8  
**Status:** C++ landed — BP wiring required for live feel  
**Authority:** `BPC_DamageSystem` still applies damage; timing decides cancel

---

## What shipped

| Impl | Feature | C++ surface |
|------|---------|-------------|
| **04** | Perfect Parry + timing polish | `UCombatTimingComponent::RequestParry` / `ResolveIncomingAttack` |
| **05** | Perfect Dodge + i-frames | `RequestDodge` + dodge i-frame resolve |
| **06** | Divine Momentum counter | `RequestDivineCounter` spends `UStatsComponent` momentum |

Supporting types:

- `UCombatTimingConfig` — tunable DataAsset (create `DA_GOTA_CombatTiming` in Editor)
- `ECombatGuardState` / `ECombatDefenseOutcome` / `FCombatDefenseResolveResult`
- Console: `gota.Combat.DumpTiming`, `ForceParry`, `ForceDodge`, `ForceDivine`, `ResolveTest`
- Bootstrap auto-attaches `UCombatTimingComponent` on player pawns

---

## Default windows (scrub in PIE)

| Window | Default |
|--------|---------|
| Parry window | 0.28 s |
| Perfect parry (from press) | 0.09 s (~5.4 frames @ 60) |
| Latency tolerance | 0.085 s |
| Dodge duration | 0.45 s |
| Dodge i-frames | 0.05–0.28 s |
| Perfect dodge (from i-frame start) | 0.12 s |
| Divine counter cost / active | 50 momentum / 0.75 s |

---

## Blueprint contract (required)

On the **defender**, when a hit would apply:

1. `Timing = GetComponentByClass(CombatTiming)`  
2. `Result = Timing->ResolveIncomingAttack(Attacker, AttackId, bUnparryable)`  
3. If `Result.bCancelDamage` → **do not** call damage apply  
4. Play FX from `OnDefenseResolved` / `UCombatComponent` Notify*  

Input (player):

- Parry press → `RequestParry`  
- Block hold → `RequestBlock(true/false)`  
- Dodge → `RequestDodge(Direction)`  
- Divine → `RequestDivineCounter(Target)` when HUD shows full / enough momentum  

Optional: assign `DA_GOTA_CombatTiming` to the component’s `TimingConfig`.

---

## Momentum grants (authority)

| Outcome | Default grant |
|---------|---------------|
| Perfect parry | +25 |
| Normal parry | +5 |
| Perfect dodge | +20 |
| Divine counter | −50 cost |

---

## Out of scope (these impls)

- Rewriting `BPC_Attacks` / GAS migration  
- Automatic montage playback (BP still owns anims)  
- Server hit-trace replacement  
