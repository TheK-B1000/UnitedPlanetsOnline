# Combat Milestone 01 — Trial of Champions: Sovereign Champion

**Mode:** Arena → Trial of Champions → *Enter the Arena* → Hero Select (champion difficulty) → Titan's Quarry, best of three vs the AI.
**URL:** `ThirdPersonMap?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3?Opponent=AI?Hero=UPAWarden?Difficulty=Veteran`
**Status:** playable end to end in standalone `-game`. Verified by scripted runs; see `GOTA_AI_DUEL_TEST_RESULTS.md`.

## 1. Principle: the Blueprint combat stays the combat

Nothing in this milestone replaces the original Blueprint foundation, and no Blueprint asset was edited.

| Concern | Owner (unchanged) | What the milestone adds |
|---|---|---|
| Swings, combos, hit traces | `BP_Player`, `BPC_Attacks`, `BP_Enemy_Melee` | — |
| Damage, health, death | `BPC_DamageSystem::TakeDamage` (only health authority) | Observes it. The Divine Counter applies its damage **through** the champion's own `TakeDamage`. |
| Block | `BP_Player::StartBlock` → `Montage_Shield_Block` "Block" notify sets `IsBlocking` | — |
| Parry | `ParryWindow` notify window (0.09–0.37 s after block start). `MC_OnBlocked` → `ParryAttack` riposte (5 dmg, Stagger). | Recognised as a **Perfect Parry**: momentum and cue. |
| Dodge | `BP_Player::DodgeInDirection` (Alt + move): blink teleport with 0.2 s `IsInvincible` | Recognised as a **Perfect Dodge** when timed to the attack's impact notify. |
| Enemy AI | `AIC_Enemy_Base` + `BT_Enemy_Melee` (tokens, attack selection, reactive `TryToBlock`) | `UGOTAChampionDirectorComponent` layers fair reads, guard chain, guard break, phase II and Sovereign Surge on top, by calling the enemy's own BP functions. |
| Divine Momentum | `UStatsComponent::DivineMomentum` (C++, already existed) | Earned from Blueprint outcomes; spent on the Divine Counter. |

No GAS. No second health or damage system. No new arena. No extra PlayerController for the bot.

## 2. The fight loop

1. **Read**: the champion's attacks have visible wind-ups. The `Montage_Sword_Slash` impact is at 0.58 s, `Montage_Melee_GroundSmash` at 1.22 s, and `Montage_Sword_SpinningAttack` at 1.22 s then every 0.5 s.
2. **Answer**:
   - **Block** (RMB): a hit inside the first 0.09–0.37 s of the guard is a **Perfect Parry**. The Blueprint ripostes and the challenger gains +20 momentum.
   - A later block is a plain **Block** (+4).
   - **Dodge** (Alt + move) up to 0.35 s before impact is a **Perfect Dodge** (+15) if the attack then misses.
   - The GroundSmash is unblockable. It is announced with "UNBLOCKABLE — EVADE" and must be dodged.
3. **Punish**: landed hits build momentum (+3).
   - **Shield overload** is the champion's weak point. Three blocked hits within 5 s (Veteran) shatter its guard: "GUARD BROKEN".
   - A guard break staggers the champion (`HitResponse(Stagger)`), disables blocking for 2.5 s and grants +15 momentum.
4. **Spend**: at 50 momentum, **Q** (gamepad Y) unleashes the **Divine Counter**:
   - The challenger faces the champion and gets 0.6 s of invulnerability.
   - It swings (`MeleeAttack`) and deals 25 unblockable Stagger damage through the champion's `BPC_DamageSystem`.
   - Damage is ×1.4 (35) while the guard is broken.
   - Range 380 uu. It is refused (with a HUD reason) without enough momentum or out of range.
5. **Phase II** — at 50 % health the Sovereign Champion ascends:
   - "THE SOVEREIGN CHAMPION ASCENDS", 1.4 s invulnerable via the BP's `IsInvincible`.
   - Then a telegraphed **ascension slam** (`GroundSmashAttack`).
   - From then on: faster reads (×0.8), +15 % defend chance, +10 % move speed, and a firmer reactive block.
   - Periodic **Sovereign Surge**: 1.1 s stand-and-glare telegraph ("SOVEREIGN SURGE — EVADE!"), then the unblockable slam. It's a dodge check that feeds Perfect Dodge.

Momentum resets each round (fresh pawns). A round ends on a knockout or the 90 s clock; on time, the healthier fighter wins.

## 3. Sovereign Champion behaviour (fair by construction)

The champion director runs **server-only** at 20 Hz on top of the existing behaviour tree:

- **Visible wind-up reads.** It reacts only to what a human opponent could see:
  - the challenger's replicated `Attacking` flag turning on, or a new swing montage/section starting.
  - It never reads input, button state or future intent.
  - Reaction is delayed by the difficulty's `ReactionSeconds`, with a hard floor of 0.2 s.
- **Answer.** With `DefendChance` it raises its guard (the enemy BP's own `StartBlock`) or sidesteps (`LaunchCharacter` lateral hop).
  - It doesn't defend while committed to its own attack. That window is the player's opening.
  - It doesn't defend beyond 340 uu.
- **Guard chain.** After it blocks a hit it is "braced" for 1.6 s and likely re-raises its guard against the follow-up (`GuardChainChance`).
  - This makes pressure build, so the shield-overload weak point is reachable by sustained aggression.
- **Reactive block (original).** `BP_Enemy_Base::TakeDamage → TryToBlock` still rolls `BlockChance` after a hit; the director only sets the value per difficulty.
- **Telegraphs.** Every GroundSmash (BT- or director-initiated) is announced to the challenger.

## 4. Architecture

```
ADuelGameMode (server)
  InitGame        ?Difficulty=Recruit|Veteran|Ascendant
  RespawnFighters → InitDuelCombat()
      ├─ UGOTADuelFighterComponent on each fighter (replicated)
      │     challenger: OnBlocked binding (BPC_DamageSystem), perfect-dodge watcher,
      │                 momentum mirror, Q → Server_RequestDivineCounter
      │     both:       NetMulticast cue (sparks, sound, hit-stop, camera impulse)
      ├─ UGOTAChampionDirectorComponent on the champion (server only)
      └─ FGOTABlueprintCombat::ConfigureHealth (spawn-time MaxHealth per difficulty)
ADuelGameState  ChampionPawn · Difficulty · ChampionPhase · bChampionGuardBroken (replicated)
                OnDuelCombatCue (local delegate → HUD callouts)
UGOTADuelHUDWidget  momentum bar + "DIVINE COUNTER READY — PRESS Q", champion state, callouts, controls hint
AGOTAPlayerCameraManager::AddImpactKick  local procedural impulse (Camera Shake setting, Reduced Motion)
```

Files:
- `Source/WarOfGods/Public|Private/Arenas/Duel/GOTADuelCombat.h/.cpp` (new).
- `DuelGameMode`, `DuelGameState`, `DuelTypes` (difficulty, cue enum).
- `GOTADuelHUDWidget`, `GOTAPlayerCameraManager`.
- `GOTAMenuSubsystem` (difficulty in the launch URL), `GOTAHeroSelectWidget` (`Btn_Difficulty`), `GOTAGameUserSettings::FightAIDifficulty`.

### Binding to Blueprint delegates

`FGOTABlueprintCombat::BindDelegate` binds C++ `UFUNCTION`s to `BPC_DamageSystem.OnBlocked(bool CanBeParried, Actor DamageCauser)`. It only binds after checking that the parameter layouts match exactly. The log line `Fighter … OnBlocked bound=1` confirms it at runtime.

### Cosmetic rules

Cues are multicast **after** the Blueprint has resolved the hit; they never feed back into outcomes.

| Cue | Effect |
|---|---|
| Blocked | `P_Sparks_C`, `axe-block`, 50 ms hit-stop, small camera impulse |
| Perfect Parry | `P_Sparks_A`, 90 ms hit-stop, impulse |
| Perfect Dodge | chime, light impulse |
| Guard Break | `P_Sparks_D`, 90 ms hit-stop, strong impulse |
| Phase II / Surge / Unblockable | `energy-charge-up` telegraph |
| Divine Counter | `P_Sparks_A`, `sci-fi-explosion`, 110 ms hit-stop, strongest impulse |

**Hit-stop** sets `GlobalAnimRateScale` on the two fighters' meshes only, never global time dilation.
- It is applied equally to both combatants, so relative timing is preserved.
- On the authority it does delay both fighters' next notifies by up to 0.11 s. That is the documented trade-off.
- `gota.Duel.HitStop 0` disables it.

**Camera impulse** is local to the involved player's view and scaled by *Camera Shake Intensity*. *Reduced Motion* disables it.

### Network readiness (listen server)

| Item | How |
|---|---|
| Health | Blueprint-replicated (unchanged) |
| Divine Counter request | `Server_RequestDivineCounter` (reliable); server validates momentum, range, phase |
| Denial | `Client_CounterDenied` |
| Momentum display | Replicated mirror `UGOTADuelFighterComponent::Momentum` (owner: `UStatsComponent`, server) |
| Champion state | Replicated `ADuelGameState` fields; HUD resolves the champion via `GS->ChampionPawn` (works on clients) |
| Cues | `NetMulticast, Unreliable` (cosmetic) |
| Director | Server only; drives the AI pawn whose movement/montages replicate through the existing BP RPCs |

Not yet verified: a remote client in a duel (see the test results).

## 5. Blueprint node instructions (optional editor follow-ups — not required to play)

None of these is needed for the milestone. They're the editor-only steps the automation intentionally didn't take, so as not to modify the original Blueprints.

1. **Sustained block while RMB is held.** `Montage_Shield_Block` is 1.38 s and ends the block on completion.
   - To hold indefinitely, open `/Game/Animations/Melee/Montage_Shield_Block`.
   - Add a section `Hold` from ~0.40 s to the end, then in the Montage Sections panel set `Hold → Hold` (loop).
   - `EndBlock` (RMB released) already calls `StopAnimMontage`, so no graph change is needed.
2. **Divine Counter on Enhanced Input** (instead of the legacy key binding used now):
   - Create `IA_DivineCounter` (Digital) and map **Q** and **Gamepad Face Button Top** in `IMC_Default`.
   - In `BP_Player` Event Graph add `EnhancedInputAction IA_DivineCounter (Started)`.
   - Wire it: `Get Component by Class (GOTADuelFighterComponent)` → `Is Valid` → `Request Divine Counter`.
   - Then remove the `BindKey` lines in `UGOTADuelFighterComponent::TryBindLocalInput`.
3. **Champion presentation.** The Sovereign Champion currently uses `BP_Enemy_Melee` (mannequin, sword + shield).
   - For a distinct look, duplicate it as `BP_SovereignChampion` and swap the mesh/material.
   - Set `ADuelGameMode::AIChampionClass` (class defaults) to the duplicate.
   - The director needs only the same BP API: `StartBlock`, `EndBlock`, `HitResponse`, `GroundSmashAttack`, `Attacking`, `BlockChance`.
4. **Phase II roar.** No roar animation exists in the project, so the ascension uses the invulnerable glare and slam. If a roar montage is added:
   - Call `Play Anim Montage` on the champion in `UGOTAChampionDirectorComponent::EnterPhaseTwo`.
   - Or add a BP event `OnAscend` and call it with `FGOTABlueprintCombat::Call(Champion, "OnAscend")`.

## 6. Controls (shown in the HUD)

| Action | Keys |
|---|---|
| Attack | LMB |
| Block (parry on impact) | RMB |
| Dodge | Alt + move |
| Divine Counter | Q / gamepad Y |

Stance keys `1` (melee) and `2` (magic) are the original BP bindings. The duel equips melee automatically each round.

## 7. Debug tools (non-shipping)

| Command | Purpose |
|---|---|
| `gota.Duel.DebugOverlay 1` | On-screen champion state: phase, hp, guard, shield pressure, last decision; challenger momentum and flags |
| `gota.Duel.Director state` | Log the director state |
| `gota.Duel.Director block\|sidestep\|guardbreak\|phase2\|surge` | Force a behaviour |
| `gota.Duel.State <slot>` | One line: hp, momentum, invulnerable, blocking, parry window, montage@position |
| `gota.Duel.Bot off\|parry\|dodge\|fight [slot] [blockLead]` | Test-only challenger driver through the same BP functions as input. Reads montage timing (a test oracle, never used by the AI). |
| `gota.Duel.Counter <slot>` | Request a Divine Counter (same path as Q) |
| `gota.Duel.Momentum <slot> <n>` | Test grant |
| `gota.Duel.Notifies <montage path>` | Dump notify timings |
| `gota.Duel.HitStop 0\|1` | Toggle hit-stop |
| `Tools/GOTA_ExportCombatGraphs.py` + `Tools/GOTA_GraphFlow.py` | Export and trace the Blueprint graphs offline (`Saved/GraphAudit`) |
