# ADR: Gameplay Ability System Adoption

**Status:** Accepted recommendation — **Defer GAS until after networked combat vertical slice**  
**Date:** 2026-10-27  
**Reconfirmed:** 2026-10-28 — **Implementation 07**  
**Engine:** UE 5.8  
**Project:** WarOfGods / Gods of the Arena

### Implementation 07 confirmation

Perfect Parry / Dodge / Divine Counter landed on `UCombatTimingComponent` + `UStatsComponent` **without** GAS.  
Do **not** enable `GameplayAbilities` until:

1. BP wires `ResolveIncomingAttack` and two-client defense feels correct  
2. Divine powers need prediction / GameplayCues beyond local Notify*  
3. Phase 6 ability count justifies AttributeSet migration  

Until then, prefer GameplayTags + timing DataAssets over a second framework.

---

## Problem statement

Combat currently lives in Blueprint (`BPC_Attacks`, `BPC_DamageSystem`, `BP_Player`) with new C++ observers (`UCombatComponent`, `UStatsComponent`). We need a long-term home for attributes, abilities, prediction, and status effects without duplicating systems or freezing Phase 1.

---

## Existing implementation

- BP attack/parry/combo/damage (authoritative for feel today)
- C++ event hub + Stats (Health observation mode; DivineMomentum C++-owned)
- No GameplayAbilities plugin enabled in `.uproject`
- Lobby/session Blueprints without GAS cues

---

## Options considered

### A — Adopt GAS now as sole combat framework
Rewrite attacks, damage, parry windows into GameplayAbilities/Effects immediately.

### B — Defer GAS; finish shared combat slice on BP + thin C++
Keep BP rules; route outcomes through typed C++ events; migrate Health later; revisit GAS after 2-client combat works.

### C — Never use GAS; expand custom C++ forever
Build custom prediction, effects, tags, costs.

---

## Decision

**Choose B for Phases 1–3.** Re-evaluate GAS at the start of Phase 6 (divine powers / status complexity) or earlier if prediction pain appears in Phase 3.

---

## Benefits of B

- Preserves working BP combat investment
- Avoids dual health writers during Phase 1
- Events + tags remain GAS-compatible later
- Faster path to playable networked slice

## Risks of B

- Temporary custom Stats may be replaced
- Delayed access to GAS prediction/GameplayCues
- Discipline required to avoid a second ability framework

## Risks avoided by not choosing A now

- Large BP rewrite before behavior is documented
- Plugin/content migration on a just-upgraded 5.8 project
- Blocking arena/combat feel work on framework churn

---

## Migration strategy (when/if GAS lands)

1. Enable `GameplayAbilities` plugin; add module dependency.  
2. Map Health/Stamina/DivineMomentum → AttributeSet.  
3. Convert divine powers first (greenfield), then attacks, then defensive windows.  
4. Keep `UCombatComponent` events as façade over ability callbacks.  
5. Retire `bAuthoritativeMutations` path once AttributeSet owns health.

## Systems retained

- `BPC_Attacks` / `BPC_DamageSystem` until explicit migration steps  
- `UCombatComponent` event API  
- `ULockonComponent`  
- Animation montages / notifies  

## Systems replaced later (if GAS)

- Custom attribute mutation in `UStatsComponent`  
- Ad-hoc status/buff BPs  

## Dependencies

- Phase 1 BP behavior capture complete  
- Phase 3 networked combat smoke test green  
- No parallel “mini-GAS” effect system in the meantime  

---

## Recommendation summary

Do **not** implement GAS in Phase 1. Document tags in a lightweight taxonomy. Prefer GameplayTags now; prefer GAS when ability count and prediction needs justify the migration cost.
