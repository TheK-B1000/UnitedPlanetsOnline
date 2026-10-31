# Combat Authority Contract (Milestone 3)

Goal: one authoritative outcome path for multiplayer without a second damage framework or GAS.

## Ownership

| Concern | Owner today | Target |
|---------|-------------|--------|
| Attack input | Local player → BP `BPC_Attacks` | Client input, server validates windows |
| Hit detection | BP / anim notifies | Server confirms overlaps / traces |
| Damage apply | `BPC_DamageSystem` | Server-only apply; clients predict FX |
| Health mutation | BP health | Single path — either BP wraps C++ or C++ wraps BP; never both |
| `UStatsComponent` | Observation mode (default) | Stay observation until BP notifies `ObserveHealth` every damage |
| Parry / dodge / block success | BP timing | Server validates with latency tolerance; client never authorizes |
| Death | BP destroy / ragdoll | Server sets eliminated; clients play death |
| `UCombatComponent` Notify* | Local observers | Keep local; do **not** treat as replication |

## Rules

1. **No duplicate damage** — `UStatsComponent::ApplyDamage` stays no-op while `bAuthoritativeMutations == false`.
2. **Observers are local** — UI/VFX bind to Notify*; GameState carries match outcomes.
3. **Duel / Horde elimination** currently use **pawn validity** as the server round/wave signal. Wire `NotifyDied` → GameMode when BP death is reliable.
4. Prefer `ObserveHealth(Current, Max)` from BP after each health change so HUD can replicate display without claiming authority.
5. Do **not** adopt GAS in this milestone.

## Migration steps (ordered)

1. From `BPC_DamageSystem` (server branch only): after applying damage, call `Stats->ObserveHealth` + `Combat->NotifyHitConfirmed` / `NotifyDamageApplied`.
2. On death: `Combat->NotifyDied` + GameMode elimination hook.
3. Add server RPC for attack commit only after local prediction looks correct in PIE.
4. Enable `SetAuthoritativeMutationsEnabled(true)` only after BP stops mutating health directly.

## Verification checklist

- [ ] Listen + 1 client: mutual hits register once
- [ ] Block / parry do not double-apply
- [ ] Death ends Duel round on server for both peers
- [ ] Skirmish unchanged when not using DuelGameMode

## Implementation 03A hook (C++)

`ADuelGameMode` now binds `UCombatComponent::OnCharacterDied` and destroys the dead pawn, then resolves the round.  
**Still required in Editor (BP):**

1. On lethal in `BPC_DamageSystem` (server): `Combat->NotifyDied(Killer, Tags)` then Destroy or Disable.
2. After each HP change: `Stats->ObserveHealth(Current, Max)`.
3. Optional: implement `IDuelFighterInterface` on `BP_Player` for `ResetForDuelRound` / `SetDuelCombatEnabled`.

## Combat Milestone 01 addendum (Trial of Champions vs AI)

- **Divine Momentum** is C++-owned (`UStatsComponent::AddDivineMomentum` / `ConsumeDivineMomentum`, server). It's earned only from outcomes the Blueprint already resolved: `BPC_DamageSystem.OnBlocked`, the `IsInvincible` dodge window, and opponent health deltas.
- **Divine Counter damage** goes through the champion's own `BPC_DamageSystem::TakeDamage` (`FGOTABlueprintCombat::ApplyDamage`). It is not a second damage path.
- **Spawn-time configuration** (`FGOTABlueprintCombat::ConfigureHealth`): per-difficulty `MaxHealth`/`Health` is written into each fighter's damage system once, before the round goes live. During a round, only the Blueprint changes health.
- **Flags written by C++**, all existing BP variables used as the BP intends:
  - `IsInvincible` during the 0.6 s Divine Counter and the 1.4 s phase-II ascension.
  - `BlockChance` per difficulty.
- **Cosmetic cues** (sparks, sound, hit-stop, camera) are multicast after resolution and never decide outcomes.
