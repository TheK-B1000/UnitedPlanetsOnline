# Combat Tuning — Trial of Champions (Sovereign Champion)

All values live in code defaults so they're reviewable in diffs:
- `FGOTAChampionProfile::ForDifficulty` and `UGOTADuelFighterComponent` UPROPERTYs in `Arenas/Duel/GOTADuelCombat.*`.
- Blueprint values are read, not changed, except the two spawn-time writes called out below.

## 1. Fixed Blueprint timings (measured with `gota.Duel.Notifies`)

| Montage | Length | Impact / window (s) | Notes |
|---|---|---|---|
| `Montage_Shield_Block` (challenger) | 1.38 | `Block` notify 0.089; `ParryWindow` 0.091–0.371 | Block becomes active ~0.09 s after RMB; parry window ≈ 280 ms |
| `Montage_Sword_Slash` (champion) | 1.50 | 0.58 | 10 dmg, blockable + parryable |
| `Montage_Sword_SpinningAttack` | 5.57 | 1.22, 1.5, 2.0 … 5.0 | 10 dmg per hit, blockable, not parryable |
| `Montage_Melee_GroundSmash` | 3.55 | 1.22 | 25 dmg, **unblockable** (dodge it) |
| `Montage_Sword_Jump_Attack` | 1.88 | 0.06 (launch), 0.75 | |
| `Montage_SwordCombo` (challenger) | 3.53 | 0.70, 1.29, 2.58 | 20 dmg per hit (BP_Player MakeStruct), blockable/parryable |
| Challenger dodge | — | `IsInvincible` 0.2 s | Blink teleport 300 uu |
| Parry riposte (`ParryAttack`) | — | — | 5 dmg, Stagger |

**Player-facing rule of thumb:** press block about a quarter-second before the champion's blade lands. A hit in the first ~0.3 s of the guard is a Perfect Parry.

## 2. Difficulty profiles

| Parameter | Recruit | Veteran (default) | Ascendant |
|---|---|---|---|
| Challenger max health ¹ | 160 | 130 | 110 |
| Champion max health ¹ | 190 | 260 | 340 |
| Reaction to visible wind-up (s) | 0.55 | 0.36 | 0.24 (floor 0.20) |
| Defend chance per wind-up | 20 % | 38 % | 55 % |
| Sidestep share of defences | 30 % | 35 % | 40 % |
| Reactive `BlockChance` (BP) ² | 0.15 | 0.25 | 0.35 |
| Guard-chain chance (after a blocked hit) | 45 % | 70 % | 80 % |
| Guard-chain window (s) | 1.6 | 1.6 | 1.6 |
| Shield overload: hits / window (s) | 2 / 6.0 | 3 / 5.0 | 3 / 4.5 |
| Guard broken for (s) | 3.0 | 2.5 | 2.0 |
| Phase II at champion health | 50 % | 50 % | 50 % |
| Phase II defend bonus / reaction scale | +10 % / ×0.9 | +15 % / ×0.8 | +15 % / ×0.85 |
| Sovereign Surge interval / telegraph (s) | 18 / 1.4 | 14 / 1.1 | 11 / 0.9 |

¹ Spawn-time tuning written into each fighter's own `BPC_DamageSystem` (`MaxHealth`, `Health`) before the round goes live. This is not a second health system: the Blueprint still applies every change. Recorded as an exception in `GOTA_COMBAT_AUTHORITY_CONTRACT.md` terms: configuration, not mutation.

² `BP_Enemy_Base::BlockChance`, the original reactive block roll in `TakeDamage → TryToBlock`.

Phase II also: +10 % `MaxWalkSpeed`, 1.4 s invulnerable ascension, an immediate ascension slam, and the first Surge at 1.4 s + 40 % of the interval.

## 3. Divine Momentum economy (`UGOTADuelFighterComponent`)

| Source | Momentum |
|---|---|
| Block | +4 |
| Perfect Parry | +20 |
| Perfect Dodge (dodge ≤ 0.35 s before impact, attack misses) | +15 |
| Landed hit | +3 |
| Guard break | +15 |
| **Divine Counter cost** | **50** (max 100, `UStatsComponent`) |

Divine Counter:
- 25 damage, ×1.4 on a broken guard.
- Unblockable and unparryable, response Stagger.
- 380 uu range; 0.6 s challenger invulnerability.

Design intent: one Divine Counter per ~2 perfect defences plus a few hits. In bot runs it fired 1–2 times per round.

## 4. Feel

| Parameter | Value |
|---|---|
| Hit-stop — block | 0.05 s |
| Hit-stop — perfect parry / guard break | 0.09 s |
| Hit-stop — Divine Counter | 0.11 s |
| Camera impulse | 0.35 (dodge) → 1.6 (counter), 0.28 s decay; scaled by *Camera Shake Intensity*, off with *Reduced Motion* |
| Callout durations | 0.9–2.0 s with a 0.35 s fade |
| "FIGHT" banner | 1.5 s |

## 5. How to tune

1. Play a match with `gota.Duel.DebugOverlay 1` and watch "last decision" and "pressure".
2. **Too passive / too oppressive:** adjust `DefendChance` first, then `ReactionSeconds`. Keep reaction ≥ 0.2 s; that's the fairness floor.
3. **Weak point never/always triggers:** adjust `GuardChainChance` or `GuardOverloadHits`.
4. **Rounds too short/long:** adjust the max-health pair. Damage values belong to the Blueprints; don't scale them in C++.
5. Re-run the bot matrix (`Tools/Tests/combat_difficulty_match.txt` with `?Difficulty=`) and compare round lengths in `GOTA_AI_DUEL_TEST_RESULTS.md`.
   - The test bot has perfect timing, so its results are an **upper bound** of player skill.
