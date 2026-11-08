# Selective GAS enhancement plan

**Date:** 2026-11-07. **Branch:** `feature/gota-cinematic-frontend`. **Engine:** Unreal Engine 5.8 native Gameplay Ability System. Nothing is pushed. RC2–RC6 packages are not modified. The RC6 Horde casting stall is already diagnosed in `GOTA_HORDE_COOP_COMBAT.md` (commit `08a5adc`): the fight driver walked in a straight line and pinned on quarry geometry, so casts were accepted and line-of-sight rejects were silent. That is not a reason to rebuild combat.

This pass does not convert perfect parry, perfect dodge, basic attacks, blocking, Divine Counter, boss timelines, or the other hero abilities to GAS.

## Inventory

### 1. Implemented and verified

- Server damage goes through `FGOTABlueprintCombat::ApplyDamage` into Blueprint `BPC_DamageSystem::TakeDamage`. Health authority stays there. `UStatsComponent` is the replicated mirror.
- Four Horde kits are real data assets: Drakon, Nyx, and Varoth have four abilities; Kairos has Time Lash. RC6 logs show those casts landing (Molten Strike 22, Titan Charge 14, Dragon Roar 6, Mind Spike, Inferno Slam, Ashen Charge, World Burn, Time Lash).
- Cooldowns are `CooldownEnd[]` on `UGOTAHeroAbilityComponent`. The server rejects `Cooldown`, `Momentum`, `NotLive`, `Busy`, death, range, and LOS. Ultimates cost 100 momentum.
- Statuses Unbreakable, Veiled, Dread, and Slowed exist as one slot on that component. Veiled sets Blueprint `IsInvincible`. Unbreakable clears `IsInterruptible` and refunds a fraction of damage. Dread slows movement and scales outgoing ability damage. Slowed scales movement only. Magnitude 0.4 means 60% walk speed.
- Horde on RC6: five waves cleared, Quarry Behemoth spawned at 900 health with four attacks, claw damage hit the host and another pawn, Behemoth kill credit was +2500. Same-team arc filter and dead-owner guards are compiled in.
- Scripted Rampage on RC6: perfect parry, perfect dodge of Earthshaker / Behemoth Charge / Crushing Grasp, and Divine Counter stun. Client leave returns to the main menu. Story lobby accepts four players.
- Stall watch (`gota.Horde.Watch`) and the fight strafe are in `08a5adc`. The short editor retest `rc6stall` logged ability hits and no `HORDE STALL`.

### 2. Implemented, and needs improvement

- One status slot. A new status replaces the previous one, so Time Lash used to clear Dread, Veiled, or Unbreakable.
- Status, cooldown, and momentum are custom replicated fields. Nothing is queryable as a Gameplay Tag.
- Status overlays are switched in `OnRep_Status`, not by a cue.
- Card text that the ability does not actually do: Molten Strike ignite, a hard stun, Soul Drain energy steal, World Burn as a damage-over-time, Colossus Form's power and area bonus. Kairos has one ability, and Time Lash does not slow attacks.
- The Horde fight driver is still a test controller. Strafe avoids a pin; it is not navigation.
- Boss-wave adds kept the RC6 finale from firing Victory. An idle team was not wiped by wave-1 grunts in two minutes. A second rendered client hit the engine cloth/skin assert. The fourth-login hitch (359 ms in RC4) has no Insights trace.

### 3. Missing

- The Gameplay Ability System was not enabled before this pass.
- Gameplay Tags on abilities, attribute modifiers, Gameplay Cues, and stacking buffs.
- A real Horde victory (exactly one) and a real wipe (exactly one) without `KillEnemy`, `ForceClear`, or `Hurt`.
- Horde perfect parry, perfect dodge, and Divine Counter during a free boss fight. `RemoteDefend` armed at join expires before the boss.
- Duel player-versus-player, HUD respawn, Shipping, and a second PC were not re-run on RC6.

### 4. What GAS genuinely improves

- Hero status effects as duration effects with their own tags, stacking rule, and cue. Time Lash's slow is the first one.
- Later, the same pattern can own Dread, Veiled, and Unbreakable without a second health pool.
- Gameplay Cues for the status overlay instead of an `OnRep` material switch.
- Attribute modifiers only when a future buff must scale damage. They must not become a second health or momentum attribute.

GAS does not improve parry windows, dodge windows, melee traces, boss attack timelines, or hit validation.

### 5. What stays unchanged

| Resource | Single owner |
|---|---|
| Health and the damage outcome | `BPC_DamageSystem` via `ApplyDamage` |
| Cooldowns | `UGOTAHeroAbilityComponent::CooldownEnd` |
| Momentum | `UStatsComponent` / the fighter |
| Cast timeline, range, LOS, hit detection | `UGOTAHeroAbilityComponent` |
| Dread, Veiled, Unbreakable | the same component's status slot |
| Perfect parry, perfect dodge, block, melee, Divine Counter | the fighter and its Blueprint actions |
| Boss attacks | `UGOTABossComponent` |
| Waves, score, victory, defeat | `AHordeGameMode` |
| Animations and hero bodies | existing assets |

## Proof of concept: Kairos Time Lash

Gameplay behavior that must stay the same: 6 damage, 12 second cooldown, aura radius 450, Slowed for 3 seconds at magnitude 0.4 (walk speed × 0.6). Damage still goes through `ApplyDamage`. The cooldown is still denied by the ability component. Momentum is not read or written by GAS.

What changed: the Slowed status is one `UGOTAGameplayEffect_Slowed`. Duration and magnitude are SetByCaller from the existing ability definition, so there is no second balance table. Stack limit is 1 and a new application refreshes the duration, which matches the old timer reset. Replication mode is Full so simulated proxies receive the effect. `ApplyMovementStatusLocally` reads that magnitude when the component slot is not already Dread or Slowed. The slot wins if both exist, so Dread is not applied twice.

The old single slot replaced whatever status was active. Time Lash no longer clears Dread, Veiled, or Unbreakable. That edge is intentional and documented here; it is the one behavior difference, and it only shows up when two statuses overlap. Time Lash by itself still only slows movement.

`UGOTAGameplayCue_Slowed` owns `M_GOTA_Overlay_Slowed`. It is registered on the runtime cue set the first time the effect is applied, because a native cue is not discovered by an asset scan. The cue does not clear a different overlay.

No other ability calls the gameplay effect. Enabling the plugin does not migrate them.

## Sequence

1. **Done.** Diagnose the RC6 casting stall (`08a5adc`). Do not start a broad migration on top of an undiagnosed stall.
2. **This pass.** Enable `GameplayAbilities` and `GameplayTasks`. Move only Time Lash's slow. Compile the editor. Test in editor `-game` (do not recook RC6): Duel, four-player Story, and Horde, with 120 ms ± 30 and 5% packet loss on the remote clients. Evidence is a `GAS slow 3.00s x0.40` log, the existing 6 damage hit, and a cooldown denial that still comes from the ability component. `gota.Horde.Watch` must stay quiet while damage is landing.
3. **Only if step 2 passes.** Move Dread the same way. It also scales outgoing damage, so the ability component must read the effect instead of the slot. Still one owner.
4. **After Dread.** Veiled and Unbreakable. These write Blueprint `IsInvincible` and `IsInterruptible`. Higher risk. Keep the Blueprint flags as the authority the damage system already reads, and drive them from the effect's apply and remove.
5. **Do not do yet.** Move cooldowns, momentum, or health onto GAS attributes. Convert abilities to `UGameplayAbility`. Rebuild parry, dodge, block, melee, or the boss.

## Benefits, risks, regressions

**Benefits.** Statuses can coexist. Duration and magnitude stay in the ability definition. The overlay has one owner. The pattern is small enough to delete if the tests fail.

**Risks.** Every slowed pawn gets an ability-system component replicated in Full mode. That is extra traffic and is acceptable only while the effect is rare. A cue registered after the manager exists can miss if `InitGlobalData` has not run; the movement slow still applies and the log shows which half failed. Applying the effect and also writing `ActiveStatus = Slowed` would slow twice; Time Lash does not write the slot. The plugin enable forces an editor rebuild. Do not copy that binary over RC6.

**Regressions that must stay green.** Time Lash damage 6 through `ApplyDamage`. Cooldown still owned by the component (a second cast inside 12 seconds logs `denied (Cooldown)`). Dread, Veiled, and Unbreakable still use `ApplyStatus`. Horde watch does not report a stall while hits land. Parry, dodge, and Divine Counter code is untouched; the RC6 Rampage result still stands and is not re-run as part of this proof. No `KillEnemy`, `ForceClear`, or `Hurt` as proof.

## Test record

Editor `-game`, not a cook. RC2–RC6 were not modified. Remote clients used `NetEmulation.PktLag 120`, `PktLagVariance 30`, and `PktLoss 5`. Every window was `-nullrhi` so four processes did not render. `gota.Net.Fight` and `gota.Net.Ability` drive the local pawn; nothing injected a kill.

**Horde, four players, final binary (`gashorde2`, port 7837).** Host Drakon, clients Nyx, Varoth, and Kairos. Kairos was a remote pawn (`local=0`). Wave 1 was live, `players=4`, `gota.Horde.Watch` armed for 12 seconds. No `HORDE STALL`. Two real kill credits (+100). Kairos Time Lash:

- `TimeLash hit BP_Enemy_Melee_C_2 for 6.0 (landed=1)` through `ApplyDamage`
- `status Slowed 3.0s x0.40 ... [GAS walk x0.60]`
- `GAS move ... fraction=0.40 walk 500 -> 300`
- the cue `GameplayCue.GOTA.Status.Slowed` ran on that enemy
- the next casts logged `TimeLash denied (Cooldown)` from the ability component
- a later cast, after the cooldown, applied 3.0s x0.40 again and walked 500 -> 300, so the slow did not compound

**Duel, remote Kairos versus the AI champion (`gasduel4`, port 7834).** The client possessed Kairos and called `gota.Net.Ability`. The server accepted Time Lash and denied the next two presses with `Cooldown`. The aura missed: nearest enemy was 92 cm away with line of sight blocked at the quarry start. The effect was not applied in that run. The cooldown owner is still the ability component under the lag settings.

**Story, four players at the Divine Shrine checkpoint (`gasstory`, port 7835).** All four joined, mission phase Active. Remote Kairos cast Time Lash and the next press was `denied (Cooldown)`. The aura missed at 499 cm (outside the 450 radius, line of sight blocked). No crash. The effect was not applied because the hit never landed.

Parry, dodge, block, melee, Divine Counter, and the boss component were not modified. The RC6 Rampage result still stands and was not re-run.
