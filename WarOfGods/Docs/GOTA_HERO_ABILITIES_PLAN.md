# Hero abilities: framework audit and signature-ability plan

This is the Polish Pass, Priority 4, dated 2026-10-31. It is an audit plus a plan only: no new abilities were implemented in this pass.

## How the existing framework works

It was built in Divine Combat Pass 1 and covers Drakon, Nyx and Varoth. The main parts:

| Part | Where | Role |
|---|---|---|
| `FGOTAAbilityDef` | `Public/Abilities/GOTAAbilityTypes.h` | One ability as data: kind, damage, cooldown, momentum cost, cast/recovery time, range, radius, arc, hit count/interval, status, block/interrupt rules, animation, FX, sound, AI hints |
| `UGOTAAbilitySet` | same file; assets in `/Game/Heroes/Abilities/DA_AbilitySet_<Hero>` | Up to 4 abilities (slot 3 is the ultimate by convention), plus the hero's `MaxHealthScale` and AI global gap |
| `UGOTAHeroDefinition::AbilitySet` | `DA_Hero_<Hero>` | Links a hero to its set |
| `UGOTAHeroAbilityComponent` | `Public/Abilities/GOTAHeroAbilityComponent.h` | Per fighter: activation, validation, timeline, statuses, AI use, HUD read side |
| `Tools/GOTA_BuildHeroAbilities.py` | — | **Source of truth.** All balance and presentation values live here; it writes the sets |
| `gota.Ability.*` | `Private/Abilities/GOTAAbilityConsoleCommands.cpp` | `Use`, `Dump`, `AI`, `Place`, `Slomo`, `Nav`, `AnimDebug` |

### Ability kinds (`EGOTAAbilityKind`)

| Kind | Behaviour |
|---|---|
| `MeleeArc` | Arc in front |
| `Charge` | Dash; the first enemy struck is knocked back |
| `SelfGuard` | Self status |
| `AreaBurst` | Radius around the caster after the cast |
| `Teleport` | Validated, collision-safe destination |
| `MultiHit` | N strikes, each re-checked in front |
| `Pull` | Range and line-of-sight checked |
| `Aura` | Debuff in a radius |

**Statuses:** Unbreakable, Veiled and Dread.

### Registering a hero's abilities

1. Add the hero's entry to `HEROES` in `Tools/GOTA_BuildHeroAbilities.py`. Use the card's ability names, and use animations from the hero's own Paragon pack. They play on the **body's** skeleton.
2. Run the script. It writes `DA_AbilitySet_<Hero>` and sets `DA_Hero_<Hero>.AbilitySet`.
3. Nothing else is needed. `ADuelGameMode::SetPlayerDefaults` (player) and the foe setup (AI) call `UGOTAHeroAbilityComponent::ApplyToPawn` with the **server-accepted** HeroId.

### Activation and authority (unchanged contract)

1. **Input:** E/R/C/V or D-pad left/up/right/down → `RequestActivate(slot)` → `Server_Activate(slot)`. A client only ever sends a slot number.
2. **Server validation** (`ValidateServer`) checks, in order:
   - round live
   - caster alive and not busy
   - cooldown
   - momentum
   - target exists
   - range
   - line of sight
   - safe destination
   A refusal goes back to the owning client as `EGOTAAbilityDenied`, for the HUD.
3. **Server timeline:** cast time → hit(s) → recovery.
   - Damage only goes through the victim's Blueprint damage system (`FGOTABlueprintCombat::ApplyDamage`), the single health authority. Block, parry and i-frames apply there.
   - Movement (charge, teleport, pull, knockback) and statuses are server-side.
   - Casts can be interrupted unless `bInterruptible` is false.
4. **Replication:**
   - Cooldown end times, status, magnitude and HeroId are replicated properties.
   - Presentation is multicast: `Multicast_BeginCast` (body animation in the retarget anim instance's DefaultSlot, cast FX/SFX), `Multicast_Impact`, `Multicast_CastCancelled`.
5. **AI:** the same validation path. `AIPriority`, `AIMinDistance`, `AIMaxDistance` and `AIMaxOwnHealthFraction` score candidates, with `AIGlobalGap` preventing spam.

### Animating abilities

- Ability animations play **on the visible body**, as a DefaultSlot montage of `UGOTARetargetAnimInstance`. Gameplay timing comes from the server timeline, not from the animation, so a Paragon ability animation can be used as-is. Tune `AnimPlayRate` and `AnimStartTime` so the visible impact matches the hit time.
- While the body plays the ability animation, it overrides the retargeted pose, so its hands can leave the hand-IK goals. That's expected for the duration of the cast.

### Testing an ability

- **Live use:** `gota.Ability.Use <fighter> <slot>`.
- **Positioning:** `gota.Ability.Place <dist> [freeze]` positions the target; `gota.Ability.Slomo` slows time to judge timing.
- **AI:** `gota.Ability.AI <slot|on|off>`.
- **Existing scripts:** `Tools/Tests/ability_slots_<Hero>.txt` (all slots, hit/miss and refusals) and `ability_film_<Hero>.txt` (screenshots).
- **To verify:**
  - It hits inside its range/arc and misses outside.
  - It's refused correctly (cooldown, out of range, no line of sight).
  - It's interrupted by stagger where allowed.
  - A listen-server client sees the animation and FX.
  - Damage lands through the Blueprint system (blocked when `bCanBeBlocked`).

## Signature ability per god (first implementation milestone)

One ability per god, taken **from the hero card** (`Tools/GOTA_HeroContent.py`), mapped onto an existing kind so the first milestone needs data only, no new code.

| God (body) | Signature (card) | Card text | Kind | Body animation | Notes |
|---|---|---|---|---|---|
| Zephyr (Kwang) | **Gale Rush** | "High-speed dash attack." | `Charge` | `ParagonKwang/.../Ability_RMB` | Fast and short; knockback |
| Lysara (Aurora) | **Gravity Well** | "Pull enemies and disrupt." | `Pull` | `ParagonAurora/.../Ability_Q` | Line-of-sight gated, like Varoth's Grasp |
| Kairos (Gideon) | **Time Lash** | "Area control. Slow enemies." | `Aura` (Dread-style slow) | `ParagonGideon/.../Burden` | A dedicated *Slowed* status would read better than Dread; that's a small code addition |
| Orion (Steel) | **Gravity Slam** | "Pull and crush enemies in an area." | `AreaBurst` | `ParagonSteel/.../Steel_Ability_GroundSmash_Start` + `_End` | The pull component could come later (Pull then AreaBurst) |
| Drakon (Greystone) | **Titan Charge** | "High movement. Breaks guard." | `Charge` | existing | Already exists as *Warbringer*; rename to the card name and make it unblockable to match "breaks guard" |
| Nyx (Countess) | **Shadow Step** | "Short-range teleportation." | `Teleport` | existing | Already exists as *Shadowstep* |
| Varoth (Sevarog) | **Inferno Slam** | "Area damage. Launch enemies." | `AreaBurst` | existing | Closest existing ability is *Oblivion* (ultimate); a non-ultimate Inferno Slam would match the card |
| Ayla | — | — | — | — | No body (Missing Asset); deferred |

### Canon alignment issue (found in this audit)

The Pass 1 sets for Drakon, Nyx and Varoth use names that **aren't on their hero cards**:

| God | Implemented | Card |
|---|---|---|
| Drakon | Titan's Cleave, Warbringer, Unbreakable, Divine Reckoning | Dragon Roar, Titan Charge, Molten Strike, Unchained Fury |
| Nyx | Shadowstep, Nightfall, Veil of Night, Eclipse | Shadow Step, Mind Spike, Void Shield, Illusion, Soul Drain |
| Varoth | Soul Reaver, Grasp of the Void, Dread Aura, Oblivion | Inferno Slam, Molten Spikes, Ashen Charge, World Burn, Colossus Form |

Recommendation: realign names and descriptions to the cards when those sets are next touched. The mechanics can mostly stay. Nothing was renamed in this pass.

### Order of work for the milestone

1. Add the four new signature entries (Zephyr, Lysara, Kairos, Orion) to `GOTA_BuildHeroAbilities.py` in slot 0, with conservative prototype numbers.
2. Run `ability_slots_<Hero>.txt`-style tests for each: hit/miss, refusal, interrupt, listen-server presentation.
3. Keep the heroes In Development until their full sets exist and a complete match with abilities passes.

## Milestone progress

### Zephyr: Gale Rush (implemented 2026-10-31, In Development)

**Data:** `Tools/GOTA_BuildHeroAbilities.py` → `HEROES["Zephyr"]`, slot 0. Build with `GOTA_HEROES=Zephyr`, which updates only that set and leaves the status overlay materials alone.

| Property | Value |
|---|---|
| Kind | `Charge` |
| Damage | 16 |
| Cooldown | 8 s |
| Cast / recovery | 0.15 s / 0.35 s |
| Dash | 700 cm in 0.3 s |
| Knockback | 750 |
| Blockable | yes |
| Body animation | Kwang `Ability_RMB` |
| FX | Kwang after-image (cast) and primary impact |
| AI use | 3.5–7 m |

**Results:**

| Check | Result |
|---|---|
| Hit (target 5 m, `ability_galerush.txt`) | 16 damage, champion knocked about 4 m |
| Re-use during cooldown | Refused (`Cooldown`) |
| Target beyond range (13 m) | Gap-closer: dashes toward the target and stops after about 6.3 m without a hit |
| Sidestep after launch (`ability_galerush_side.txt`, `gota.Ability.PlaceOnCast`) | **Miss**; control dash right after: hit |
| Outside a live round | Refused (`NotLive`) |
| AI (`?Foe=Zephyr`) | The AI Zephyr chose Gale Rush at 5.5 m and hit the player for 16, in each round |
| Listen server + client | The host's Gale Rush hit the client (100 → 84). The client saw Zephyr's Kwang body play `Ability_RMB` |

**Framework fixes made along the way:**
- A **missed charge now stops at the end of its dash.** It used to keep sliding on its launch velocity: 12.3 m for a 7 m dash. This also applies to Drakon's Warbringer.
- `GOTA_BuildHeroAbilities.py` now updates sets in place instead of deleting them. A set is referenced by its hero definition, so the delete would fail and the script stopped, the same failure the boss builder had.

**Card note:** the card says "twin wind blades", while Kwang has a single sword.

### Lysara: Gravity Well (implemented 2026-11-01, In Development)

**Data:** `Tools/GOTA_BuildHeroAbilities.py` → `HEROES["Lysara"]`, slot 0. Build with `GOTA_HEROES=Lysara`.

| Property | Value |
|---|---|
| Kind | `Pull`, needs line of sight |
| Damage | 10, with a stagger ("disrupt") |
| Cooldown | 12 s |
| Cast / recovery | 0.5 s / 0.4 s |
| Range | 850 cm |
| Pull | up to 1600 |
| Body animation | Aurora `Ability_R_InPlace` |
| FX | Aurora ultimate warm-up (cast) and freeze whirlwind (impact) |
| AI use | 3.8–8.4 m |

**Results:**

| Check | Result |
|---|---|
| Hit (target 6.5 m, `ability_gravitywell.txt`) | 10 damage; target pulled to about 3 m from Lysara |
| Re-use during cooldown | Refused (`Cooldown`) |
| Target at 11 m | Refused (`OutOfRange`) |
| Target escapes during the cast (`gota.Ability.PlaceOnCast`) | **Fizzles** ("pull target lost") |
| Control hit (next round) | Hit, pulled |
| AI (`?Foe=Lysara`) | The AI Lysara chose Gravity Well at 6.5 m and 8.1 m and hit the player |
| Listen server + client | The host's Gravity Well hit the client (100 → 90) and **pulled the remote player** from 6.5 m to 3 m. The client saw the Aurora body play `Ability_R_InPlace` |

**Not covered by a dedicated test:** the line-of-sight refusal. It's the same validation Varoth's Grasp of the Void uses (verified in Pass 1).

**Card note:** the card names "Celestial Twin Rings" as her weapon, while Aurora carries an ice blade.

### Kairos: Time Lash, and the new Slowed status (implemented 2026-11-01, In Development)

**`EGOTAStatus::Slowed`** (new, appended to the enum):
- Scales **movement speed only**, by (1 − Magnitude). Damage, attack speed, animation rate and cooldowns are untouched. Only Dread reduces ability damage.
- It's applied on every peer (server and owning client) so movement prediction agrees, and it shows the blue `M_GOTA_Overlay_Slowed` overlay plus a HUD line ("TIME-LASHED - SLOWED").
- **Restore fix, shared by Slowed and Dread:** if anything else changes `MaxWalkSpeed` during the status (e.g. a Blueprint stance change), that value becomes the new base, and the **latest** base is restored on expiry. Dread used to restore a stale speed.

**Time Lash:** `HEROES["Kairos"]`, slot 0, built with `GOTA_HEROES=Kairos`.

| Property | Value |
|---|---|
| Kind | `Aura`, radius 450 |
| Damage | 6 |
| Effect | Slowed, 40% for 3 s |
| Cooldown | 12 s |
| Cast / recovery | 0.5 s / 0.4 s |
| Body animation | Gideon `Burden` (1.03 s) |
| FX | Gideon portal cast and portal entrance |
| AI use | 0–4.3 m |

`PortalBlast` was tried first and rejected: it's only 0.03 s long, so there was no visible cast.

**Results:**

| Check | Result |
|---|---|
| Hit at 3 m (`ability_timelash.txt`) | 6 damage; target walk speed 500 → **300** for 3 s → **500** restored. Animation rate and time dilation stay 1.00 |
| Target outside the radius (7 m) | Cast, nothing affected |
| Re-use during cooldown | Refused |
| Slowed target's own ability (`status_slowed.txt`, deterministic via `gota.Ability.Status`) | Gale Rush casts normally while slowed, with the normal 8 s cooldown |
| Stance change while slowed | Magic stance 200 → **120** (adopted as the new base) → expiry restores **200**, not the stale 430 → Melee 430 |
| Death during casting (`ability_death_cast.txt`) | Cast cancelled; no status reaches the target; the next round starts clean |
| AI (`?Foe=Kairos`) | The AI Kairos chose Time Lash at 3–4.2 m and slowed the player (430 → 258) |
| Listen server | The remote player's own client logged walk 300 under Time Lash, 500 after expiry, then 300 again under a second slow, so prediction agrees and the restore replicates. The client saw Kairos play the cast |

**Limitation (unchanged design):** a fighter holds one status at a time, so a new status replaces the current one.

### Orion: Gravity Slam (implemented 2026-11-01, In Development)

A **close-range, telegraphed area crush**, deliberately unlike Gravity Well: untargeted, centred on Orion, no pull, a visible wind-up and one ground-impact damage moment.

**Data:** `HEROES["Orion"]`, slot 0, built with `GOTA_HEROES=Orion`.

| Property | Value |
|---|---|
| Kind | `AreaBurst`, radius 330 |
| Damage | 20, with a stagger |
| Knockback | none ("crush") |
| Blockable | **no** |
| Interruptible | **yes, during the wind-up** |
| Cooldown | 10 s |
| Wind-up / recovery | 0.7 s / 0.6 s |
| Body animation | Steel `GroundSmash_End` at 0.229× |
| FX | Steel melee charge glow (wind-up), ultimate ground impact |
| AI use | 0–3 m |

**Lining up the impact:** the slam lands 0.16 s into the animation (hand height sampled over the clip: 3 m overhead → ground at 0.16 s). At 0.229× it lands at 0.70 s, exactly the server's damage moment. A slow-motion film (`ability_gravityslam_film.txt`) shows fists raised overhead at 0.42 s, the impact at 0.74 s game time, then recovery.

**Results** (`ability_gravityslam.txt` and `ability_gravityslam_ai.txt`):

| Check | Result |
|---|---|
| Target at 2 m | 20 damage |
| Re-use during cooldown | Refused |
| Target at 4.5 m (outside the radius) | Cast, no hit |
| Target leaves 0.35 s into the wind-up (`PlaceOnCast`) | **No hit** (the damage window is avoidable) |
| Orion damaged during the wind-up | **Cancelled (interrupted)**, no damage |
| AI Orion against a **blocking** player | Lands anyway (`landed=1`, 75 → 55): unblockable as designed |
| AI use | Chose Gravity Slam at 1.0 m and 2.2 m |
| Listen server | The host's slam hit the client (20). The client saw Orion's Steel body play the slam |

### Gravity Well follow-ups (2026-11-01)

Script: `ability_gravitywell_los.txt`. Dev helpers: `gota.Test.SetupBlocker` finds real arena geometry at runtime (Titan's Quarry dressing loads in game), and `gota.Test.Encroach` checks whether a fighter's capsule overlaps solid geometry.

| Check | Result |
|---|---|
| **Line of sight:** target 5.4 m away (in range) behind an 88 cm block | **Refused (`NoLineOfSight`)**, no cooldown spent. Offset placements (up to ±1.6 m) were refused the same way |
| **Pull into a wall:** caster's back 1.2 m from a wall, target 6.5 m in front | Pulled to 2.9 m in front of the caster; target **not in geometry** before or after |
| **Why it can't tunnel** | A pull needs line of sight at cast **and** at resolve (refused or fizzles otherwise), so it never starts with geometry between the fighters. The pull itself is a `LaunchCharacter` through the normal swept character movement, so it collides and can't pass through |
| Grazing (sightline clears an edge but the body would clip it) | Not reproduced: the Quarry's obstacles near usable floor are too broad for a sightline to graze them |

**Lysara's weapon:** her canonical weapon is the **Celestial Twin Rings**; Aurora's ice blade is a stand-in. Recommended direction:
- Floating ring meshes attached to hand or orbit sockets on her body.
- Gravity Well's cast shows the rings orbiting and flaring.
- Needs ring meshes (modelled, or a suitable licensed asset) and a cast animation layer. The ability's data and timing don't change.

## Ability regression suite

`Tools/GOTA_AbilityRegression.py` launches the game per case, runs a `Tools/Tests` script, and checks the log for expected and forbidden lines. It prints PASS/FAIL per check and exits with code 1 on any failure.

```
python Tools/GOTA_AbilityRegression.py                 # editor -game
python Tools/GOTA_AbilityRegression.py --packaged      # packaged Win64 (GOTA_PACKAGE_DIR); copies the test scripts in
python Tools/GOTA_AbilityRegression.py --only TimeLash
python Tools/GOTA_AbilityRegression.py --check-only    # re-evaluate the last logs
```

**Coverage per signature ability:**
- successful activation (damage/effect values asserted)
- cooldown refusal
- an invalid target: out of range or radius, sidestep, no line of sight, escape mid-cast
- death during the cast (no effect lands)
- multiplayer: the host's hit lands on a remote player and the client sees the body animation
- every case also asserts no crash

Cases are data in `CASES` and `MP`; new abilities add entries there. Scripts use `gota.Ability.ResetCooldowns` instead of waiting out cooldowns, so every case fits inside one round in both editor and packaged runs.

**Results (2026-11-01):** **editor 16 of 16, packaged Win64 16 of 16** (packaged evaluated with `--packaged --check-only` over the latest packaged-run logs, all from the current package).

Notes from getting there:
- The first packaged pass failed two cases on **test timing**, not gameplay: cooldown waits outran the round timer, and the 3 s slow expired before a stretched wait ended.
- The fix: `gota.Ability.ResetCooldowns` replaces cooldown waits; Time Lash has a lean script with no camera or screenshots; exact walk-speed values are asserted by the deterministic 8 s `status_slowed.txt` case.
- The test runner now caps each frame's script-time step at 0.5 s instead of 0.1 s, so waits no longer crawl in an unfocused packaged window. This lands in the next package.

## Open issue: intermittent D3D12 crash on the results screen

Seen in 5 of 7 long editor runs and in 1 of 16 packaged suite runs, always while idling on the post-match results screen. The packaged crash has symbols:

```
EXCEPTION_ACCESS_VIOLATION reading 0xffffffffffffffff
FD3D12ContextCommon::BindDiagnosticBuffer <- FD3D12DescriptorCache::SetSamplerTable
<- FD3D12CommandContext::RHISetShaderParameters <- ... FRHICommandListExecutor::FTranslateState::Translate_ExecuteCommandList
(Background Worker: RHI parallel translate)
```

- `BindDiagnosticBuffer` dereferences `PSOCommonData->RootSignature` when the root signature changes (`D3D12StateCache.cpp:531`). A garbage pointer there means a pipeline state whose root signature is no longer valid, while commands are translated in parallel.
- This is engine-side; no game code is on the stack. The results widget is a plain image plus text.
- **Candidate mitigation (not applied):** `r.RHICmd.ParallelTranslate.Enable=0`. It costs some rendering throughput, and at this repro rate it needs a long A/B soak before it can be called a fix.
- Next steps: install editor debug symbols, run a long idle-results soak with and without the cvar, and if it reproduces, report it upstream (UE 5.8, D3D12, RTX 5070 / driver 610.88).

## Card alignment: Drakon, Nyx, Varoth (2026-11-01)

The Pass 1 sets now carry their **hero-card names and descriptions** (`Tools/GOTA_HeroContent.py`). Where an existing mechanic already fit a card ability it was kept and renamed. Where a Pass 1 ability had no card equivalent, it was replaced by a card ability that an existing kind can express. Card effects the framework can't do yet are listed in the last column.

| God | Slot | Now (card) | Was | Kind | Not implemented yet |
|---|---|---|---|---|---|
| Drakon | 0 | Molten Strike | Titan's Cleave | MeleeArc (same values) | "ignites" (burning DoT) |
| Drakon | 1 | **Titan Charge** ★ | Warbringer | Charge, now **unblockable** ("breaks guard") | — |
| Drakon | 2 | Dragon Roar | Unbreakable (not on card) | AreaBurst r400, knockback 850 | — |
| Drakon | 3 | Unchained Fury (ult) | Divine Reckoning | AreaBurst (same values) | — |
| Nyx | 0 | **Shadow Step** ★ | Shadowstep | Teleport (same) | — |
| Nyx | 1 | Mind Spike | Nightfall | MultiHit 3× (same), staggers | a true stun |
| Nyx | 2 | Void Shield | Veil of Night | SelfGuard Veiled (same) | — |
| Nyx | 3 | Soul Drain (ult) | Eclipse (5 slashes) | **Aura r450: 18 damage + Dread** ("weakens") | "steals energy" |
| Varoth | 0 | **Inferno Slam** ★ | Soul Reaver (arc swing) | **AreaBurst r350, 22 damage, launch 650** | — |
| Varoth | 1 | Ashen Charge | Grasp of the Void (not on card) | Charge (Sevarog `Speedburst`) | — |
| Varoth | 2 | World Burn | Dread Aura | Aura + Dread (same) | burning DoT |
| Varoth | 3 | Colossus Form (ult) | Oblivion | SelfGuard Unbreakable 6 s (½ damage, no stagger) | power / area-damage bonus |

★ = the god's signature ability for the "one signature per god" milestone.

**Cards left without a slot:** Nyx's *Illusion* (decoys, no mechanic yet), Varoth's *Molten Spikes* (5th card ability), and Lysara, Kairos, Zephyr and Orion's other card abilities (one signature each so far).

**Balance note:**
- Changed values: Titan Charge is now unblockable; Dragon Roar, Inferno Slam, Ashen Charge, Soul Drain and Colossus Form are new mechanics for their slots.
- Unchanged values: Molten Strike, Unchained Fury, Shadow Step, Mind Spike, Void Shield and World Burn.

**Verified (editor):**
- **Every slot** (`ability_slots_<Hero>.txt`) casts and lands under its new name: Molten Strike 22, Titan Charge 14, Dragon Roar 6, Mind Spike 3×9, Void Shield → Veiled 1.3 s, Shadow Step teleports, World Burn 5 + Dread, Ashen Charge 16, Inferno Slam cast.
- **Ultimates** (`ability_ultimate.txt`, with momentum granted):
  - Unchained Fury hits.
  - Soul Drain hits and applies Dread 4 s ×0.35.
  - Colossus Form gives Unbreakable 6 s ×0.5.
- **Signatures in the regression suite**, each with activation, cooldown, an invalid target or sidestep, death during cast, and multiplayer: **Titan Charge 4/4** (including landing on a **blocking** player: "breaks guard"), **Shadow Step 3/3** (blink behind, forward blink with nobody in range), **Inferno Slam 3/3**. Every god now has a signature ability in the suite: **26 of 26 checks pass** in the editor.

**Builder fix:** `GOTA_BuildHeroAbilities.py` now raises an error when a set fails to save. Drakon's first rebuild silently didn't save because a running test game held the package open.
