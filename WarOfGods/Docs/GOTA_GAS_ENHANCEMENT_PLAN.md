# Selective GAS enhancement plan

**Updated:** 2026-11-15. **Branch:** `feature/gota-cinematic-frontend`. The Burning code commit `548a433` is on `origin`: it was pushed from GitHub Desktop's Push button (its log: `[ui] Executing push ... feature/gota-cinematic-frontend` at 2026-11-14 18:07:31 local), not by an agent. Everything after it is local only. RC1–RC11 packages are not modified; Burning was packaged as RC8, the stabilization pass as RC9, the renderer fix as RC10, all fixes plus ready-up and telemetry as RC11, and the enemy combat work as RC12 / RC12b / RC12c (RC12c is the candidate). **Engine:** Unreal Engine 5.8 native Gameplay Ability System.

We use GAS where it improves on what we already have. Health, momentum, cooldowns, basic attacks, block, perfect parry, perfect dodge, Divine Counter, and boss timelines keep their current owners. No talent trees, XP, levels, or persistent progression.

## Where we are

| Milestone | Commit | State |
|---|---|---|
| GAS plugin, combat ASC, Time Lash's slow as a gameplay effect | `3bc3fb2` | Done |
| Dread on GAS; shared status base; strongest-penalty-wins; death cleanup; bootstrap ASC; HUD | `cf869ce` | Done |
| Veiled on GAS | `5a16a2d` | Done |
| Unbreakable on GAS; all four statuses are effects | `6f1d135` | Done |
| Four-player Story / Horde status runs, `gota.Net.Status`, this document | `d103007` | Done |
| Boss status policies, resist feedback, lazy enemy ASC, cost measurement | `0a9f852` | Done |
| Packaged (RC7) regression and this record | `bb69d11` | Done |
| Burning (damage over time), Molten Strike ignites, element definitions, BURNING indicator | `548a433` | Done |
| Packaged (RC8) regression and the Burning record | `66e96c8` | Done |
| RC8 stabilization: cloth crash mitigation, Story shrine and Horde findings, packaged RC9 regression | `839f397` | Done |
| Horde director fix (victory-time array bug) | `04d35a4` | Done (packaged in RC10) |
| Story shrine boss always lands after its floating attack (`UGOTAFloatAttackGuard`) | `74c480d` | Done (packaged in RC11) |
| Horde wave enemies get their AI controller and attack | `f423fad` | Done (packaged in RC11) |
| Post-victory renderer crash: `r.SkeletalMesh.UseCachedMDCs=0` | `b619ee5` | Provisionally validated (packaged RC10, 3 of 3 clean); cost measured in RC11 (+1.4 ms render thread) |
| Horde ready-up, end-phase fix, combat telemetry, RC11 package | `a8e6e87` | Done (packaged RC11); human playtest not yet run |
| RC12 enemy combat fixes (spin, replication, dead AI, armed heroes, targeting, Story bases, lobby) | `336b6cb` | Done (packaged RC12c) |
| Reflection-buffer crash fix, lobby button | `10f4bb2` | Done (packaged RC12c) |
| Boss definition kept loaded on the server | `05704ae` | Done (packaged RC12c) |
| Milestone A: Stunned and Frozen | `3475459` | Done (packaged, 51/51) |
| Milestone B: shields, buffs, debuffs | `9d53e51` | Done (packaged, 54/54) |
| Milestones C and D: elements, hero abilities | `a38b241` | Done (editor 63/63; packaged 62/63 first pass, the one timing failure passed 3 of 3 on rerun) |
| Milestone E, part 1: projectiles, zones, World Burn zone | `d6a9ae2` | Editor only: 65/66, the one failure a stale ignite count; Heroes and Delivery 8/8 after correcting it. Full suite not repeated. No packaged run |
| Milestone E: remote-client delivery test | `a956541` | Editor, listen server + one client: pass |
| Milestone E, part 2: enemy powers | `729852b` | Editor, clean build: 72/72 on the first run. Packaged: 70/72 on the first pass, see the milestone E section. Four-player and latency runs still to do |
| Milestones F, G | | Not started |

## GAS completion roadmap (from 2026-11-18; GAS is the sole feature priority)

Audit of the RC14 baseline (`cdd2214`):

| Area | State | Where |
|---|---|---|
| ASC on the pawn, Mixed replication, combat AttributeSet (walk speed, outgoing damage; strongest penalty wins) | done | `GOTAAbilitySystemComponent`, `GOTACombatAttributeSet` |
| Statuses Slowed, Dread, Veiled, Unbreakable, Burning (DoT, attribution) | done | `GOTAStatusEffects` |
| Boss status policies (aspect immunity, partial resistance), resisted cue and feedback | done | `GOTAStatusPolicy`, `UGOTABossDefinition` |
| Element table (Fire filled; Frost, Lightning, Shadow named with no status), element tag on abilities | partial | `GOTAElements` |
| Frozen / Stunned | **missing** (tags reserved, `GOTA.Immunity.Stun` has no reader) | |
| Shields, incoming damage reduction / amplification, buffs | **missing** (Unbreakable refunds after the fact through `Heal`) | |
| Per-element resistances and interactions | **missing** | |
| Card effects not implemented: Mind Spike stun (Nyx), Void Shield "negates control", World Burn burning area, Colossus Form power, Soul Drain energy steal | **missing** | `Tools/GOTA_BuildHeroAbilities.py` |
| Projectiles, persistent zones, enemy supernatural abilities | **missing** (abilities are instant arcs / bursts / charges) | |
| Status presentation: overlays for 4 statuses (Slowed, Dread faint), flames for Burning, local BURNING line, resist line; no icons or durations except the Duel HUD text | partial | cues, `UGOTACombatFeedbackSubsystem` |

Ordered checklist (each step verified before the next; focused tests during work, full editor + packaged regressions at each milestone):

- **A. Frozen and Stun** (hard crowd control). A1 effects, tags, cues, `Control` aspect (`GOTA.Immunity.Stun`); rules: one hard CC at a time, not extended while active, refused on an invincible target (Veiled, a dodge's i-frames, a boss's floating phase), 2 s recovery immunity after it ends. A2 the lock: server cancels the cast, denies abilities, Divine Counter and dodge arming, rejects the player's combat RPCs, ends a block, stops montages, pauses AI and stops movement; the owning client disables input; everything resumes on expiry (never on the dead). A3 Frozen: animation paused on every peer, ends early on the first real hit (shatter; status ticks do not). A4 overlays, local STUNNED / FROZEN line, resist feedback (bosses immune by default). A5 first user: Mind Spike's last strike stuns (Nyx card). A6 tests.
- **B. Shields, buffs, debuffs.** Incoming damage hook before the Blueprint damage system (all damage, including Blueprint melee); `Shield` pool attribute with absorb, break and expiry; incoming damage multiplier (reduction / amplification); outgoing damage and walk speed buffs; data-driven effect rows with explicit stacking and refresh; interactions with Dread, Veiled, Unbreakable, Slowed.
- **C. Elements.** Frost, Lightning, Shadow rows with statuses; per-element resistance in status policies, applied at the two elemental damage points; a small fixed interaction table (for example Fire thaws Frozen, Frost puts out Burning).
- **D. Hero abilities.** Card effects above; reuse A–C; no second cooldown system.
- **E. Projectiles, AOE zones, enemy powers.** Replicated server-hit projectile, persistent zone with periodic effects, an enemy ability path that uses the same effects and policies.
- **F. Presentation.** Status icons with time left for the local player and the target, shield bar, element VFX per status, sounds, consistency in Duel, Story, Horde.
- **G. Final integration.** Multiplayer, bosses, respawn, performance, packaged regressions; this document updated.

## Ownership

| Value or decision | Single owner | GAS role |
|---|---|---|
| Health, damage outcome, block, death | `BPC_DamageSystem` via `FGOTABlueprintCombat::ApplyDamage` | none |
| Momentum | `UStatsComponent` / `UGOTADuelFighterComponent` | none |
| Ability cooldowns, costs, cast timeline, range, LOS, hit detection | `UGOTAHeroAbilityComponent` | none |
| Statuses Slowed, Dread, Veiled, Unbreakable, Burning, Stunned, Frozen | **`UGOTAAbilitySystemComponent` (gameplay effects)** | owner |
| Hard crowd-control lock and release (`GOTA.State.Incapacitated`) | **`GOTAGAS::SetControlLock`**, enforced by the ability component, fighter, player RPC guard and AI brain | owner |
| When a Burning tick happens, who owns it, how much it asks for | **`UGOTAGameplayEffect_Burning`** (period, instigator, `Data.TickDamage`) | owner |
| What a Burning tick does to health | `BPC_DamageSystem`, called by `GOTAGAS::DealStatusDamage` | caller only |
| Element tuning (which status, duration, strength, tick interval, effect) | `UGOTAElementSettings` (Project Settings > GOTA Elements) | data |
| Which abilities are elemental and ignite | `FGOTAAbilityDef::Element`, `bInflictsElementStatus` (`Tools/GOTA_BuildHeroAbilities.py`) | data |
| "BURNING" line for the local player, flames on a burning body | `UGOTACombatFeedbackSubsystem`, `UGOTAGameplayCue_Burning` | presentation |
| Walk speed multiplier, outgoing damage multiplier | **`UGOTACombatAttributeSet`** | owner |
| Which effect aspects a combatant ignores (bosses) | **status policy** on `UGOTABossDefinition`, granted as tags by the ASC | owner |
| "Resists" feedback on screen | `UGOTACombatFeedbackSubsystem` (local), fed by the Resisted cue | presentation |
| `MaxWalkSpeed` writes | `UGOTAHeroAbilityComponent::ApplyMovementStatusLocally` (reads the attribute) | read |
| Blueprint `IsInvincible` while Veiled, `IsInterruptible` while Unbreakable | the ASC's tag events write them; `BPC_DamageSystem` still reads them | bridge |
| Unbreakable damage refund | `UGOTAHeroAbilityComponent` through the Blueprint's own `Heal` | reads tag + magnitude |
| Status overlay on the body | `GOTAGAS::RefreshStatusOverlay`, called by the cue | owner |
| Perfect parry, perfect dodge, melee, Divine Counter | fighter + Blueprint actions | none |
| Boss attacks, staggers, parry recoil, Divine Counter stun, waves, score, victory | `UGOTABossComponent`, game modes | none (boss attacks read the damage multiplier once) |

The legacy status slot on `UGOTAHeroAbilityComponent` still compiles. `GOTAGAS::OwnsStatus` decides, per status, whether GAS or the slot applies it. Today it returns true for all four. Returning false for one status rolls it back without other changes.

## Architecture

- **ASC owner and avatar are the pawn.** Health already lives on the pawn, and every respawn (`RestartPlayer` in Duel, Horde, and Story) creates a new pawn, so statuses can never carry over. The PlayerState keeps `HeroId`, which is where a future loadout or perk choice would live.
- **Who gets one, and when.** Players get theirs from `UCombatBootstrapSubsystem` at possession; bosses from `UGOTABossComponent::ApplyToPawn` (with their status policy). Plain AI enemies get one only when their first status lands (`gota.GAS.EnemyASC 0`, the default). An idle ASC on 34 extra enemies measured +31% host upload and about 1.5 ms host frame time, so regular enemies only pay when GAS is used on them. Clients receive the replicated component.
- **Replication: Mixed.** The owning client gets full effects (its HUD shows time left). Everyone gets tags, cues, and attribute values.
- **Effects are native C++ classes** (`UGOTAStatusGameplayEffect` and five subclasses; Burning adds a period and `UGOTABurnTickExecution`). Duration and strength are SetByCaller values taken from the existing `FGOTAAbilityDef`, so balance numbers stay in one table.
- **Cues:** one native `UGOTAGameplayCue_StatusOverlay` is registered for all four cue tags in the runtime cue set on every peer.
- **Cues:** a second native cue, `UGOTAGameplayCue_Resisted`, is executed when a target resists all or part of a status.
- **Cues:** a third native cue, `UGOTAGameplayCue_Burning`, attaches the Fire element's looping effect to a burning body.
- **Tags:** `GOTA.Status.*`, `GOTA.Immunity.*` (Slow, Weaken, Stun, Interrupt, Burn), `GOTA.Element.*`, `GOTA.Data.*`, `GameplayCue.GOTA.*` (`GOTAGameplayTags.h`). Frozen and Stunned are reserved names with no reader yet.

## Status interaction rules

| Rule | Behavior |
|---|---|
| Different statuses | Coexist. A new status never removes another. The old slot replaced whatever was active, so Time Lash used to strip Unbreakable or Dread. |
| Same status again | One stack per target. Reapplying refreshes the duration and keeps the running strength. Every current source of a status uses the same strength. |
| Overlapping penalties | **Strongest penalty wins** for walk speed and hero-ability damage, using the engine's `MostNegativeMod_AllPositiveMods` aggregator. Dread (×0.65) + Time Lash (×0.60) walk at ×0.60, not ×0.39. Future buffs still add on top. |
| Damage modifier | Dread's ×(1 − 0.5 × Magnitude) is read once per hit, before `BPC_DamageSystem`: in `DealDamage` for hero abilities and in `UGOTABossComponent::DamageTarget` for boss attacks. Blueprint melee does not read it, as before. |
| Expiry | Each status expires on its own timer. Movement and overlay fall back to whatever is still active. |
| Removal | Death (`UStatsComponent::OnDied`), ability reset, and test reset remove every `GOTA.Status` effect. Respawn is a new pawn. Session travel is a full map load; no seamless travel is configured, so nothing survives it. |
| Overlay priority | Veiled > Unbreakable > Dread > Slowed. One overlay material at a time; when the top status ends, the next one shows. |
| Immunity and resistance | From a status policy; see below. |
| Burning beside other statuses | Coexists with all four. It has no attribute modifier, so it never takes part in "strongest penalty wins", and it has no overlay, so it never hides another status's overlay. |
| Burning again | One effect per target. Re-applying refreshes the duration and hands ownership to the new applier; the tick timer keeps its beat, so the rate never exceeds one tick per interval. |

Formulas kept from the slot: walk ×clamp(1 − Magnitude, 0.2, 1); Dread damage ×clamp(1 − 0.5 × Magnitude, 0.2, 1); Unbreakable refund = health lost × clamp(Magnitude, 0, 0.9).

## Burning and elements

### What Burning is

`GOTA.Status.Burning` is a duration effect with a period. Numbers come from the Fire row of `UGOTAElementSettings`: **3 s, 2 damage per second, 1 s tick interval** (three ticks of 2.0, 6 damage). Molten Strike is the first user: impact 22 as before, then the burn, 28 in total on a target that stays alive. The card text "Ignites enemies" is now true and the in-game description says so.

Burning is not Molten Strike code. Any ability with `Element = GOTA.Element.Fire` and `bInflictsElementStatus` ignites, and `gota.Ability.Status` / `gota.Net.Status` can apply it directly.

### Tick schedule (server clock)

| Moment | What happens |
|---|---|
| Applied at t = 0 | Tag, cue and flames. **No tick.** |
| t = 1, 2, 3 | One tick each, `Data.TickDamage` = damage per second x interval. |
| t = 3 | The tick that falls on expiry still fires, then the effect is removed. Exactly three ticks; the engine's own duration check runs the last one (`CheckDuration`), there is no fourth. |
| Re-applied at t = 1.5 | Duration restarts (ends at 4.5). The period timer is **not** reset: ticks stay at 2, 3, 4. New applier, new numbers. |
| Target dies | `ClearStatusEffects` removes it; a tick that kills ends the effect inside the same tick. |

Ticks only run on the server. Clients get the tag, the cue and (owner only) the effect; no per-tick message is sent.

### Damage ownership

A tick calls `GOTAGAS::DealStatusDamage`, which calls `FGOTABlueprintCombat::ApplyDamage` on the victim's `BPC_DamageSystem::TakeDamage`. GAS has no health attribute and writes no health. One call per tick; the log line carries health before and after.

A tick is damage but **not a hit**:

| Rule | Hit (Molten Strike impact) | Burning tick |
|---|---|---|
| Block / parry | Blockable | Cannot be blocked or parried |
| Blueprint response | `Stagger`, forces an interrupt | `None`, no forced interrupt |
| Hero-ability cast interrupt | Interrupts the wind-up | Does not (`OnStatusDamage` discounts it) |
| Divine Momentum | +3 hit landed, +2 ability hit | None |
| Dread on the attacker | Scales the damage (x0.825) | Not scaled; the burn is the element's number |
| Veiled (`IsInvincible`) | Untargetable | Tick does nothing while veiled; the effect keeps running |
| Unbreakable | Half refunded | Half refunded (1.0 of 2.0) |
| Boss stagger threshold | Counts | Counts (the boss component sums health lost) |
| Combat not live (between rounds, intermission) | No casts | Tick skipped |

### Attribution

- The effect's instigator is the pawn that applied it. Each tick is credited to it through `UStatsComponent::NoteDamageFrom`, the same call ability hits use, so kill credit, score and Horde eliminations follow.
- Re-application replaces the instigator. With two players burning one enemy, ticks after the second application belong to the second player. Whoever dealt the last damage before death gets the kill, as for every other damage source.
- If the applier's pawn no longer exists, ticks still deal damage and credit nobody.

### Elements

| Piece | Where | Notes |
|---|---|---|
| Damage type | `GOTA.Element.Fire / Frost / Lightning / Shadow / Void / Time` | No tag = untyped (physical). |
| Per-element row | `FGOTAElementDef` in `UGOTAElementSettings` | Status, duration, strength, tick interval, looping effect. Fire is filled in; Frost, Lightning and Shadow are named rows with no status. |
| Ability | `FGOTAAbilityDef::Element`, `bInflictsElementStatus` | Read once when a hit lands. Nothing per frame. |
| Effect | Element tag is an asset tag on the status effect | So a future resistance can match on it. |
| Immunity | `GOTA.Immunity.Burn` in a status policy | Refuses Burning; the impact still lands. |
| Resistance | `FGOTAStatusResistance` for Burning | `DurationScale` shortens, `MagnitudeScale` lowers damage per second. |
| Future per-element damage scaling | `UGOTAHeroAbilityComponent::DealDamage` and `GOTAGAS::DealStatusDamage` | The only two places elemental damage is dealt. Not implemented. |
| Rollback | `gota.GAS.ElementStatus 0` | Abilities deal damage only; nothing ignites. |

Elemental combinations (for example Frost on a Burning target) are not designed and not implemented.

### Presentation

- **Flames:** the Fire row's `StatusFX` (`P_Fire_Small` from Realistic Starter VFX Vol 2, the brazier fire the arenas already use) attached to the body's root. One component per actor, found by tag, so re-application cannot stack them. Removed by the cue's `OnRemove` (expiry, death, reset) and owned by the actor, so it is destroyed with it.
- **Local indicator:** `UGOTACombatFeedbackSubsystem` shows "BURNING" while the local player's own pawn burns. It is driven by `GOTAGAS::OnStatusCue` (raised by the status cues) and by the pawn's `OnDestroyed`; nothing polls. It works in Duel, Horde and Story. The Duel HUD's older per-tick status text is unchanged and does not list Burning.
- **Boss immunity:** the existing resist line, "THE QUARRY BEHEMOTH RESISTS BURNING" or "... SHRUGS OFF PART OF BURNING".

## Status policies (boss immunity and resistance)

A status policy (`FGOTAStatusPolicy`) says how a kind of combatant takes status effects. Bosses carry one on `UGOTABossDefinition::StatusPolicy`; the C++ default for every boss is **immune to `Slow` and `Stun`**, and each boss asset can override it in the editor. Nothing about it is Rampage-specific.

Statuses are split into independent **aspects**, each with its own immunity tag:

| Aspect | Immunity tag | Statuses that have it |
|---|---|---|
| Movement impairment | `GOTA.Immunity.Slow` | Slowed (all of it), Dread (its walk modifier) |
| Attack-power reduction | `GOTA.Immunity.Weaken` | Dread (its damage modifier) |
| Hard crowd control | `GOTA.Immunity.Stun` | Stunned, Frozen (also granted for 2 s after either ends) |
| Cast interruption | `GOTA.Immunity.Interrupt` | the hero-ability interrupt check (as Unbreakable) |
| Fire damage over time | `GOTA.Immunity.Burn` | Burning (all of it). No boss has it by default: the Quarry Behemoth burns. |

How a status meets a policy (`GOTAGAS::ApplyStatus`, server):

1. Every aspect blocked: the status is refused. Time Lash on the Quarry Behemoth still deals its 6 damage; the slow is refused.
2. Some aspects blocked: the effect applies and the blocked modifier is set to a neutral ×1. Dread on the Behemoth weakens its attacks (Claw Combo 9 → 7.4) but does not slow it.
3. A per-status resistance (`DurationScale`, `MagnitudeScale`) shortens or weakens what remains; a duration scale of 0 is full resistance.
4. Any of the above executes `GameplayCue.GOTA.Resisted`. Every peer logs it; the player who applied the status sees "THE QUARRY BEHEMOTH RESISTS SLOW", "…RESISTS THE SLOW OF DREAD" or "…SHRUGS OFF PART OF …" on a centred line in any mode.

What a policy does **not** touch: a boss's own stagger threshold, parry recoil (1.6 s) and the Divine Counter stun (2.6 s, 90 damage). Those belong to `UGOTABossComponent` and are not gameplay effects, so `GOTA.Immunity.Stun` cannot block them.

Why the default boss policy blocks slows: the boss component writes its own walk speed (420 / 520 per phase, 1500 during a charge, 0 while stunned). A slow scaled whatever speed it found, including the charge rush, so Time Lash could blunt a designed attack.

Engine limitation, and how it is handled: for persistent effects, Unreal evaluates attributes without target tags, so a per-modifier immunity filter is never checked. The adapter therefore neutralises a blocked aspect when the effect is applied. The modifier filters are kept because instant and periodic effects (future Burning) do honour them. An immunity granted after a status has already landed does not cancel that status; policies are granted at spawn, so this does not happen today.

## Hard crowd control: Stunned and Frozen (milestone A)

Both are status effects (`UGOTAGameplayEffect_Stunned`, `UGOTAGameplayEffect_Frozen`) that grant their own `GOTA.Status` tag and the shared **`GOTA.State.Incapacitated`**. Every lock and release hangs off that one tag (`UGOTAAbilitySystemComponent` → `GOTAGAS::SetControlLock`, on every peer).

| Rule | Behavior |
|---|---|
| What it stops | Movement (walk speed 0 on the server and the owning client), melee, spells, block, parry, dodge, Divine Counter, hero abilities. The current action is cut short: hero-ability cast cancelled (cooldown stays spent), block ended, attack montage stopped. Damage already dealt stands. |
| Server enforcement | Abilities: `EGOTAAbilityDenied::Incapacitated`. Player combat RPCs `SR_MeleeAttack / SR_ShootMagicSpell / SR_StartBlock / SR_ParryAttack` refused, and so are those events (and their `MC_` multicasts) when C++ calls them on the authority. Divine Counter and perfect-dodge arming refused. AI: brain paused, movement stopped, focus cleared. |
| Local input | The locally controlled pawn's input is disabled and re-enabled (listen host and remote clients alike). A Blueprint's own internal calls cannot be intercepted from C++, so on the authority the input lock is what stops a human; the server rules above stop everything that reaches it. |
| Release | Expiry, Frozen's shatter, death, round reset. The AI's brain is resumed **and restarted from the root** (blackboard kept): a resumed move task waited forever for the move the lock had stopped, and released enemies far from the players never walked again (found in the Horde run). Enemy targeting pauses while locked and orders the attack again on release; the stall watch ignores locked enemies. A defeated AI is never woken. |
| Duration and refresh | From the source (ability definition or element row). **Not extended while active** and **one hard CC at a time**: a second Stunned or a Frozen on an incapacitated target is refused ("already incapacitated … not extended"). |
| Recovery | When it ends (not by death), `UGOTAGameplayEffect_ControlRecovery` grants `GOTA.Immunity.Stun` for `gota.GAS.ControlRecovery` seconds (default 2). New Stunned / Frozen in that window is resisted with the usual feedback ("TARGET RESISTS STUN"). No combination of casters can chain-lock a target. |
| Invincible targets | Refused while the target's damage system is invincible: Veiled, a dodge's i-frames, a boss's floating phase. |
| Bosses | Immune by default (`GOTA.Immunity.Stun` in the boss status policy): "THE QUARRY BEHEMOTH RESISTS STUN / FREEZE". The boss's own staggers, parry recoil and the Divine Counter stun are not effects and are unchanged. |
| Frozen | Element Frost. Animation held (`bPauseAnims`) on every peer by the cue. The first **real hit** removes it early ("Frozen shattered by a hit"); status ticks (Burning) do not count. |
| Coexistence | Coexists with every other status. Burning keeps ticking on a stunned or frozen target. |
| Presentation | Overlays `M_GOTA_Overlay_Frozen` (pale ice) and `M_GOTA_Overlay_Stunned` (gold), above every other status in priority. Local line "FROZEN" / "STUNNED" (with "BURNING" if both: "FROZEN  \|  BURNING"). |
| First user | Nyx **Mind Spike**: the last of its three strikes stuns for 0.8 s if it lands (card: "Stuns and disrupts"). A striking ability's status is applied only to enemies its damage reached. |

Ownership: the lock is `GOTAGAS::SetControlLock`; walk speed stays with `UGOTAHeroAbilityComponent` (the only `MaxWalkSpeed` writer; created on an enemy when a hard CC lands, as for slows); the shatter rule lives in its server tick next to the Unbreakable refund.

## Shields, buffs and debuffs (milestone B)

Six status effects, numbers from the ability definition (`StatusDuration`, `Magnitude`), like every status:

| Status | Effect | Immunity |
|---|---|---|
| Shielded | absorbs `Magnitude` damage from every source before health; breaks when used up; the unused part ends with the effect | none |
| Empowered | outgoing ability and boss damage ×(1 + Magnitude) | none |
| Weakened | outgoing ability and boss damage ×(1 − Magnitude) | `GOTA.Immunity.Weaken` |
| Fortified | incoming damage from every source ×(1 − Magnitude) | none |
| Vulnerable | incoming damage from every source ×(1 + Magnitude) | `GOTA.Immunity.Vulnerable` (new) |
| Hasted | walk speed ×(1 + Magnitude) | none |

**Attributes.** `IncomingDamageMultiplier` (new) joins walk speed and outgoing damage, all three in [0.2, 3] with the same aggregation: only the strongest penalty counts and every bonus adds (Fortified 0.5 + Vulnerable 0.5 = ×1.0; Hasted +0.3 with Slowed 0.4 = ×0.9; Empowered 0.5 with Weakened 0.5 = ×1.0). `ShieldPool` (new) is written by the server's shield bookkeeping and replicated for HUDs.

**Stacking.** One per target. Re-applying refreshes the duration and keeps the running strength; a **stronger** application replaces the running one ("replaced (x40.00 -> x60.00, stronger)"). Found while testing: a stacked GAS re-application takes the new spec's numbers, so a weaker one is re-made with the running strength.

**Incoming damage, every source.** C++ damage (hero abilities, boss attacks, status ticks) goes through `FGOTABlueprintCombat::ApplyDamage`, which applies the victim's multiplier **before** the Blueprint sees the number. A Blueprint hit (enemy and player melee) cannot be intercepted before the Blueprint subtracts health (Blueprint-to-Blueprint calls never reach C++), so the victim's ability component corrects it in the same server tick: a refund through the Blueprint's own `Heal` (Fortified) or extra damage that is not a hit (Vulnerable). Known limit: a Blueprint hit that is lethal before the reduction still kills; C++ damage has no such edge.

**Shield as over-health.** On application the Blueprint's `MaxHealth` and `Health` rise by the pool; as damage lands the pool shrinks and `MaxHealth` falls with it, so heals (including the Horde intermission restore) never refill the shield, and a hit bigger than shield + health still kills by the Blueprint's own rule. On expiry or removal the unused pool is taken back off health. Order in one server pass: incoming multiplier, shield, Unbreakable refund (Unbreakable only refunds what reached health).

**Bookkeeping owner.** `UGOTAHeroAbilityComponent::TickDefense` (server), which replaces the Unbreakable-only refund; the component is created on any combatant a status needing it lands on, as for slows.

## Elements: Fire, Frost, Lightning, Shadow (milestone C)

| Element | Status from a hit (element row) | Notes |
|---|---|---|
| Fire | Burning, 2 per second for 3 s | unchanged |
| Frost | **Chilled**, walk ×0.7 for 3 s | another Frost hit while chilled: **deep freeze**, Frozen 1.5 s (the row's `FollowUpStatus`); hard-CC rules apply, so the recovery immunity also stops a third |
| Lightning | **Shocked**, incoming damage ×1.2 for 3 s | aggregates like Vulnerable |
| Shadow | Dread 0.25 for 3 s | the status Nyx and Varoth already use |

**Interaction table** (`GOTAGAS::ApplyElementOnHit`, fixed, after the damage): Fire on a Chilled or Frozen target thaws both and does not ignite; Frost on a Burning target puts it out and does not chill; Frost on a Chilled target deep-freezes. Nothing else combines.

**Element resistance.** `FGOTAStatusPolicy::ElementResistances` (element, damage scale 0–3), applied at every elemental damage point: hero abilities with an element (`DealDamage`), status ticks of that element (`DealStatusDamage`: Burning is Fire) and elemental hits from other code (`GOTAGAS::DealElementalHit`, the entry point enemy powers and projectiles use). 0 = no damage, and a hit that lands no damage carries no status. Status immunities still apply per aspect: the default boss policy (immune to slows) means Frost never chills or freezes a boss.

**Entry points.** Hero abilities keep `Element` + `bInflictsElementStatus` on the ability definition; any other code calls `GOTAGAS::DealElementalHit`. Test commands: `gota.Ability.ElementHit <fighter> <Element> <damage> [source]`, `gota.Ability.ElementResist <fighter> <Element> <scale>`. Rollback: `gota.GAS.ElementStatus 0` (damage only).

## Hero abilities (milestone D)

The card effects use the statuses and elements from A–C through new data fields on `FGOTAAbilityDef` (`BonusStatus`, `BonusStatusDuration`, `BonusMagnitude`, `bCleansesControl`, `bUsableWhileIncapacitated`, `MomentumSteal`), set in `Tools/GOTA_BuildHeroAbilities.py`. There is no second cooldown system; `UGOTAHeroAbilityComponent` still owns activation and cooldowns, and `UStatsComponent` still owns Divine Momentum.

| Hero | Ability | New effect |
|---|---|---|
| Nyx | Mind Spike | Stuns for 0.8 s on the last strike (shipped in milestone A) |
| Nyx | Void Shield | Usable while stunned or frozen, breaks free of control, Veiled 1.3 s, then a 25-point shield for 4 s |
| Nyx | Soul Drain | Steals up to 20 Divine Momentum from the target, in addition to Dread |
| Drakon | Unchained Fury | Fire: ignites |
| Varoth | World Burn | Fire: ignites and still dreads. Milestone E adds a 4 s burning zone (provisional balance, see below) |
| Varoth | Colossus Form | Also Empowered: +25% outgoing damage for 6 s |
| Zephyr | Gale Rush | Lightning: shocks |
| Lysara | Gravity Well | Frost: chills |

## Projectiles and zones (milestone E, in progress)

`GOTAEffectDelivery` adds a data payload (`FGOTAEffectPayload`), a straight-line projectile (`AGOTAProjectile`) and a persistent area (`AGOTAEffectZone`). The server alone decides who is hit. Clients receive the initial state once and draw it. Every hit goes through `GOTAGAS::DeliverEffect`, then `DealElementalHit` and the victim's `BPC_DamageSystem`, so there is no second health, immunity or attribution path.

- **Targets (`GOTAGAS::CanAffect`).** Living combatants only, never the source, a co-op ally or an untargetable (Veiled) pawn. Enemy powers never hit other enemies.
- **Dodge.** A dodge timed within 0.35 s of a projectile's arrival protects against that one projectile, through its impact plus 0.15 s. An early dodge protects nothing. Zones are avoided by leaving them, so a dodge gives no protection from a zone pulse.
- **Projectile.** Swept every server frame. World geometry stops it, and it ends at maximum range.
- **Zone.** A server timer at its interval, with no per-frame work. Needs a clear line from the centre. A zone with 0 damage applies only its element's status, and full resistance stops that too.
- **Cleanup.** Deliveries end when their source is defeated, its abilities reset for a round, or the round is no longer live.
- **World Burn (provisional balance).** It leaves a 4 s, 1 s interval burning zone in its radius. Standing in it keeps Burning refreshed, so a target that stays takes up to 7 burn ticks (14 damage) instead of 3 (6). This has had no human balance testing and is not to be retuned without evidence.

**Enemy powers (Quarry Behemoth, data in `DA_Boss_QuarryBehemoth`, built by `Tools/GOTA_BuildBosses.py`).** Both are boss attack entries that use the delivery path above, so there is no second damage system. Their numbers are provisional.

| Power | Use | Warning and counterplay |
|---|---|---|
| Molten Boulder | Fire projectile, 12 damage plus ignite, range 420 to 1500, cooldown 9 s, blockable | Telegraph text and a 0.95 s wind-up. Sidestep, block, shield or a timed dodge |
| Scorched Earth | Phase II only. Fire zone, radius 300, 5 s, 1 s pulses, no direct damage, never staggers, cooldown 14 s | Telegraph text and a 1 s arming delay. Walk out. A dodge does not protect |

**Verified in the editor and package.** Overlapping zones never stacked: two zones gave 4 or 5 ticks of 2.0, and a shield absorbed each. World Burn gave 6 or 7 ticks (14 damage at most), all credited to the caster. A caster's death ended its zone and projectile, and a boss's death cleared a zone that had not armed.

**Burn tick count varies by one.** A zone refresh restarts Burning's duration but not its tick timer, so the expiry and the last tick can fall within a few milliseconds of each other. Frame order then decides whether that last tick fires. The upper limit holds, and the result can only be one tick (2 damage) lower. The two tests were changed from an exact count to these ranges after the first packaged pass exposed this. Game code was not changed.

**Packaged record (`729852b`, `B:\Unreal\GOTA_Builds\GAS_E_729852b`).** First pass: 70/72.

| First-pass failure | Cause | Rerun on the same package |
|---|---|---|
| Time Lash, hit + slow + restore + cooldown + radius | Frame stall: the 0.5 s cast took 1.17 s to land, so the second press arrived during recovery. Same failure as the C + D package | TimeLash group, 3 passes: 12/12 cases, test unchanged |
| Overlapping zones | Exact count of 5 ticks was wrong (see above); this run had 4 | Delivery group, 5 passes: 30/30 cases, with the corrected range |

**Remote client (editor, listen server + one client, no added latency or loss).** The host decided a projectile hit and three zone pulses. The client saw both deliveries, received the impact, removed both, and logged no hit of its own.

**Known limits.** Only the Quarry Behemoth has powers. Regular Horde enemies and Story's Blueprint shrine boss do not. The ground-marker effect has not been checked by eye. A Blueprint hit that is lethal before reduction still kills. The host's Blueprint 0.20 s i-frames also cover other attackers. A modified client's local dodge is only corrected by server movement.

Not done yet: packaged verification, four-player Horde and Story with these effects, and a run under simulated latency and loss.

## Behavior changes

1. Statuses coexist (the point of this milestone). Before, a second status silently replaced the first.
2. Statuses are removed on death. Before, they ran out on their own.
3. The Duel HUD lists every active status at once instead of the single slot.
4. Bosses are no longer slowed by Time Lash or by Dread. Before, a slow also slowed the boss's charge rush.
5. Dread on a boss now weakens its attacks (×0.825). Before, boss attacks ignored Dread.
6. Plain Horde enemies get an ability system only when a status lands on them.

7. Molten Strike ignites: 6 more damage over 3 s on each enemy it lands on. Its impact, cooldown, animation, hit detection and momentum are unchanged.
8. `FGOTABlueprintCombat::ApplyDamage` takes an optional `bForceInterrupt` (default true, the old behaviour). Only status ticks pass false.
9. Milestone A: Nyx's Mind Spike stuns (0.8 s, last strike, bosses immune). Bosses now also refuse Stunned and Frozen with the resist line.
10. The local status line shows FROZEN and STUNNED as well as BURNING (several at once, most important first).
11. Milestone B: all incoming damage passes through the shield and incoming multiplier, including Blueprint melee, which is corrected just after the Blueprint subtracts health.
12. Milestone C: Frost, Lightning and Shadow hits apply their statuses, and the fixed interactions in the element section apply.
13. Milestone D: the hero abilities in the table above gain their card effects. These change hero balance and have had no human balance testing.

Apart from these, the same numbers, Blueprint flags and damage path are in use.

## Test record

### GAS milestones A to D (`3475459`, `9d53e51`, then C and D together), editor and packaged

| Check | A: Stunned / Frozen | B: shields, buffs, debuffs | C + D: elements, hero abilities |
|---|---|---|---|
| New suite group | Control 5/5 (stun, frozen, boss immunity, Mind Spike, multiplayer) | Defense 3/3 (shield, modifiers, multiplayer) | Element 4/4 (table, resistance, boss, multiplayer); Heroes 5/5 (Void Shield, Soul Drain, Varoth, Gale Rush, Gravity Well) |
| Editor suite | 51/51 first pass | 54/54 first pass | **63/63 first pass** |
| Packaged | `GAS_A_3475459` (fresh cook, 3,939 packages, 0 incrementally skipped): **51/51 first pass** | `GAS_B_9d53e51` (fresh cook, 3,945 packages): **54/54 first pass**, run from the staged build after the archive copy failed for disk space; archive completed afterwards | `GAS_CD_a38b241` (fresh cook, 3,947 packages, 0 incrementally skipped, `.ucas` 8.68 GB verified): **62/63 first pass**. The failure was Time Lash `hit+slow+restore+cooldown+radius`: a first-use shader compile and audio underrun stalled the frame, the 0.5 s cast landed about 1.03 s after it started, and the scripted second press arrived during recovery, so it was refused as Busy rather than Cooldown. Rerun of the Time Lash group on the same package, test unchanged: **12/12 (3 passes of 4 cases)**. The full 63-case suite was not repeated |
| Horde, four players, 120 ms / 5% loss | whole waves stunned and frozen five times (`gasA_horde3`): 19 locks, 19 releases, every released enemy attacked again, 0 stalls, 0 crashes | not run | not run |

Not tested for B, C or D: Horde and Story with these effects, human play and balance, separate PCs, quiet-machine profiling. The Heroes multiplayer path is covered only through the shared Element and Defense multiplayer cases; no hero-specific case runs on a remote client.

Known limits: a Blueprint hit that is lethal before reduction still kills; the host's Blueprint 0.20 s i-frames also cover other attackers, while remote players' do not reach the server; a modified client's local dodge is corrected only by server movement; the HUD health bar is inflated by a shield until milestone F.

Disk: drive B: filled up during the B archive (0.3 GB free; a successful-looking cook left an archive without its 8.7 GB container). Pass2_Paragon and RC1–RC9 were deleted with approval (their commits remain); RC10–RC14 and the GAS packages are kept.

Found and fixed during A: released AI resumed a move task that waited forever for the move the lock stopped (now the tree restarts on release); targeting counted locked time as failed pursuit; the stall watch counted locked enemies; Blueprint-internal calls cannot be refused from C++ (input lock covers the human, C++ callers are refused).

### RC14 one perfect-dodge rule (`e0f794f`), editor and packaged RC14

No GAS feature was added (Frozen / Stun stay on hold); no enemy damage, dodge animation, movement, parry or attack timing changed. Package `B:\Unreal\GOTA_Builds\RC14_e0f794f` (fresh cook). Detail: `GOTA_HORDE_COOP_COMBAT.md` "RC14"; matrix rows R14-1 to R14-10.

- **Approved rule (2026-11-17):** host and remote players get the same server-authoritative perfect-dodge protection, 0.35 s window, through the qualifying attack's impact + 0.15 s, for that impact only. Implemented as a per-impact guard at the attacker's damage notify (`UGOTAMontageReplicator` → `UGOTADuelFighterComponent::TryGuardDodgedImpact`), replacing RC13's remote-only blanket window. The qualifying attack is whichever is about to land (any enemy in reach), judged by the montage's damage notify.
- **Results (bots):** packaged armed dodges host 37 / remote 81, none hit by the dodged attack; 106 guarded impacts, 0 landed; too-early dodges unprotected; follow-up swings and other enemies still land. Suite 46/46 packaged first pass; Victory flow, Story, Duel, Rampage, menu, leave all pass. Best legitimate bot match: wave 4.
- **Remaining asymmetry, not changed:** the Blueprint dodge's own 0.20 s i-frames are on the server only for the host (all attackers).

**Release blockers after RC14:** (1) human playtest; (2) a legitimate five-wave victory; (3) quiet-machine render profile; (4) separate PCs.

### RC13 human combat validation (`a941f8c`, `04b69f8`), editor and packaged RC13

No GAS feature was added (Frozen / Stun stay on hold); no enemy damage or difficulty was changed. Package `B:\Unreal\GOTA_Builds\RC13_a941f8c` (fresh cook). Horde detail: `GOTA_HORDE_COOP_COMBAT.md` "RC13"; matrix rows R13-1 to R13-13. **All results are bot evidence; no human has played RC13.**

| Area | Change | Commit |
|---|---|---|
| Remote perfect dodges against regular enemies were hit anyway (35 of 75) | server sets `IsInvincible` from an accepted armed dodge until impact + 0.15 s, never while Veiled; after: 80/80 editor, 67/68 packaged | `a941f8c` |
| Telemetry | `TELEM dodge`, `perfect_dodge`, per-wave per-player line (damage taken / dealt, blocks, parries, dodges, deaths, kills, first death, avoided) | `a941f8c` |
| Test harness | `waitplayers`, listen-based client launch, 75 s host tail, INCONCLUSIVE for starved clients, `--mp-only`, `--repeat`, failure archive | `a941f8c`, `04b69f8` |
| Intermission healing | recorded as the approved rule (2026-11-17), revisit after human balance testing | `04b69f8` |

**Suite:** packaged 45/46 with one INCONCLUSIVE (TitanCharge, starved client). The RC12 GaleRush / GravityWell failures are explained by starved clients (33/33 over three multiplayer passes with a client getting frames); the RC12 first pass is still recorded as 44/46, not 46/46.

**Profile (rendered client, waves 1–2, 60 s):** frame 17.2 ms, render thread 17.0 (bottleneck), game thread 10.5, RHI 5.6, GPU 5.2, 1.66 GB, 9.5 / 4.0 KB/s down / up. Insights warnings in the analysis log are trace-analysis artifacts, not game faults; memory tracing is not needed.

**Release blockers after RC13:** (1) human playtest (kit ready for RC13); (2) a legitimate five-wave victory (bots lose wave 2); (3) separate PCs; (4) render-thread cost (17 ms with four heroes and a wave on screen, measured on a loaded machine).

### RC12 enemy combat intelligence and Horde playability (`336b6cb`, `10f4bb2`, `05704ae`), editor and packaged RC12c

No GAS feature was added (Frozen / Stun stay on hold). No enemy damage, health or count was changed. Horde detail: `GOTA_HORDE_COOP_COMBAT.md` "RC12".

**Packages:** three fresh cooks (each 3,937 packages, 0 incrementally skipped). **RC12c (`B:\Unreal\GOTA_Builds\RC12c_05704ae`) is the release candidate.** RC12 (`336b6cb`) and RC12b (`10f4bb2`) are kept and must not be used: RC12's host crashed on the first wave-1 target, RC12b's Horde boss froze invincible. RC1–RC11 not touched.

**Fixes (all C++ except the spin damage track):**

| Area | Fix | Commit |
|---|---|---|
| Spin attack | 9 damage notifies → 4 (Slash 1.224 s, AOE 2.0 / 3.25 / 4.5 s); ≤ 40 per target; cooked montage verified in the package | `336b6cb` |
| Enemy attack animations on remote clients | `UGOTAMontageReplicator` multicasts AI montages (template played them on the server only) | `336b6cb` |
| Dead enemies kept attacking | defeated AI stops brain, movement, montages; cannot start an attack | `336b6cb` |
| Horde heroes unarmed, blocks impossible | server draws the weapon via the hero's `SR_MeleeStance` | `336b6cb` |
| Enemies idle out of sight range, ranged move failures | `UGOTAEnemyTargeting` (acquire / reacquire / leash / pursuit failure / path-failure recovery / return home) | `336b6cb` |
| Survivors carried damage into later waves | `bRestoreSurvivorsDuringIntermission` — **approved 2026-11-17** (current Horde rule; revisit after human balance testing) | `336b6cb` |
| Story clients dropped movement corrections | deterministic, net-addressable graybox component names | `336b6cb` |
| Horde not reachable from the public menu | lobby mode button (created at runtime; the layout had no slot) | `336b6cb`, `10f4bb2` |
| **Packaged host crash** (`FGOTABlueprintCombat::Call` from targeting) | every reflection parameter buffer is the function's full structure size; `InitializeStruct` wrote Blueprint locals past a `ParmsSize` stack buffer (latent since the helpers were written; also affected `ApplyDamage`) | `10f4bb2` |
| **Horde boss froze, invincible** (packaged only) | server keeps a hard reference to the soft `UGOTABossDefinition`; garbage collection unloaded it mid-fight. Reproduced on demand with `obj gc` in RC12b; gone in RC12c | `05704ae` |

**Packaged RC12c regression** (matrix rows R12-1 to R12-15): suite 46/46 after one rerun (first pass 44/46, two client-side multiplayer checks, both pass alone); four-player Story with shrine guard and 0 dropped movement corrections; Horde through the public lobby; full match to Victory with the healing harness and 1,063 s of clean post-victory rendering; legitimate match lost in wave 2 at 9/11 kills; Duel rematch; Drakon versus Rampage; client and host leave.

**Not verified in RC12:** a legitimate five-wave victory (bots lose wave 2), human players, perfect dodges against regular enemies, a clean-machine performance profile, separate PCs.

**Release blockers after RC12:** (1) human playtest (kit ready, RC12c); (2) ~~sign-off on the intermission restore rule~~ approved 2026-11-17; (3) a legitimate full victory, by humans or a stronger test controller; (4) clean-machine profile (RC12c client 17.5 ms frame on a loaded machine, render-thread bound).

### RC11 real combat validation (`a8e6e87`), editor and packaged RC11

No GAS feature was added, no enemy number or behaviour was changed, and the fight driver was not made stronger. Horde detail: `GOTA_HORDE_COOP_COMBAT.md` "RC11". Human playtest kit: `GOTA_HORDE_PLAYTEST.md`.

**Package:** fresh `RunUAT BuildCookRun` Win64 Development from `a8e6e87` (`Saved/Cooked` and `Saved/StagedBuilds` removed first), archive `B:\Unreal\GOTA_Builds\RC11_a8e6e87`. Cook: 3,937 packages, **0 incrementally skipped**, `BUILD SUCCESSFUL`. The game binary was linked from the committed source (its unity object was compiled before any later edit). RC1–RC10 not touched. Test scripts copied into the archive's `WarOfGods\Tools\Tests` (as for RC9/RC10; without them the packaged host runs no script).

**What RC11 contains, and how it was checked in the package** (all fixes are C++ or ini; no Blueprint or behavior-tree asset changed, so each is proved by its runtime log line):

| Fix | Packaged evidence |
|---|---|
| `r.SkeletalMesh.UseCachedMDCs=0` | `Set CVar [[r.SkeletalMesh.UseCachedMDCs:0]]` on host and clients |
| Horde wave-enemy controller (`f423fad`) | `Horde spawned … controller=AIC_Enemy_Base_C_…` |
| Story shrine guard (`74c480d`) | `float attack guard attached`; `float attack ended without StopFloat (sequence aborted, now Wait) after 6.9s; landing (called=1)`; Burning then damages the boss `700 -> 698 -> 696 -> 694` |
| Ready-up (`a8e6e87`) | `Horde readiness: all 4 ready after 34.0s -> countdown` |
| Telemetry (`a8e6e87`) | `TELEM` lines; reports below |

**New in `a8e6e87`:**

- **Horde ready-up.** Wave 1 waits until every expected player has loaded, received its pawn and has a hero (server-decided), then the usual 5 s countdown; 60 s timeout; leavers are dropped from the count. Replicated to clients and shown on the HUD.
- **Horde end phase.** `HordePhase` now reaches Victory / Defeat (it used to stay `BossActive` / `WaveActive`).
- **Combat telemetry** (`UGOTACombatTelemetry`, `gota.Telemetry`, development builds only, compiled out of Shipping): one line per enemy attack, hit on a player, death, heal, spawn, enemy death, boss attack, movement failure, stall, wave and match end. Event-driven plus a 1 Hz sample; nothing per frame. `UStatsComponent` pairs each Blueprint damage response with the health drop it caused, so attacker and damage per hit are exact.
- Test harness only: `gota.Net.Sustain` (heals players so bots survive long enough to exercise enemy behaviour; never balance evidence), `GOTA_MPSession.py` records every process's exit code and whether the harness killed it (`<tag>_exits.txt`), `Tools/GOTA_HordeTelemetry.py`, `Tools/GOTA_PerfCsv.py`.

#### Packaged regression (RC11)

| Check | Result |
|---|---|
| Regression suite (statuses, Burning, boss immunity and resistance, multiplayer cases) | **46/46**, first pass |
| Four-player Story, shrine boss, Burning on it | **pass** (`rc11p_story_float`) |
| Four-player Horde with active enemies, kill credit, Burning on enemies | **pass** (`rc11p_aival_A`, healing harness) |
| Natural team wipe | **pass** (`rc11p_ready_wipe2`, one Defeat) |
| Duel PvP result and rematch | **pass** (`rc11p_duel_rematch`) |
| Drakon versus Rampage | **pass** (matches RC9) |
| Host / Find / Join into Horde, client leave, host leave | **pass** (`rc11p_menu_horde`, `rc11p_leave`, `rc11p_hostleave`) |
| Five-wave Horde victory with fighting enemies, Horde boss with a controller | **not run** (bots lose wave 1) |
| Human playtest | **not run** |

#### Renderer cost of `r.SkeletalMesh.UseCachedMDCs=0` (packaged, rendered client, 120 s CSV profile in waves 1–2)

| | Caching off (as shipped, `rc11p_aival_A`) | Caching on (`-dpcvars=…=1`, `rc11p_aival_B`) | Difference |
|---|---|---|---|
| Frame | 15.40 ms (p95 18.1) | 14.00 ms (p95 16.2) | +1.4 ms |
| Render thread | 15.37 ms (p95 18.0) | 13.98 ms (p95 15.9) | **+1.4 ms (~10%)** |
| Game thread | 8.38 ms | 7.27 ms | +1.1 ms (waits on the render thread) |
| RHI thread | 5.46 ms | 5.30 ms | +0.2 ms |
| GPU | 5.14 ms | 4.75 ms | +0.4 ms |
| Physical memory used | 1,496 MB (peak 1,742) | 1,682 MB (peak 1,868) | not attributable (run-to-run) |
| Frames over 100 ms | 1 (734 ms) | 1 (814 ms) | same one-off hitch in both |

The client is render-thread bound in both. One run each, and the fights were not identical (wave 1 lasted 118 s and 245 s), so the cost is an estimate: **about 1.4 ms of render thread with four heroes and a wave 1–2 fight on screen**. It grows with the number of visible skinned meshes. Acceptable for RC11; before release, either keep it and budget for it, or find the stale uniform buffer in the skinned-mesh proxy and turn caching back on.

#### Story client "log stopped" (RC10 open item): explained

`story_float_mp` was launched with `--kill-client 2 --kill-client-at 45` (a disconnect-during-attack test). The harness hard-killed client 2 45 s after the last client launched: client 3 started at 01:45:20–22, client 2's log ends at 01:46:05.7, the host logged a normal connection close at 01:46:05.8, and there is no crash report. **It was the test harness, not a crash, a timeout, the shrine or the renderer.** The RC10 record called it unexplained; that was wrong. The harness now writes each process's exit code and whether it killed it.

Separate finding from the same logs: every Story client (not only client 2) logs `ClientAdjustPosition_Implementation could not resolve the new relative movement base actor, ignoring server correction!` hundreds of times (1,409 and 1,789 on the other two clients; 0 in every Horde log). Cause: `AArenaGrayboxBuilder` builds the Story floors as runtime `UStaticMeshComponent`s with generated names on both server and client, so the server's movement-base reference cannot be resolved on the client and those position corrections are dropped. Risk: client position drift / rubber-banding in Story. Proposed fix (not made in RC11): give built components stable, deterministic names and make them net-addressable, or build the Story floor as level geometry.

### RC10 stabilization (`74c480d`, `f423fad`, `b619ee5`), editor and packaged RC10

No GAS feature was added in this pass, and no enemy difficulty value or fight-driver aggression was changed to produce a result.

**Packaged build:** fresh `RunUAT BuildCookRun` Win64 Development, archive `B:\Unreal\GOTA_Builds\RC10_2454bff_skelmdc`: `2454bff` plus the `r.SkeletalMesh.UseCachedMDCs=0` setting (later committed as `b619ee5`). Cook: 3,937 packages, **0 incrementally skipped**. RC1–RC9 not touched. **RC10 does not contain the shrine guard (`74c480d`) or the Horde controller fix (`f423fad`)**: in both RC10 victories wave enemies were still passive and only the boss fought (0 `Horde spawned ... controller=` lines, 0 float-guard lines in the host logs). Those two fixes are verified in the editor only. Multiplayer clients: 120 ± 30 ms, 5% loss. One machine.

#### P0: rendered client crash after a Horde victory (fixed, provisionally validated)

**Symptom.** One to three minutes after a legitimate victory the rendered client crashed on a parallel render worker in the base pass: packaged `EXCEPTION_ACCESS_VIOLATION reading 0xffffffffffffffff` in `FD3D12DescriptorCache::SetSamplerTable` <- `RHISetShaderParameters`; editor `Assertion failed: Resource [D3D12CommandContext.h:439]`, or access violations at `0x48`.

**Root cause.** With RHI validation (`-rhivalidation`) and GPU breadcrumbs (`-gpucrashdebugging`) on the rendered client, the first error before the crash was `Non MultiFrame Uniform buffer has been allocated in a previous frame` while drawing the base pass in parallel, followed by layout and size mismatches on view uniform buffers (`FVirtualShadowMapUniformParameters`, `FBlueNoise`, `FHairStrandsViewUniformParameters`, `LayoutConstantBufferSize`). A **cached mesh draw command for a skinned mesh** kept a single-frame uniform buffer that had already been released; when its memory was reused the draw bound garbage. A real boss fight triggers it; a forced clear does not.

**Controlled runs** (editor, four players, rendered client Drakon; "quick" = fight the boss wave only, then hold the results screen):

| Configuration on the rendered client | Runs | Result |
|---|---|---|
| Quick, boss removed with `gota.Net.KillEnemy` (no fight) | 3 | no crash |
| Quick, boss fought and killed (default renderer) | 7 | **crash 7/7**, 75–175 s after the kill |
| … with the client camera idle (nothing to do on the client) | incl. above | still crashed |
| … with `r.GPUSkin.BoneTransformAllocationMode=1` (the RC9 cloth setting reverted) | incl. above | still crashed: the RC9 setting is not involved |
| … with four Drakon players (no cloth heroes at all) | incl. above | still crashed: cloth is not involved |
| … with Burning off | earlier (`horde_full_noburn`) | still crashed: Burning is not involved |
| All cached mesh draw commands off (`r.MeshDrawCommands.UseCachedCommands=0`) | 1 | no crash |
| Skeletal meshes hidden (`ShowFlag.SkeletalMeshes 0`) | 1 | no crash |
| Only skeletal-mesh draw caching off (`r.SkeletalMesh.UseCachedMDCs=0`) | 1 | **no crash, 195 s held** |

`gota.Test.SceneWatch` on the rendered client showed Rampage's mesh gone from the client within 3 s of the victory, so the crash is not a dangling boss body; it is the stale cached command.

**Fix (`b619ee5`).** `r.SkeletalMesh.UseCachedMDCs=0` in `Config/DefaultEngine.ini` `[SystemSettings]`. Skinned meshes build their draw commands each frame; static meshes still use cached commands. Confirmed active on every RC10 peer (`Set CVar [[r.SkeletalMesh.UseCachedMDCs:0]]`). No gameplay, effect or results-screen timing was changed; character rendering is unchanged.

**Before and after:**

| | Fought boss kills | Rendered client crashed |
|---|---|---|
| Before (quick diagnostics, editor) | 7 | **7** |
| Before (full five-wave victories: `horde_full_e1`, `e2`, `noburn` editor; `rc9_horde_full` packaged) | 4 | **4** |
| After, quick test (editor, `diag_bossfight_skelmdc0`) | 1 | 0, held 195 s after the kill |
| After, packaged RC10 `rc10_accept1` | 1 | 0: Victory `score=13575`, 62 kill credits, 0 ensures; clients up **900, 902, 910 s** after victory, 0 `LogRHI: Error` |
| After, packaged RC10 `rc10_accept2` | 1 | 0: Victory, 62 kill credits, 0 ensures; clients up **823, 824, 832 s** after victory, 0 `LogRHI: Error` |
| `rc10_accept3` | – | **cancelled by the user** before the victory (6 kills); counts as neither pass nor fail |
| Runs 4 and 5 | – | **not run** |

**Acceptance decision.** The target was five legitimate packaged victories. It was reduced to two full packaged victories plus one quick test, on the evidence: before the change 11 of 11 fought boss kills crashed within 175 s; after it 3 of 3 held at least 195 s, and both full matches held more than 13 minutes, over four times the latest crash ever seen. Accepted as **provisionally validated**. The next release run should still include more full packaged victories, now with fighting wave enemies (RC10 had passive ones).

**Cost (to be profiled).** Building skinned-mesh draw commands every frame costs render-thread CPU per visible skinned mesh. RC10 rendered client in `rc10_accept1`: average frame 20.15 ms, GPU 5.63 ms, peak memory 1,899 MB; host 9.64 ms. There is no same-match measurement with caching on (every such match crashed), so the cost is **not measured**. Profile with `stat SceneRendering` / Unreal Insights in a 30-enemy wave before release; if it matters, the alternative is to find and fix the stale uniform buffer in the skinned mesh proxy and turn caching back on.

#### P1: Horde wave enemies attack (fixed, editor)

- **Cause.** Enemy Blueprints keep the engine default `AutoPossessAI = PlacedInWorld`, and `AHordeEncounterDirector::TrySpawnOne` spawned them with a bare `SpawnActor`. Wave enemies had no controller: no perception, no behavior tree, no attacks (runtime: `brain none (controller None)`). This is why "Horde enemies do not engage" (RC8 stabilization below).
- **Fix (`f423fad`).** If the spawned pawn has no controller, the director calls `SpawnDefaultController()`, which uses the enemy's own `AIControllerClass` (`AIC_Enemy_Base`) as a placed enemy would. Behavior trees, attack tokens, damage and numbers are unchanged. It also applies to the Horde boss role, so the Quarry Behemoth now has a controller too; Horde boss behaviour with a controller is **not yet verified** (no match has reached wave 5 since).
- **Verified:** wave enemies spawn with `AIC_Enemy_Base_C_*` and a running tree (`horde_enemy_brain.txt`).
- **Natural defeat, standard match (`horde_natwipe_ai1`, editor, four players, nothing spawned or injected):** four players walk into wave 1 without attacking and are killed by the enemies' own attacks between 29 and 40 s after the wave starts. Each player is `ELIMINATED` once and the host logs exactly one `Horde match complete outcome=Defeat score=0`.
- **Full matches against fighting enemies** (`horde_ai_full1/2/3`, editor, fight driver unchanged except the block reaction below): the bots **lose wave 1 every time**, with 0 enemy kills. Player deaths after the wave starts: `full2` +16, +36, +45, +60 s; `full3` +24, +33, +33, +42 s. In `full3`, 23 of 24 bot ability hits landed, about 50–75 damage per 100-health enemy before the team died.
- Fight-driver change in this commit: `gota.Net.Fight` blocks when the nearest enemy within 400 cm is in an attack montage less than 0.35 s from its damage notify (as a player reacts to a wind-up). It does not make the bots stronger in any other way. Host blocks per match: 19; remote clients: 0 (the montage is not visible to the driver on clients; driver limitation).

#### P2: Story shrine boss always lands (fixed, editor)

- **Cause** (RC8 stabilization below): `BT_Enemy_Boss` runs `BTT_Boss_StartFloat` (raise and `IsInvincible`), AOE Trail, then `BTT_Boss_StopFloat`. If the sequence aborts between them (the attack kills its target), StopFloat never runs.
- **Fix (`74c480d`).** `UGOTAFloatAttackGuard`, a server-only component added at combat bootstrap to any pawn whose Blueprint has `StartFloat` and `StopFloat`. Every 0.1 s it notes the composite the active `BTT_Boss_StartFloat` runs in. If the active node leaves that composite, the brain stops, the boss dies, or it is destroyed before a `BTT_Boss_StopFloat` node runs, it calls the Blueprint's own `StopFloat`. The attack and its temporary invulnerability are unchanged; no behavior-tree asset was edited.
- **Verified (editor):**

| Case | Result |
|---|---|
| Target survives the attack | boss lands by itself; guard silent |
| Target dies mid-attack, solo | guard lands it; Burning ticks then damage it (`700 -> 694`) |
| Target dies mid-attack, four players (`story_float_mp`) | `float attack ended without StopFloat (sequence aborted, now Wait) after 7.1s; landing (called=1)` |
| Target changes or disconnects mid-attack | **not exercised** (the boss stayed passive after landing) |

- In `story_float_mp` remote client 2's log stops 37 s after the landing with no crash report, after 267 `ClientAdjustPosition ... could not resolve the new relative movement base actor` warnings (standing on the floating platform). Cause not established; the host saw an ordinary connection close. Watch in the next Story run.
- Observed in the editor: after a 3 s editor hitch the period timer caught up and three burn ticks fell within 0.17 s. The total is still three ticks.

#### Corrections to the RC8 stabilization record

- "after the client had nearly stopped rendering" was **wrong**. The frame number in a log line is `GFrameCounter % 1000`, and an idle client writes few lines, so "180 frames in 170 s" was misread. The client rendered normally until the crash.
- The cloth setting (`r.GPUSkin.BoneTransformAllocationMode=0`) is unrelated to the victory crash: reverting it did not change the crash, and four non-cloth heroes still crashed. It stays for the separate cloth assertion.
- "Horde enemies do not engage" was not tuning: they had no controller (P1).

#### Difficulty: human players versus the test bots

These are two separate questions, and the data answers only the second.

- **The bots cannot win wave 1.** They do not block on remote clients, swing melee at moving or blocking enemies (`BlockChance` 0.25) and stand in the pack. That is a test-tool limit, not evidence that the game is too hard.
- **Whether humans can win is unknown.** No human has played a Horde match with attacking enemies. The only real numbers: a team that does nothing dies 29–40 s into wave 1; a bot team that fights dies 16–60 s in. Every earlier victory (RC1–RC10) was against passive wave enemies and says nothing about balance.
- **Structural concerns worth checking in a playtest:**
  - Player-count scaling raises enemy count and concurrency (`ScaledCount`, `MaxConcurrentPerExtraPlayer=2`) but not enemy damage. Four players face more enemies at once, each hitting as hard as for one.
  - In the bot runs, wave 1 started with only the host connected (preparation is about 6 s; clients joined 8–26 s later). The first player faces the opening alone. A real lobby starts everyone together, but a late joiner meets this.
  - There is no log of damage per enemy hit, so time-to-kill cannot be measured from logs.
- **Recommended before any tuning:** (1) a human playtest of waves 1–2 with 2–4 people; (2) a development log of each enemy hit on a player (attacker class, damage, health after) and per-wave player deaths, to measure time-to-kill; (3) decide on wave-start rules for late joiners. Change numbers only after that.

**Next gameplay blocker:** Horde balance and readability now that wave enemies attack. A standard match can now be lost, but no one has shown it can be won against fighting enemies, and the Horde boss with a controller has not been seen.

### RC8 stabilization (`839f397`), editor and packaged RC9

**Packaged build:** fresh `RunUAT BuildCookRun` Win64 Development from `839f397` (`Saved/Cooked` and `Saved/StagedBuilds` removed first), archive `B:\Unreal\GOTA_Builds\RC9_839f397` (inner exe 346,712,576 bytes). Cook: 3,937 packages, **0 incrementally skipped**, no cook errors. RC1–RC8 not touched. Multiplayer clients: 120 ± 30 ms, 5% loss. One machine.

No GAS feature was added in this pass. Code changes: one engine setting (`r.GPUSkin.BoneTransformAllocationMode=0`) and development-only test commands.

#### Results

| Check | Result |
|---|---|
| Packaged regression suite (all 46, after the `burn_boss.txt` correction) | **46/46**, first pass |
| Four-player Horde with Burning, packaged, rendered remote client | **pass**: 4 players; tick 1 credited to p0, ticks 2–4 to p1 after re-application, `100 -> 98 -> 96 -> 94 -> 92`; kill by p1's burn credited to p1; 34 ticks, 12 ignitions, 3 refreshes; rendered client 12 flames on, 11 off (the last was lit 1.4 s before the host quit, on an enemy still burning); no assertion on any client |
| Four-player Story with Burning, packaged, rendered remote client | **pass**: Veiled blocks two ticks, Unbreakable refunds `1.0 of 2.0` three times, death clears the burn and the player respawns clean; rendered client 5 flames on and 5 off; no assertion |
| Drakon versus Rampage, packaged | **pass**: 4 perfect parries, 6 perfect dodges, 5 Divine Counters (90 damage, 2.6 s stun), Phase II, boss defeated |
| Five-wave Horde victory, editor, unforced | **pass on the server, 3 of 3**: `horde_full_e1` (`score=13575`; 62 kill credits, 107 Molten Strike hits, 81 ignitions, 233 burn ticks; boss killed by remote Kairos), `horde_full_e2` (`13875`, with the director fix), `horde_full_noburn` (`13575`, Burning off). Exactly one `Horde match complete outcome=Victory` each, 0 player deaths, nothing injected. **The rendered client crashed after every one** (see "Crashes at a real victory"). |
| Five-wave Horde victory, packaged RC9, unforced | **pass on the server, crash on the rendered client**: waves 1–5 and the boss, exactly one `Horde match complete outcome=Victory score=13575`; 62 kill credits, 113 Molten Strike hits, 92 ignitions, 259 burn ticks, 0 player deaths, boss killed by a remote player. The host hit the director ensure below at that moment, and the rendered client crashed about two minutes later (see "Crashes at a real victory"). |
| Natural team-wipe defeat in a standard match | **not achievable today**: see "Horde enemies do not engage" below |

#### Cloth crash (`GPUSkinVertexFactory.cpp:1348`)

- **Occurrences:** 2 in about 350 logs that created cloth hero bodies: `rc6fight2` client 2 (2026-11-07, before any GAS work) and `pkg_burn_story` client 1. Both on a rendered client within 2 s and 15 s of joining, within 0.1 s of a cloth body (Gideon, Countess) being created on a remote pawn, on the same render path (`SingleLayerWater` pass -> `UpdateSceneExtensionHeader` -> cloth uniform buffer). Not related to GAS or Burning.
- **Cause, from the engine source:** in the default `r.GPUSkin.BoneTransformAllocationMode=1` (Scene), a parallel RHI task rebuilds a cloth mesh's uniform buffer (`UpdateSceneExtensionHeader`). On the mesh's second simulated frame the render thread's `UpdateClothSimulationData` flips `CurrentBuffer` (line 1234) before allocating that slot (line 1243). A reader in that window finds the current slot empty and trips `check(bPrevious)` (line 1348). `HasClothBufferForReading` computes the index before testing `bEnabled`, so the check is reached even for a disabled section.
- **Change:** `r.GPUSkin.BoneTransformAllocationMode=0` in `Config/DefaultEngine.ini` `[SystemSettings]`. In vertex-factory mode `UpdateSceneExtensionHeader` is never called (guarded at line 2087, `check`ed at 2098), and the cloth uniform buffer is rebuilt on the render thread after the cloth write. Confirmed active in packaged RC9 (`Set CVar [[r.GPUSkin.BoneTransformAllocationMode:0]]`).
- **Evidence level:** this removes the crashing call path by construction. The crash was **not** reproduced on demand: about 3,500 cloth-body rebuilds under the default setting (one rendered process, then three at once; `gota.Test.RebuildBodies`, `cloth_stress.txt`) gave no assertion, so a before/after comparison is not possible. Both real crashes happened during a client's join. Cloth heroes render correctly with the new setting (`cloth_mode0_nyx.png`). No frame-time cost measured (RC9 baseline 16.5 ms against RC8's 18.1 ms, within noise).

#### Crashes at a real victory

Both legitimate victories (editor `horde_full_e1`, packaged `rc9_horde_full`) ended the same way:

- **Host: a real bug in project code, fixed.** `Ensure condition failed: Array has changed during ranged-for iteration!` in `AHordeEncounterDirector::TickAliveScrub`. The scrub loops over `TrackedEnemies` and calls `NotifyEnemyDefeated`. On the last enemy that broadcasts `OnWaveCleared`; the game mode then calls `CompleteVictory` (or `ConfigureWave` for the next wave), and both call `CleanupTrackedEnemies`, which runs `TrackedEnemies.Reset()` inside the loop. The loop then walked a reset array. Present since before the GAS work; it shows only at a real victory because only that path ends the match from inside the scrub. Reporting the ensure also froze the host for 6 s, which is why every client logged "Hit limit of 96 saved moves" at victory. Fix: both loops that can trigger it (`TickAliveScrub`, `HandleEnemyEndPlay`) now go by index and re-read `Num()` each pass.
- **Rendered client: renderer crash** (root cause and fix in RC10 above; the "nearly stopped rendering" remark below is corrected there). Packaged: `EXCEPTION_ACCESS_VIOLATION reading 0xffffffffffffffff` in `FD3D12DescriptorCache::SetSamplerTable` <- `FD3D12CommandContext::RHISetShaderParameters`, on a parallel render task in the base pass (symbolized from the minidump with the build's PDB). Editor: `Assertion failed: Resource [D3D12CommandContext.h:439]`, also on a parallel render worker. In both, a draw referenced a GPU resource that no longer existed, one to three minutes after the victory, after the client had nearly stopped rendering.
- A forced victory (`diag_postvictory2`, ForceClear, diagnostic only) held for four minutes did **not** crash. A real victory differs in that the boss dies in combat and `CompleteVictory` destroys every remaining enemy in the same tick, inside the host's broken loop.
- **It does not come from the host bug, and not from Burning.** Two more full unforced matches with the director fix (editor):
  - `horde_full_e2`: Victory `score=13875`, **no ensure**; the rendered client still crashed about two minutes later (access violation reading `0xffffffffffffffff`).
  - `horde_full_noburn`: the same match with `gota.GAS.ElementStatus 0` (0 ignitions, 0 burn ticks). Victory `score=13575`, no ensure; the rendered client still crashed 75 s later (access violation reading `0x48`).
- In every case all flame components created on that client had been removed before the crash (created minus removed = 0).
- So: **4 of 4 legitimate victories** crashed the rendered client one to three minutes later, with or without Burning; 0 of 1 forced victories did. The `-nullrhi` clients never crashed. Cause unknown; the next step is a packaged run with the D3D12 debug layer and GPU crash breadcrumbs on the rendered client, and a look at what a real boss kill presents on clients (death presentation, then `CompleteVictory` destroying the boss pawn) that a forced clear does not.

#### Story shrine enemy (`BP_Enemy_Boss` in `L_GOTA_Story_Prologue`)

- It refuses **all** damage, not only Burning: a 99,999-point hit through the same Blueprint path returned `landed=0`, as did every burn tick and Molten Strike.
- Cause, sampled every 5 s (`story_shrine_watch.txt`, `gota.Net.InspectDamage` now prints values, the actor's own state and its behavior-tree task): at 0 s it is vulnerable on the ground (Z 1690). At 5 s `BT_Enemy_Boss` runs **AOE Trail**, whose `BTT_Boss_StartFloat` raises it on `BP_FloatingPlatform` (Z 2050) and sets `IsInvincible` (the Blueprint's `StartFloat` event). At 8 s that attack kills the player, the tree switches to the passive Wait subtree, and `BTT_Boss_StopFloat` never runs. It stays in the air and invincible for the rest of the session (65 s observed).
- Invincibility while floating is intended. Staying afloat after the branch aborts is a content bug in the boss behavior tree: no `StopFloat` on abort. **Not changed** here; fixed in RC10 by `UGOTAFloatAttackGuard` (`74c480d`).
- The same class was defeated in an early Horde run (`rc2hordewin`), so it is not invulnerable by design.

#### Horde enemies do not engage

- Players who stand 70 cm from a wave-1 enemy, with line of sight, for 20 s or more are never attacked (`horde_wipe_e1`/`e2`). Provoking with one melee swing per enemy (it misses: the enemies stand 53 cm higher on the spawn step) or one ability hit per enemy (6 hits landed, `horde_wipe_e4`) did not change that.
- Across every Horde host log on disk (back to 2026-11-01), every enemy-caused player death came from enemies added with `gota.Net.SpawnEnemies`; wave-director enemies have killed no one. In the full victory, 0 players died.
- This predates the GAS work. A natural four-player defeat in a standard match therefore cannot currently be produced; the one defeat observed (`pkg_burn_perf_base`, one `outcome=Defeat`) had 20 extra spawned enemies. Not fixed here; RC10 found the cause (no AI controller) and fixed it in `f423fad`.

#### Test-driver change

`gota.Net.Fight` now leaves a target it has not closed on for 6 s (ledge, no line of sight) for 12 s and takes the next nearest. The earlier attempt stalled in wave 5 for this reason: the host strafed 172 times against enemies 53 cm above it. Hits are still real melee swings and abilities.

### Burning milestone (`548a433`), editor and packaged RC8

**Packaged build:** fresh `RunUAT BuildCookRun` Win64 Development from `548a433` (same command line as RC7; `Saved/Cooked` and `Saved/StagedBuilds` removed first), archive `B:\Unreal\GOTA_Builds\RC8_548a433` (inner exe 346,700,800 bytes). Cook log `RC8_548a433_cook.log`: 3,937 packages cooked, **0 incrementally skipped**, 16 skipped by platform, no cook errors. RC1–RC7 not touched. One machine; no Shipping build; no second PC.

Multiplayer clients used `NetEmulation.PktLag 120`, `PktLagVariance 30`, `PktLoss 5` (pings 131–166 ms). Damage is proven from the server log: every tick line carries the victim's health before and after.

#### Regression suite (`Tools/GOTA_AbilityRegression.py`)

The suite has 46 cases: the 40 that existed at `bb69d11` (30 single-player, 10 listen-server + client; the earlier "42" in this document was a miscount) and 6 new Burning cases.

| Run | Result |
|---|---|
| Editor, final binary | **46/46**, first pass (run twice on successive binaries, both 46/46) |
| Packaged RC8 | **45/46** on the first pass; the Burning group re-run alone after a test fix: **6/6** |

The packaged failure was the boss Burning case. Its second Molten Strike missed the boss at 250 cm (`MoltenStrike missed nearest=... horiz=250 los=1`): the script cast 0.2 s after `gota.Duel.Face`, before the facing had applied at packaged frame rates, so the immunity step had no hit to refuse. No Burning code ran in the failing step. The script now faces twice over 0.8 s. The other 40 cases were not re-run after that script change.

Burning cases and what each proves:

| Case | Key evidence |
|---|---|
| Molten Strike ignites | Impact 22.0 exactly once. `Burning applied: 2.0 per tick every 1.00s for 3.0s`. Three ticks, `238 -> 236 -> 234 -> 232`, none on application, no fourth; still 232 two seconds later. Momentum stays at 5 (3 hit landed + 2 ability hit from the strike; ticks add none). One flame on, one off. |
| Refresh, repeated cycles | Burning at t = 0 and again at 1.5 s: one effect, one flame, 4 ticks, 260 -> 252. Then four apply/expire cycles: 12 ticks and 24.0 damage in total, 5 flames on and 5 off, nothing left. |
| Statuses and damage rules | Burning + Dread + Slowed listed together, walk x0.60. Under Veiled two ticks leave health unchanged and the third lands after the veil. Under Unbreakable `absorbed 1.0 of 2.0` three times. Dread on Drakon: impact 18.1, ticks still 2.0. A burning Drakon casts Molten Strike through a tick: no `cancelled`. 18 ticks, 32.0 damage. |
| Death by burning | Champion at 3 hp: tick to 1, tick kills. `defeated (by BP_Player...)`, `cleared 1 status effect(s) (death)`, flame removed, 2 ticks in total. |
| Boss | Quarry Behemoth burns by default (3 ticks). With `GOTA.Immunity.Burn`: 22 still lands, `status Burning resisted (whole burn )`, "THE QUARRY BEHEMOTH RESISTS BURNING". With a 50% resistance: 1.5 s at 1.0 a tick, one tick, "... SHRUGS OFF PART OF BURNING". |
| Listen server + remote client | Host Drakon ignites the remote player: 3 ticks on the server. The client logs the cue, one flame on and off, and its own `FEEDBACK STATUS BURNING on` / `off`. No tick line on the client. |

#### Four-player Horde (`mp_burn_horde_*`; host Drakon, remote Drakon, Kairos, Nyx; one rendered remote client)

| | Editor (`burn_horde_f`) | Packaged (`pkg_burn_horde`) |
|---|---|---|
| Players | 4 | 4 |
| Ownership: p0 ignites, p1 re-applies 1.2 s later | Tick 1 credited to p0, ticks 2–4 to p1, `100 -> 98 -> 96 -> 94 -> 92` | same |
| Kill by a remote player's burn | `defeated (by <p1 pawn>)`, `Horde kill credit: ... +100` to that player | same |
| Molten Strike hits (both Drakons, real casts) | 5 | 6 |
| Burning applied / refreshed / ticks | 11 / 3 / 33 | 12 / 3 / 35 |
| Rendered remote client: flames on / off | 11 / 11 | 12 / 12 |
| Removal cues on each of the three clients | 11 | 12 |
| Remote players set alight (p1, p2) | each client logs its own BURNING line on and off once; p3 never does | same |
| Ability systems | 4 at start, one more per enemy first ignited, gone with the enemy | same |
| `HORDE STALL`, assertion, crash | none | none |

The first editor run (`burn_horde`, before the performance fix) gave the same ownership and cleanup results.

Rendered evidence: `Saved/Screenshots/GOTA/burn_visual_2_burning.png` (flames on the champion and the BURNING line, editor) and `mp_burn_all_2.png` (a burning enemy seen from the rendered remote client, editor). Under `-nullrhi` no flame component is created, so `flames=0` on those processes while `burning` is non-zero.

#### Four-player Story (`mp_burn_story_*`, packaged)

Statuses are applied with `gota.Net.Status`, as in the earlier Story runs (the fight driver cannot reach the shrine enemies).

- **Run 1 (`pkg_burn_story`): incomplete.** The rendered remote client crashed 15 s after joining, 0.1 s after the remote Nyx body (Countess, a cloth mesh) was created on it: `Assertion failed: bPrevious [GPUSkinVertexFactory.cpp:1348]` in the engine's cloth vertex factory, on a render worker thread. That was about 20 s before the first Burning was applied and the client's log has no Burning cue. The server-side checks in that run still matched run 2. The same client rendered the same bodies in both Horde runs without this assertion; it has been seen once.
- **Run 2 (`pkg_burn_story2`): pass.** 4 players throughout. Ownership hand-off on the shrine enemy; a remote player burning beside Slowed; under Veiled two ticks leave 100/100 and the third lands; under Unbreakable `absorbed 1.0 of 2.0` three times; a remote player killed while burning: `cleared 1 status effect(s) (death)`, respawned as a new pawn with no status. Rendered client: 5 flames on, 5 off. 14 ticks.
- The shrine enemy (`BP_Enemy_Boss`) took no damage from its burn: every tick logged `landed=0`, `700 -> 700`. Its damage system refused the damage. Whether it refuses direct hits in that state too was not checked.

#### Duel and Drakon versus Rampage (packaged)

- Suite groups on RC8: every Duel ability case and its listen-server case, Dread 5/5, Veiled 3/3, Unbreakable 3/3, Boss 3/3 (policy, `slice_mechanics` with 2 Divine Counters at 90 damage and the 2.6 s stun, boss death).
- Full fight (`slice_drakon_rampage.txt`, bot in fight mode): boss defeated, Phase II at 49%, 4 perfect parries, 7 perfect dodges, 5 Divine Counters (`90 damage (landed=1)`, each `STUNNED ... for 2.6s`). The bot does not cast abilities, so nothing burned; the five Divine Counters, which play Molten Strike's animation, ignited nothing, as intended.

#### Horde performance with Burning (four players, 20 extra enemies, everyone fighting, host rendering, clients `-nullrhi`, 30 s window)

The Burning run re-ignites every living enemy every 2.5 s while both Drakons ignite with Molten Strike; the baseline is the same script with `gota.GAS.ElementStatus 0` and no burns.

| | Editor baseline | Editor Burning | Packaged baseline | Packaged Burning |
|---|---|---|---|---|
| Concurrent Burning effects (peak) | 0 | 24 | 0 | 24 |
| Ability systems on the host | 6 | 28 | 4 | 28 |
| Burn ticks (damage events) / damage | 0 | 668 / 1,336 | 0 | 687 / 1,374 |
| Refreshes | 0 | 248 | 0 | 251 |
| Host frame, average | 18.2 ms | 24.9 ms | 18.1 ms | 20.5 ms |
| Host frame, worst | 67.7 ms | 65.6 ms | 49.3 ms | 53.7 ms |
| Frames over 100 ms | 0 | 0 | 0 | 0 |
| GPU, average | 5.0 ms | 6.1 ms | 4.8 ms | 6.4 ms |
| Host upload in the window | 713 KB | 696 KB | 524 KB | 854 KB |
| Host download in the window | 371 KB | 352 KB | 356 KB | 372 KB |
| Real ability casts / hits | 33 / 38 | 24 / 25 | 21 / 17 | 23 / 19 |
| Kill credits | 3 | 3 | 2 | 2 |
| Flames left at the end | 0 | 0 | 0 | 1 (one enemy still burning when the script quit) |

- The Blueprint damage call itself costs about 0.1 ms per tick (median 0.10 ms over 733 ticks in the editor).
- Twenty-four enemies burning at once cost 2.4 ms (packaged) to 6.7 ms (editor) of host frame time and about 1.5 ms of GPU. The split between the extra ability systems, the 24 particle emitters and replication was not measured.
- The packaged upload figures are not a clean comparison: in the packaged baseline all four players had died 12 s before the window closed, so less was being replicated. The editor pair, where the fight lasted the whole window, shows no difference.
- Normal play ignites one to three enemies per Molten Strike; this test is a deliberate worst case.

**A performance defect found and fixed during this work.** The first load run (`burn_perf`, before the fix) averaged 58.6 ms per host frame with 32 frames over 100 ms, against the 19 ms baseline. Two causes, both in new code: igniting a plain enemy created a `UGOTAHeroAbilityComponent` on it, which searched for a combat target every frame; and the "not a hit" event reached every such component on every tick. Now Burning does not create that component, and the AI momentum check runs only for duel fighters. Same script after the fix: 27.3 ms, 0 frames over 100 ms (`burn_perf3`), then 24.9 ms with the host fighting too.

**A second defect found and fixed.** The first Burning run showed each tick granting the player +3 Divine Momentum as a "hit landed", because the duel fighter component reads landed hits from the opponent's health. `GOTAGAS::OnStatusDamage` now discounts status ticks there and in the cast-interrupt check; the suite asserts momentum is unchanged by ticks.

#### Horde outcomes without forcing (packaged)

| Attempt | Result |
|---|---|
| Team wipe by not fighting (`pkg_burn_wipe`) | **Not achieved.** Four idle players stood at spawn for four minutes of wave 1 and took no damage (all 100/100); the wave-1 enemies never attacked them. No conclusion about defeat handling from this run. |
| Team wipe from combat (`pkg_burn_perf_base`) | **Observed.** With 20 extra test-spawned enemies and ignite switched off, all four fighting players were killed by enemy attacks; each logged `ELIMINATED`; `Horde match complete outcome=Defeat score=200` logged once. No damage was injected. It is not a standard match because of the extra enemies. |
| Five-wave victory (`pkg_burn_victory`) | **Incomplete: no victory, no defeat.** Four players fought for 20 minutes with melee and real abilities; nothing was injected (`SkipPrep` only ended the opening preparation timer). Waves 1–4 were cleared; the script ended in wave 5 of 5 with 9 of 14 enemies defeated, score 7,750, all four players alive (0 player deaths). The boss phase was not reached. 51 kill credits, 427 ability hits, 86 Molten Strike hits, 63 ignitions, 4 refreshes, 176 burn ticks for 352 damage, one enemy killed by a tick. Wave 5 slowed to about one kill every two minutes, which is the fight driver reaching scattered enemies, not a Burning effect; `HORDE STALL` (30 s without a landed hit) was never logged. No assertion or crash. A longer scripted run is needed to finish the match. |

### Boss policy milestone (`0a9f852`), editor and packaged RC7

**Packaged build:** fresh `RunUAT BuildCookRun` Win64 Development from `0a9f852`, archive `B:\Unreal\GOTA_Builds\RC7_0a9f852` (inner exe 346,651,648 bytes). Cook log: 3,933 packages cooked, **0 incrementally skipped**, 16 skipped by platform, no cook errors. RC1–RC6 not touched.

| Run | Editor | Packaged RC7 |
|---|---|---|
| Full regression suite (all abilities + Dread, Veiled, Unbreakable, Boss) | 42/42, first pass | 42/42, first pass |
| Four-player Horde with the Quarry Behemoth, rendered remote Kairos, clients 120 ± 30 ms, 5% loss | pass (`bp_horde`) | pass (`pkg_bp_horde`) |

Boss group (`?Boss=QuarryBehemoth`):

- **Policy:** `status policy QuarryBehemoth immune=[Slow Stun]` at spawn.
- **Time Lash:** hits the boss for 6.0; `status Slowed resisted (whole movement)`; no `GAS move` on the boss; the caster's screen shows "THE QUARRY BEHEMOTH RESISTS SLOW" (screenshot `Saved/Screenshots/GOTA/boss_resist_feedback.png`).
- **Dread on the boss:** `resisted (movement)`, effect still applied with walk ×1.00 and damage ×0.82. Claw Combo hits 9 / 9 / 11 before, 7.4 / 7.4 / 9.1 with Dread.
- **Divine Counter (`slice_mechanics.txt`):** 2 counters, each `STUNNED … for 2.6s` and `90 damage (landed=1)`, one cutting an attack short; 2 perfect parries (`PARRIED … open for 1.6s`); 5 perfect dodges. The bot made three parry attempts: two landed and one was late (lead 0.00) because that combo started before the script placed the player. The slice document's 9–11 "parries" for this script could not be reproduced or compared: no log of those runs exists, and a parried combo is cut off, so at most one parry per forced combo is possible.
- **Boss death:** `cleared 1 status effect(s) (death)`; the policy (not a status) stays.

Four-player Horde with the boss (packaged): `players=4`; 16 Slowed applications on the boss credited to remote Kairos, all resisted; remote Nyx's Dread partially resisted (walk ×1.00, damage ×0.82); Kairos's client showed the resist line 16 times (packaged screenshots `RC7_0a9f852\Windows\WarOfGods\Saved\Screenshots\GOTA\mp_resist_*.png`), Nyx's client once (her Dread), Drakon's none. No crash. In the editor run of the same script, real Time Lash casts still slowed regular enemies (`BP_Enemy_Melee_C_1: status Slowed 3.0s x0.40`).

Fixed during the run: a client could show "TARGET RESISTS SLOW". The boss definition is held softly on clients and had been unloaded after the body was built; the feedback now reloads it for the name.

**Replication cost of an ASC per enemy** (four-player Horde, 30 extra enemies walking in, nobody fighting, host `-nullrhi`, 20 s window, two runs each):

| `gota.GAS.EnemyASC` | ASCs | Host upload in 20 s | Host avg frame |
|---|---|---|---|
| 1 (every enemy at spawn) | 38 | 378 KB, 387 KB | 17.4 ms, 17.6 ms |
| 0 (only when a status lands) | 4 | 287 KB, 288 KB | 16.2 ms, 15.8 ms |

An idle ASC costs about 2.8 KB of host upload per enemy per 20 s and adds frame time. The default is now 0. With it, the four-player status run (`st_horde_lazy`) attached 11 ASCs (4 players, 7 enemies hit by statuses); every status, cue and overlay still reached all three clients; 4 kill credits; no stall. A fighting run with the same 30 enemies was too noisy to compare (253 KB vs 786 KB, the opposite way).

### Status milestone (`cf869ce`–`d103007`)

All runs use editor `-game`, not a cook. Multiplayer clients used `NetEmulation.PktLag 120`, `PktLagVariance 30`, `PktLoss 5` (pings 143–145 ms). Four-player windows ran `-nullrhi`. Nothing injected a kill except where stated.

### Regression suite (`Tools/GOTA_AbilityRegression.py`, Duel, single player plus listen server and one remote client)

Before the final status commit: 31/31 pass (every existing ability plus 5 Dread cases). Status groups on the final binary:

| Group | Cases | Key evidence |
|---|---|---|
| Time Lash | 4/4 | 6 damage, `status Slowed 3.0s x0.40`, walk 500 → 300, cooldown denial from the ability component, remote client |
| Dread | 5/5 | Molten Strike 22 → 18.1 (22 × 0.825, logged to one decimal) exactly once; Dread + Slowed walk ×0.60, never ×0.39; Slowed expires first, Dread stays at ×0.65; Molten Strike back to 22.0; Soul Drain and World Burn apply it; death clears both statuses; remote client walks ×0.65 → ×0.60 → restored |
| Veiled | 3/3 | `IsInvincible` on/off; Molten Strike misses and the Blueprint 20 hit does nothing (260/260) while Dread keeps running; after expiry the same hit takes 20 and Molten Strike lands 22; Nyx Void Shield 1.3 s; remote client overlay on/off |
| Unbreakable | 3/3 | `IsInterruptible` off/on; refunds 10 of 20 and 5 of 10; Inferno Slam not interrupted while armoured, interrupted after; coexists with Slowed; Colossus Form 6 s ×0.5; remote client overlay hands off Unbreakable → Slowed at expiry |

**Final binary, full suite: 36/39 on the first pass, 39/39 after re-running the three failures.** The three (Titan Charge cooldown and multiplayer, Unbreakable multiplayer) happened while the machine was heavily loaded: the remote client advanced 9 frames in two minutes, so the server applied and removed the 4 s Unbreakable between two client frames. The Slowed effect from the same step still reached that client and expired correctly. Re-running only those two groups: 7/7 pass.

### Four-player Horde (`st_horde`, Quarry, host Varoth; remote Nyx, Kairos, Drakon)

`players=4`. All statuses came from real abilities through the fight driver: World Burn and Soul Drain Dread on enemies, Time Lash Slowed on enemies, Void Shield Veiled on remote Nyx, Colossus Form Unbreakable on the host. One enemy carried Dread and Slowed together and walked ×0.60. Three real kill credits (+100 each). No `HORDE STALL`, no crash, no ensure. Every remote client logged the enemy cues and both player statuses (Veiled, Unbreakable) applied and removed.

### Four-player Story (`st_story2`, Divine Shrine, same heroes)

The fight driver cannot reach the shrine enemies (abilities miss with `los=0` at 438 cm; same in the earlier GAS Story run). Statuses were therefore applied with the new server command `gota.Net.Status`, which uses the same `ApplyStatus` path as the abilities. Results:

- `players=4`; remote Veiled player hit for 20: 100/100; remote Unbreakable player hit for 20: 90/100 (`absorbed 10.0 of 20.0`).
- Overlaps Unbreakable + Slowed and Dread + Slowed, on players and on a shrine enemy: walk ×0.60, then ×0.65 after Slowed expired, then restored.
- Lethal hit on remote Drakon carrying Dread + Slowed: `cleared 2 status effect(s) (death)`, its own client restored walk 500, respawned as a new pawn at 100/100.
- Every client logged the overlay for each other player's status. No crash.

### Rendered overlay check (Duel, single player)

Screenshots `Saved/Screenshots/GOTA/gas_overlay_*.png`, overlay log: none → Slowed → Dread → Veiled → Dread → Slowed → none. Veiled reads clearly. **Slowed and Dread are barely visible** in arena lighting. Those are the pre-existing overlay materials, unchanged by this work, and need stronger art.

### Test-harness fixes found on the way

The editor no longer hitches the way the November 1 packaged run did, which exposed timing assumptions:

- Six "cooldown denial" presses came during cast + recovery and were refused as `Busy`. They now wait out cast + recovery.
- The remote client checked its body after the host had already quit. It now checks several times.
- The Unbreakable interrupt case runs the wind-up at 0.2× so a slow frame cannot skip past it.
- The client now logs `WhileActive` cues, which is how a replicated effect usually reaches the owning client.

### Not verified

RC11:

- Human players. The playtest kit is ready (`GOTA_HORDE_PLAYTEST.md`); no person has played against attacking enemies.
- A five-wave victory against wave enemies that fight, a post-victory window with fighting enemies, and the Horde boss with an AI controller.
- Ready-up on separate PCs; the timeout and leave cases packaged (editor only).
- Shrine boss when its target changes or disconnects mid-attack.

RC10 stabilization:

- ~~The shrine guard and the Horde controller fix in a package~~ done in RC11.
- ~~The frame cost of `r.SkeletalMesh.UseCachedMDCs=0`~~ measured in RC11 (+1.4 ms render thread).
- Victory runs 4 and 5 of the original packaged target.

Burning milestone:

- Shipping configuration, a second PC, dedicated server. Everything ran on one machine with emulated lag and loss.
- A four-player Story run in the editor (Story was run packaged only), and Story burns from a real Molten Strike (applied with `gota.Net.Status`, as before).
- ~~Burning damage on the Story shrine enemy~~ explained in RC8 stabilization: the enemy is stuck invincible (behavior-tree content), and refuses all damage.
- ~~A standard-match team wipe~~ done in RC10 (editor): `horde_natwipe_ai1`, one natural `outcome=Defeat`.
- ~~The other 40 packaged suite cases after the `burn_boss.txt` fix~~ done on RC9: 46/46.
- Which part of the 24-burn frame cost is ability systems, particle emitters or replication. Totals only: on RC9, +3.6 ms host frame, +1.4 ms GPU, about +40 MB peak memory.
- A packaged run reproducing the cloth crash before the setting change (it could not be reproduced on demand).
- A player who disconnects while their burn is running on an enemy (the rule is "ticks continue, credit nobody"; only reasoned from the code).
- Burning applied by an enemy or boss ability. No enemy has one; only test commands set players alight.
- Sound. Burning has no audio cue.

Earlier milestones:

- Shipping configuration, a second PC, dedicated server.
- Story statuses from real hero abilities (the fight driver cannot reach shrine enemies; Story statuses were applied with `gota.Net.Status`).
- A full five-wave Horde to victory with the boss policy, and a team wipe. Waves, kill credit and the stall watch were exercised, not a complete match.
- `GOTA.Immunity.Stun`, `Weaken` and `Interrupt` have no live user yet (no Stunned/Frozen effect, no boss with Weaken). They are checked by code paths exercised only through Slow.
- Rendered status overlays on a remote client. The resist line was rendered remotely; the status overlay materials were only logged remotely.
- Performance with fighting at 30 enemies (too noisy in one run each), CPU/GPU on a rendered host, memory.

### Remaining risks

- **Cloth assertion** (`GPUSkinVertexFactory.cpp:1348`): mitigated in RC9 by `r.GPUSkin.BoneTransformAllocationMode=0`, which removes the crashing call path. Not reproduced on demand, so the fix is reasoned from the engine source rather than demonstrated. Watch rendered-client joins.
- **Release blockers after RC11 (in order):** (1) no human has played Horde against fighting enemies; (2) the spinning sword attack takes 90 of 100 health from one target and almost always lands; (3) enemies more than about 12–15 m from every player stop and never re-engage, so a player who hangs back can stall a wave forever; (4) ranged enemies' move requests fail (`Invalid`); (5) no five-wave victory against fighting enemies, so the post-victory crash fix has not been re-checked in that situation; (6) Story clients drop movement-base corrections (graybox floor components). None of these were changed in RC11.
- **Menus cannot host multiplayer Horde.** The Multiplayer screen always selects Duel, and `bHordeLaunchEnabled` is off; human playtests use the Development console (`GOTA_HORDE_PLAYTEST.md`).
- **Post-victory renderer crash:** fixed by `r.SkeletalMesh.UseCachedMDCs=0` (RC10; 11/11 crashed before, 3/3 clean after). Provisionally validated on three runs. Cost measured in RC11: about +1.4 ms render thread. Engine-level root cause (which skinned proxy keeps the single-frame buffer) not found, so a future engine or content change could bring it back if caching is turned on again.
- **Horde balance against attacking enemies (next gameplay blocker):** the bots lose wave 1 every time; human difficulty is unknown; enemy damage does not scale with player count. Needs a human playtest and hit/time-to-kill logging before any tuning.
- **Horde boss with an AI controller** has not been seen in a match.
- ~~Story remote client log stopped~~ explained in RC11: the harness's own `--kill-client-at 45`.
- ~~Story shrine boss stuck invincible~~ fixed in `74c480d` (editor).
- ~~Horde wave enemies never attack~~ fixed in `f423fad` (editor).
- **Many simultaneous burns cost host frame time** (2.4–6.7 ms for 24). Fine for Molten Strike; an area ability that ignites a whole wave would need the cost split measured first.
- **Burn ticks on the same frame:** enemies ignited by one swing tick together. At 0.1 ms each this is not a hitch today; it would be if the Blueprint damage path became heavier.
- **The Duel HUD does not list Burning** in its per-tick status text; the mode-independent BURNING line does. Two status displays exist until the HUD is made event-driven.

- **Overloaded-client test failures** (tracked separately from GAS): on a starved machine a remote client can advance a few frames per second and miss a 4 s effect entirely, and frame-time-sensitive script steps can mistime. Seen once on 2026-11-08; all affected cases passed on rerun and in every run since.
- **Resistance on a boss with its own speed control:** if a future boss policy *reduces* rather than blocks slows, the hero ability component would scale whatever walk speed the boss component last wrote, including a charge. Use immunity for bosses, or teach the boss component to read the movement attribute first.
- **Execute cues are unreliable multicasts:** under packet loss a resist message can be dropped. It is cosmetic; the server result is unaffected.
- **Lazy enemy ASC:** once an enemy has had a status, its ASC stays until it dies. A long boss fight with constant statuses on many adds will keep paying the cost for those enemies.

## Next reusable GAS features (proposal, in order)

1. ~~Boss status rules~~ done in `0a9f852`.
2. ~~Burning (damage over time)~~ done in `548a433`, with the element definitions.
3. **Frozen / stagger.** Tag-only effect plus a short `Stunned` tag the ability component and AI respect. Needs an AI cancellation path in `UGOTABossComponent` and the Horde AI. Reuses the overlay cue.
4. **Shields.** A `Shield` attribute absorbed before `ApplyDamage` in the C++ ability path, and a Blueprint-side check before Blueprint melee. This is the first change that touches the damage pipeline, so it needs its own design note.
5. **Buffs and debuffs.** Positive modifiers on the existing two attributes (they already stack positively). First user: Colossus Form's "increased power" card text.
6. **Resistances.** `GOTA.Element.*` asset tags on damage, read against per-element multiplier attributes, applied in the same single place as Dread.
7. **Projectiles and AOE.** A reusable spawned actor that carries an effect spec and applies it on overlap. Only once a hero needs a projectile.
8. **Enemy abilities.** Regular enemies and bosses apply these same effects; their AI and attack scheduling are unchanged.
9. **HUD and cues.** Event-driven status icons from tag changes instead of the current per-tick text; stronger overlay art for Slowed and Dread; status cues for sound.

Converting abilities to `UGameplayAbility`, or moving cooldowns, momentum, or health into GAS, is not proposed. None of those fixes a current problem.
