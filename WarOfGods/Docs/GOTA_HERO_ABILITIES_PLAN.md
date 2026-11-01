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
