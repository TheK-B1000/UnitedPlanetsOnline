# Selective GAS enhancement plan

**Updated:** 2026-11-15. **Branch:** `feature/gota-cinematic-frontend`. The Burning milestone (`548a433` and the record commit after it) is local only, not pushed. **Engine:** Unreal Engine 5.8 native Gameplay Ability System. RC1–RC7 packages are not modified; Burning was packaged as RC8.

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
| Packaged (RC8) regression and the Burning record | next commit | see test record |

## Ownership

| Value or decision | Single owner | GAS role |
|---|---|---|
| Health, damage outcome, block, death | `BPC_DamageSystem` via `FGOTABlueprintCombat::ApplyDamage` | none |
| Momentum | `UStatsComponent` / `UGOTADuelFighterComponent` | none |
| Ability cooldowns, costs, cast timeline, range, LOS, hit detection | `UGOTAHeroAbilityComponent` | none |
| Statuses Slowed, Dread, Veiled, Unbreakable, Burning | **`UGOTAAbilitySystemComponent` (gameplay effects)** | owner |
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
| Hard crowd control | `GOTA.Immunity.Stun` | reserved for future Frozen / Stunned effects |
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

## Behavior changes

1. Statuses coexist (the point of this milestone). Before, a second status silently replaced the first.
2. Statuses are removed on death. Before, they ran out on their own.
3. The Duel HUD lists every active status at once instead of the single slot.
4. Bosses are no longer slowed by Time Lash or by Dread. Before, a slow also slowed the boss's charge rush.
5. Dread on a boss now weakens its attacks (×0.825). Before, boss attacks ignored Dread.
6. Plain Horde enemies get an ability system only when a status lands on them.

7. Molten Strike ignites: 6 more damage over 3 s on each enemy it lands on. Its impact, cooldown, animation, hit detection and momentum are unchanged.
8. `FGOTABlueprintCombat::ApplyDamage` takes an optional `bForceInterrupt` (default true, the old behaviour). Only status ticks pass false.

Nothing else changed: the same numbers, the same Blueprint flags, the same damage path.

## Test record

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

Burning milestone:

- Shipping configuration, a second PC, dedicated server. Everything ran on one machine with emulated lag and loss.
- A four-player Story run in the editor (Story was run packaged only), and Story burns from a real Molten Strike (applied with `gota.Net.Status`, as before).
- Burning damage on the Story shrine enemy: its damage system refused every tick; not investigated.
- A standard-match team wipe. The defeat that was observed had 20 extra test-spawned enemies.
- The other 40 packaged suite cases after the `burn_boss.txt` script fix (only the Burning group was re-run).
- Which part of the 24-burn frame cost is ability systems, particle emitters or replication.
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

- **Engine cloth assertion on a rendered remote client** (`GPUSkinVertexFactory.cpp:1348`, seen once, packaged Story, when a cloth-mesh hero body was created). It happened before any Burning existed in that session. It needs its own investigation; a real remote player would be disconnected by it.
- **Many simultaneous burns cost host frame time** (2.4–6.7 ms for 24). Fine for Molten Strike; an area ability that ignites a whole wave would need the cost split measured first.
- **Burn ticks on the same frame:** enemies ignited by one swing tick together. At 0.1 ms each this is not a hitch today; it would be if the Blueprint damage path became heavier.
- **Idle players are not attacked in Horde wave 1.** Seen in the unforced-wipe attempt. Not caused by this work, but it means a match can sit forever if nobody moves.
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
