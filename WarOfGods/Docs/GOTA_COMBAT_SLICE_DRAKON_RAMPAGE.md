# Combat vertical slice: Drakon vs Rampage (Quarry Behemoth)

**Date:** 2026-11-01
**Goal:** one complete, playable fight in which attacks feel powerful, the enemy responds convincingly, perfect parries
and dodge timing matter, and the Divine Counter is a spectacular payoff. The same foundation then carries over to
Story, Horde and PvP.

**Run it:** `L_Arena_TitansQuarry?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=1?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth`

## What was missing (before)

The duel's parry, dodge and counter layer (`UGOTADuelFighterComponent`) was built against the Fight AI champion. It
read the champion's Blueprint montage notifies. The boss runs its own attack timeline, so against Rampage:

| Mechanic | Before |
|---|---|
| Perfect parry | **Impossible.** Every boss hit was sent with `CanBeParried=false` |
| Perfect dodge | **Never detected.** Dynamic boss montages have no impact notifies |
| Divine Counter | 25 damage (2.8% of 900 HP). No reaction unless the boss was already idle |
| Enemy reaction | Stagger only after 70 damage in 3 s, and only while chasing or recovering |
| Test bot | Could not see boss attacks: no blocks, no dodges. Died in about 12 s with the boss at 700/900 |

## The fight loop now

| Player action | Rampage's response | Reward |
|---|---|---|
| **Block timed into the parry window** against Claw Combo (the only parryable attack) | Combo cut off. Recoils (`KnockBack`). **Open 1.6 s** | +20 momentum, big sparks, hit-stop, camera kick |
| **Dodge within 0.35 s of impact** against Charge, Earthshaker or Crushing Grasp (unblockable, telegraphed) | The attack passes through; the charge doesn't knock back | +15 momentum, chime. The dodge is *guaranteed* to avoid that impact |
| Dodge too early | The Blueprint's own 0.2 s i-frames run out, and you're hit unless you moved clear | — |
| **Divine Counter** (Q, 50 momentum, within ~4.4 m of his capsule edge) | Caught at once (even mid-attack). **Stunned 2.6 s** (`Stun_Start` + `Stun_Idle`). Phase II waits until the stun ends | Drakon plays **Molten Strike** (his own animation, FX and sound); the blow lands 0.3 s later for **90 damage** with the DIVINE COUNTER callout |
| Landed hits while he chases or recovers | **Directional flinch** (front/back/left/right `HitReact`, upper body, at most every 0.9 s) | +3 momentum |

Design notes:

- **Parry vs dodge is readable.** Claws are the only parryable attack (no telegraph text). Everything with a red
  telegraph ("SIDESTEP!", "GET CLEAR!", "BACK OFF!") must be dodged.
- **Punish windows don't chain.** Damage dealt while he's open (staggered or stunned) no longer triggers the ordinary
  70-damage stagger when he recovers, so one parry or counter buys exactly one window.
- **Server-authoritative.** Parry still comes from the player's Blueprint damage system (`OnBlocked` inside the parry
  window). Perfect-dodge protection and counter damage are decided on the server. The counter strike is a reliable
  multicast with presentation only.

## Verification

Bot-driven (`gota.Duel.Bot`, now boss-aware: it reads the boss's hit windows, parries the claws and dodges the
unblockables to an explicit point via `BP_Player::DodgeToLocation`).

**Scripted mechanics** (`Tools/Tests/slice_mechanics.txt`):
- Forced Claw Combos, parried.
- Each unblockable dodged.
- A counter on an idle boss, and one mid-Claw-Combo.

| Run | Perfect parries | Perfect dodges (slam, charge, grasp) | Counters (stun, 90 dmg) | Stray stagger after stun |
|---|---|---|---|---|
| mech1 (first build) | 10/10 | 2/3 (the slam was hit) | 2/2 | **yes** (fixed) |
| mech2 | 9/9 | 3/3 | 2/2 | none |
| mech3 (final) | 11/11 | 3/3 | 2/2 | none |

**Full fight** (`Tools/Tests/slice_drakon_rampage.txt`, bot in `fight` mode after the intro):

| Run | Result | Parries | Perfect dodges | Counters | Boss HP at end |
|---|---|---|---|---|---|
| Baseline (old code) | Drakon dead in ~12 s | 0 | 0 | 0 | 700/900 |
| fight1 (dodge i-frames only) | Drakon dead in phase II (chained Earthshakers) | 4 | 1 | 3 | 260/900 |
| fight2 (final) | **Rampage defeated** in ~39 s, Drakon 81/130 | 5 | 6 | 5 | 0 |
| fight3 (final) | **Rampage defeated** in ~43 s, Drakon 74/130 | 4 | 8 | 5 | 0 |

**Fight AI regression** (`combat_fight_short.txt`, no boss): the counter still lands for 25 against the Sovereign
Champion.
- As Drakon: hero strike.
- As the Warden: Blueprint melee fallback.
- No crashes.

The bot's timing is perfect, so its win rate says nothing about difficulty for a human. It shows that every link in
the loop fires in a real fight: parry → open → counter → stun → punish → phase II → defeat.

## Known gaps / next steps

- **Rampage's pack is incomplete on this machine.**
  - `Content/ParagonRampage/.../Rampage/Textures` and the sound waves are missing, so he renders grey and his effort
    sounds are silent.
  - Re-add *Paragon: Rampage* from Fab (full pack), then re-run step 3 of `GOTA_THIRD_PARTY_ASSETS.md`.
- **Human playtest.** Tune `ParryStaggerSeconds`, `CounterStunSeconds`, `DivineCounterDamage` and `FlinchCooldown`
  (all in `Tools/GOTA_BuildBosses.py`), and `PerfectDodgeLeadSeconds` and `DivineCounterImpactDelay` (on
  `UGOTADuelFighterComponent`).
- **Parry tell.** A brief glint on the claw before a parryable swing would make the parry read without a HUD prompt.
- **Reuse.**
  - Story and Horde bosses get all of this through `UGOTABossComponent` data.
  - Ordinary Horde enemies and PvP still use the Fight AI path. The hero-strike counter presentation already applies
    there.
