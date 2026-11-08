# Selective GAS enhancement plan

**Updated:** 2026-11-08. **Branch:** `feature/gota-cinematic-frontend`, local commits only, nothing pushed. **Engine:** Unreal Engine 5.8 native Gameplay Ability System. RC2–RC6 packages are not modified.

We use GAS where it improves on what we already have. Health, momentum, cooldowns, basic attacks, block, perfect parry, perfect dodge, Divine Counter, and boss timelines keep their current owners. No talent trees, XP, levels, or persistent progression.

## Where we are

| Milestone | Commit | State |
|---|---|---|
| GAS plugin, combat ASC, Time Lash's slow as a gameplay effect | `3bc3fb2` | Done |
| Dread on GAS; shared status base; strongest-penalty-wins; death cleanup; bootstrap ASC; HUD | `cf869ce` | Done |
| Veiled on GAS | `5a16a2d` | Done |
| Unbreakable on GAS; all four statuses are effects | `6f1d135` | Done |
| Four-player Story / Horde status runs, `gota.Net.Status`, this document | next commit | Done |

## Ownership

| Value or decision | Single owner | GAS role |
|---|---|---|
| Health, damage outcome, block, death | `BPC_DamageSystem` via `FGOTABlueprintCombat::ApplyDamage` | none |
| Momentum | `UStatsComponent` / `UGOTADuelFighterComponent` | none |
| Ability cooldowns, costs, cast timeline, range, LOS, hit detection | `UGOTAHeroAbilityComponent` | none |
| Statuses Slowed, Dread, Veiled, Unbreakable | **`UGOTAAbilitySystemComponent` (gameplay effects)** | owner |
| Walk speed multiplier, hero-ability damage multiplier | **`UGOTACombatAttributeSet`** | owner |
| `MaxWalkSpeed` writes | `UGOTAHeroAbilityComponent::ApplyMovementStatusLocally` (reads the attribute) | read |
| Blueprint `IsInvincible` while Veiled, `IsInterruptible` while Unbreakable | the ASC's tag events write them; `BPC_DamageSystem` still reads them | bridge |
| Unbreakable damage refund | `UGOTAHeroAbilityComponent` through the Blueprint's own `Heal` | reads tag + magnitude |
| Status overlay on the body | `GOTAGAS::RefreshStatusOverlay`, called by the cue | owner |
| Perfect parry, perfect dodge, melee, Divine Counter | fighter + Blueprint actions | none |
| Boss attacks, waves, score, victory | `UGOTABossComponent`, game modes | none |

The legacy status slot on `UGOTAHeroAbilityComponent` still compiles. `GOTAGAS::OwnsStatus` decides, per status, whether GAS or the slot applies it. Today it returns true for all four. Returning false for one status rolls it back without other changes.

## Architecture

- **ASC owner and avatar are the pawn.** Health already lives on the pawn, and every respawn (`RestartPlayer` in Duel, Horde, and Story) creates a new pawn, so statuses can never carry over. The PlayerState keeps `HeroId`, which is where a future loadout or perk choice would live.
- **Attached by `UCombatBootstrapSubsystem`** on the server to every pawn with a Blueprint damage system: players, AI fighters, Horde enemies, bosses. Clients receive the replicated component.
- **Replication: Mixed.** The owning client gets full effects (its HUD shows time left). Everyone gets tags, cues, and attribute values.
- **Effects are native C++ classes** (`UGOTAStatusGameplayEffect` and four subclasses). Duration and strength are SetByCaller values taken from the existing `FGOTAAbilityDef`, so balance numbers stay in one table.
- **Cues:** one native `UGOTAGameplayCue_StatusOverlay` is registered for all four cue tags in the runtime cue set on every peer.
- **Tags:** `GOTA.Status.*`, `GOTA.Immunity.*`, `GOTA.Element.*`, `GOTA.Data.*`, `GameplayCue.GOTA.Status.*` (`GOTAGameplayTags.h`). Burning, Frozen, Stunned, the immunities, and the elements are reserved names with no reader yet.

## Status interaction rules

| Rule | Behavior |
|---|---|
| Different statuses | Coexist. A new status never removes another. The old slot replaced whatever was active, so Time Lash used to strip Unbreakable or Dread. |
| Same status again | One stack per target. Reapplying refreshes the duration and keeps the running strength. Every current source of a status uses the same strength. |
| Overlapping penalties | **Strongest penalty wins** for walk speed and hero-ability damage, using the engine's `MostNegativeMod_AllPositiveMods` aggregator. Dread (×0.65) + Time Lash (×0.60) walk at ×0.60, not ×0.39. Future buffs still add on top. |
| Damage modifier | Dread's ×(1 − 0.5 × Magnitude) is read once per hit in `DealDamage`, before `BPC_DamageSystem`. Blueprint melee does not read it, as before. |
| Expiry | Each status expires on its own timer. Movement and overlay fall back to whatever is still active. |
| Removal | Death (`UStatsComponent::OnDied`), ability reset, and test reset remove every `GOTA.Status` effect. Respawn is a new pawn. Session travel is a full map load; no seamless travel is configured, so nothing survives it. |
| Overlay priority | Veiled > Unbreakable > Dread > Slowed. One overlay material at a time; when the top status ends, the next one shows. |
| Immunity | `GOTA.Immunity.Slow` blocks Slowed. Nothing grants it yet; bosses are the expected first user. |

Formulas kept from the slot: walk ×clamp(1 − Magnitude, 0.2, 1); Dread damage ×clamp(1 − 0.5 × Magnitude, 0.2, 1); Unbreakable refund = health lost × clamp(Magnitude, 0, 0.9).

## Behavior changes

1. Statuses coexist (the point of this milestone). Before, a second status silently replaced the first.
2. Statuses are removed on death. Before, they ran out on their own.
3. The Duel HUD lists every active status at once instead of the single slot.

Nothing else changed: the same numbers, the same Blueprint flags, the same damage path.

## Test record

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

- Packaged build, Shipping, a second PC, dedicated server.
- Story statuses from real hero abilities (the fight driver cannot reach shrine enemies; see above).
- Statuses on a boss. Bosses get the ASC, but no ability or test has applied one yet.
- Performance cost of an ASC on every Horde enemy. No `stat net` or Insights capture yet.
- Rendered overlays on a remote client (four-player runs were `-nullrhi`; the overlay logs show the material was set).

## Next reusable GAS features (proposal, in order)

1. **Boss status rules.** Grant `GOTA.Immunity.Slow` (or a shorter duration) to bosses through a passive effect from the boss definition, so Time Lash cannot lock down the Quarry Behemoth. Add a stun limit before Stunned exists.
2. **Burning (damage over time).** Periodic effect whose execution calls `ApplyDamage` once per tick, so `BPC_DamageSystem` stays the only health writer. First user: Molten Strike's card text "ignites enemies". Element tag Fire.
3. **Frozen / stagger.** Tag-only effect plus a short `Stunned` tag the ability component and AI respect. Needs an AI cancellation path in `UGOTABossComponent` and the Horde AI. Reuses the overlay cue.
4. **Shields.** A `Shield` attribute absorbed before `ApplyDamage` in the C++ ability path, and a Blueprint-side check before Blueprint melee. This is the first change that touches the damage pipeline, so it needs its own design note.
5. **Buffs and debuffs.** Positive modifiers on the existing two attributes (they already stack positively). First user: Colossus Form's "increased power" card text.
6. **Resistances.** `GOTA.Element.*` asset tags on damage, read against per-element multiplier attributes, applied in the same single place as Dread.
7. **Projectiles and AOE.** A reusable spawned actor that carries an effect spec and applies it on overlap. Only once a hero needs a projectile.
8. **Enemy abilities.** Regular enemies and bosses apply these same effects; their AI and attack scheduling are unchanged.
9. **HUD and cues.** Event-driven status icons from tag changes instead of the current per-tick text; stronger overlay art for Slowed and Dread; status cues for sound.

Converting abilities to `UGameplayAbility`, or moving cooldowns, momentum, or health into GAS, is not proposed. None of those fixes a current problem.
