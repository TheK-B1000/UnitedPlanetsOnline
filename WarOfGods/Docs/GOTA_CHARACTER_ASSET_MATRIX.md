# Character asset matrix

This is the installed-asset inventory and the character assignments for the roster pass.

- **Audit date:** 2026-10-31.
- **How it was produced:** an editor scan of the asset registry, run from `scratchpad/inventory_probe*.py`. Every path below was loaded from the project, not assumed.
- **Status terms:**
  - **Missing Asset**: nothing suitable is installed.
  - **Imported**: the asset is in the project.
  - **Integrated**: the body is wired to the hero or enemy definition.
  - **Combat Tested**: a full match has been verified with it.
  - **Packaged Tested**: also verified in a packaged Win64 build.
  - **Playable**: selectable in normal play.

> **Licensing:** Paragon packs are free from Epic (Fab). They may be used in Unreal projects, but the Paragon name must not be used to market the game. Windwalker Echo is an Epic sample. None of this source content is committed to the public repo; see `GOTA_THIRD_PARTY_ASSETS.md`.

## Installed character content

Every Paragon hero pack has its own skeleton, Anim Blueprint, 144–234 animation sequences, 3–5 montages and alternate skins. All of them use UE4-mannequin-style bone names (`pelvis`, `spine_01-03`, `clavicle`, `upperarm`, `hand`, `thigh`, `calf`, `foot`, `neck_01`, `head`), so the project's existing Paragon IK chain set fits them.

| Pack | Main mesh | Skeleton | Anim BP | Anims | Skins | Notes |
|---|---|---|---|---|---|---|
| Greystone | `/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone` | Greystone_Skeleton | Greystone_AnimBlueprint | 154 | 4 | Sword, shield bones |
| Countess | `/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/SM_Countess` | S_Countess_Skeleton | Countess_AnimBlueprint | 234 | 4 | |
| Sevarog | `/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog` | Sevarog_Skeleton | Sevarog_AnimBlueprint | 144 | 3 | About 3.1 m including hammer; no ball bones |
| Kwang | `/Game/ParagonKwang/Characters/Heroes/Kwang/Meshes/Kwang_GDC` | Kwang_Skeleton | Kwang_AnimBlueprint | 177 | 5 (Albino, Rosewood, Sunrise, Manbun, Frostwalker) | Sword; `weapon_l/_r` bones |
| Aurora | `/Game/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora` | Aurora_Skeleton | Aurora_AnimBlueprint | 191 | 4 (FrozenHearth, GlacialEmpress, MoonCrystal, SpringCeremony) | Ice blade; metacarpal fingers |
| Gideon | `/Game/ParagonGideon/Characters/Heroes/Gideon/Meshes/Gideon` | Gideon_Skeleton | Gideon_AnimBlueprint | 201 | 4 (Inquisitor, Mephisto, Tough, Undertow) | Caster, unarmed; lots of cloth bones |
| Steel | `/Game/ParagonSteel/Characters/Heroes/Steel/Meshes/Steel` | Steel_Skeleton | Steel_AnimBlueprint | 186 | 5 | About 3.5 m bounds; arm shield bones; 2-joint fingers |
| Feng Mao | `/Game/ParagonFengMao/Characters/Heroes/FengMao/Meshes/FengMao_GDC` | FengMao_Skeleton | FengMao_AnimBlueprint | 196 | 3 | Armoured swordsman |
| Sparrow | `/Game/ParagonSparrow/Characters/Heroes/Sparrow/Meshes/Sparrow` | Sparrow_Skeleton | Sparrow_AnimBlueprint | 152 | 7 | Archer (bow) |
| Serath | `/Game/ParagonSerath/Characters/Heroes/Serath/Meshes/Serath` | Serath_Skeleton | Serath_AnimBlueprint | 177 | 4 | Winged |
| Wraith | `/Game/ParagonWraith/Characters/Heroes/Wraith/Meshes/Wraith` | Wraith_Skeleton | Wraith_AnimBlueprint | 229 | 2 | Sniper rifle |
| Rampage | `/Game/ParagonRampage/Characters/Heroes/Rampage/Meshes/Rampage` | Rampage_Skeleton | Rampage_AnimBlueprint | 148 | 3 | Monster; own skeleton |
| Minions | 27 meshes (below) | 9 skeletons | **none** | 344 sequences | Dawn/Dusk | **No Anim Blueprints or montages**: they must be built |
| Windwalker Echo | `/Game/EchoContent/Characters/Echo/Meshes/Echo` | Echo_Skeleton (UE5 Mannequin layout) | post-process only | — | — | Default player body |
| UE mannequins | `SKM_Manny_Simple`, `SKM_Quinn_Simple`, `SK_Mannequin` (UE4) | SK_Mannequin | ABP_Manny / ABP_Player | | | Gameplay frame (hidden) and **training dummy only** |
| Mage | `/Game/Characters/Mage/Mesh/SKM_Mage` | SK_Mage | ABP_Mage | | | Used by `BP_Enemy_Mage` |

**Minion meshes:**
- Lane minions (Dawn and Dusk): Melee, Ranged, Siege, Super, and Core variants.
  - Melee and Ranged share the lane core skeleton.
  - Siege and Super each have their own skeleton.
  - Animation folders: Melee 44 (attacks A–E), Ranged 43 (Fire, death, hit reacts), Siege 40, Super 34.
- Jungle camp: `Buff_Black`, `Buff_Green`, `Buff_Red`, `Buff_Blue` and `Buff_White`.
  - Red has punches, uppercut and big smash.
  - Black is a biter.
  - Blue is a 0.6 m ranged sprite.
- `Prime_Helix`: a 4.8 m guardian with special attacks.

## Current enemy and boss pawns

All of these mannequin uses are **not acceptable** under the roster brief.

| Blueprint | Mesh today | Used by | Problem |
|---|---|---|---|
| `BP_Enemy_Melee` | SKM_Manny_Simple (mannequin) | **Fight AI champion**, Horde | Mannequin as a normal enemy |
| `BP_Enemy_Ranged` | SKM_Manny_Simple | Horde | Mannequin |
| `BP_Enemy_Base` | SKM_Manny_Simple | parent class | — |
| `BP_Enemy_Boss` | SKM_Manny_Simple | Horde boss wave | Mannequin boss |
| `BP_Enemy_Mage` | SKM_Mage | Horde | Has a real mesh |
| `BP_Boss_QuarryBehemoth` | SKM_Manny_Simple by default; `UGOTABossComponent` swaps in Rampage at runtime | Duel `?Boss=QuarryBehemoth` | Falls back to a mannequin boss if the Rampage pack is missing |
| `BP_Player` | SKM_Quinn_Simple (hidden gameplay frame) | every player | Visible body comes from the hero definition |

## God assignments

The approved roster comes from the hero cards in `Tools/GOTA_HeroContent.py`. Assignments are prototype bodies, not final designs.

| God | Candidate | Fit (from the cards) | Status | Abilities | Notes |
|---|---|---|---|---|---|
| Drakon (Warfall Titan, two-handed axe) | Greystone | Good: armoured titan | Integrated, Combat Tested, Packaged Tested; **In Development** | 3-ability set | Preserve |
| Nyx (Shadow Weaver, shadow blades) | Countess | Good | Integrated, Combat Tested; **In Development** | 3-ability set | Preserve |
| Varoth (Inferno Colossus) | Sevarog | Good silhouette | Integrated, Combat Tested; **In Development** | 3-ability set | Hands about 50 cm off the gameplay hands; fix before visually complete |
| Zephyr (Wind Rebellion, twin wind blades) | Kwang | Fair: agile swordsman | Imported → integration next | none | |
| Lysara (Astral Sovereign, celestial rings) | Aurora | Good: elegant divine warrior | Imported → integration | none | |
| Kairos (Chronos Warden, chrono blade) | Gideon | Fair: supernatural caster; the card says blade, Gideon is unarmed | Imported → evaluate | none | |
| Orion (Starforge Sentinel, gravity lance) | Steel | Fair: armoured sci-fi defender; shield, no lance | Imported → evaluate | none | |
| Ayla (Desert Storm; silver hair, cybernetic arms) | none | No installed asset matches | **Missing Asset** | none | Stays unavailable; needs a dedicated model |
| UPA Warden (prototype frame) | Windwalker Echo | — | **Playable**, Packaged Tested | none | Default player |

## Enemy and boss candidates

| Archetype | Candidate | Why | Needed |
|---|---|---|---|
| Melee Soldier | Lane Melee minion (Dawn) | Sword-armed humanoid; attack, hit and death sets | Anim Blueprint + montages (none ship) |
| Ranged Soldier | Lane Ranged minion | Gun-armed; Fire, hit and death sets | Anim Blueprint + projectile |
| Heavy Brute | `Buff_Red` (punch/uppercut/smash) or Lane Super | Large, distinct silhouette | Anim Blueprint |
| Magic Caster | Existing `BP_Enemy_Mage` (SKM_Mage), or Gideon/Serath bodies | The Mage already has a real mesh and casts | Review |
| Elite champion (Fight AI) | Feng Mao / Sparrow | Full hero animation sets | Body over the enemy frame |
| Boss 01: Quarry Behemoth | Rampage | Own skeleton and Anim BP | Claw reach fix |
| Training dummy | UE mannequin | Explicitly allowed | Designate explicitly |

Updated as each stage lands; see the integration log below.

## Integration log

(Stages are appended here as they complete.)

### Stage 3–4: god bodies (2026-10-31)

Rigs were built with `Tools/GOTA_BuildParagonRigs.py` (`GOTA_BODIES=Kwang,Aurora,Gideon,Steel`), 9 chains each. Bodies are set in `Tools/GOTA_HeroContent.py` → `BODIES` and applied with `Tools/GOTA_BuildHeroBodies.py`. Every check is a Fight AI duel in Titan's Quarry with `?DevHeroes=1`, using `Tools/Tests/god_regress.txt`. Hand gap means the distance from the visible body's hands to the gameplay hands.

| God | Body | Scale | Hand gap | Result | Status |
|---|---|---|---|---|---|
| Drakon | Greystone | 1.0 | 11 cm | Unchanged; full matches, packaged | In Development |
| Nyx | Countess | 1.0 | 6 cm | Unchanged | In Development |
| Varoth | Sevarog | **0.68** (was 1.0) | **6–9 cm** (was 50–85) | A scale sweep showed the gap was pure size: 0.65–0.70 is the minimum. No longer towers or hunches over the frame. Hammer in hand. | In Development |
| Zephyr | Kwang | 1.0 | 5–8 cm | Sword in hand, fights, death and respawn | In Development (no abilities) |
| Lysara | Aurora | 1.0 | 12–14 cm | Ice blade in hand, fights, death and respawn | In Development (no abilities) |
| Kairos | Gideon | 1.0 | 8–9 cm | Unarmed caster body, fights, death and respawn. The card's chrono blade isn't represented | In Development (no abilities) |
| Orion | Steel | 0.75 | **29–37 cm** | Steel's shoulders are much wider than the frame's, so no scale gets below about 30 cm; this needs IK hand goals. The deployable shield (slot `M08_Steel_Shield`) is hidden with the project material `M_GOTA_HiddenSlot`, since its unchained bones floated in the reference pose | In Development; hand IK open |
| Ayla | — | — | — | No installed asset fits | Missing Asset (preview only) |

Gameplay sword and shield stay hidden for all god bodies (`bHideAttachedWeaponMeshes`). Each body shows its own weapon. Damage is the frame's forward sweep, unchanged.

### Stage 6–7: enemy bodies and the Horde (2026-10-31)

Enemies keep their combat pawns (AI, attacks, montages and hit windows on the hidden mannequin frame) and now wear real bodies through the same runtime retarget as the gods.
- **Definitions:** `UGOTAEnemyDefinition` assets in `/Game/Enemies/Data` (primary asset type `GOTAEnemy`, never in hero selection), built by `Tools/GOTA_BuildEnemyBodies.py`.
- **Applying them:** `UGOTAEnemyBodySubsystem` dresses each enemy server-side when it spawns, matching its exact class. That covers Fight AI, Horde, Story and level-placed enemies.
- **Replication:** clients rebuild the body from the replicated ID.
- **Missing assets:** a body that fails to load is logged as an error, never a silent fallback.

| Archetype | Pawn | Body | Hand gap | Verified |
|---|---|---|---|---|
| Elite champion (Fight AI opponent) | BP_Enemy_Melee, dressed by `ADuelGameMode` | Paragon Feng Mao (glaive) | 17–19 cm | Full Fight AI rounds: fights, dies, respawns as Feng Mao |
| Melee soldier | BP_Enemy_Melee | Lane Melee minion (Dawn) | ~40 cm (pose) | Horde waves 1–5, every spawn dressed |
| Ranged soldier | BP_Enemy_Ranged | Lane Ranged minion (Dawn) | 35–42 cm (pose) | Horde waves 2+ |
| Heavy brute (Horde boss wave) | BP_Enemy_Boss | Lane Super minion (Dawn) | ~50 cm (pose) | Horde final wave |
| Magic caster | BP_Enemy_Mage | Its own `SKM_Mage` (not a mannequin) | — | Unchanged |
| Quarry Behemoth | BP_Boss_QuarryBehemoth | Rampage, own skeleton | — | Charge, slam, grasp and claws all land (below) |
| Training dummy | — | UE mannequin | — | No dummy actor exists; the mannequin's only role is the hidden combat frame |

**Minion hand gaps** don't change with scale: their arms are posed wider than the frame's. Their own blades and guns show, and the frame's sword is hidden. Enemy damage is unchanged.

**Mannequin fallbacks removed or flagged:**
- The Fight AI opponent and all Horde soldiers no longer render as mannequins.
- A `?Boss=` encounter whose body isn't installed is refused with an error; the duel falls back to the standard Fight AI (Feng Mao), never a mannequin boss.
- A god or enemy body that fails to load logs an error naming the missing asset.

**Horde fix (pre-existing bug):** `AHordeEncounterDirector::HandleEnemyEndPlay` wasn't a `UFUNCTION`, so its `OnEndPlay` binding failed on every spawn ("Unable to bind delegate"). It's now bound.

### Stage 8: Rampage claw reach (2026-10-31)

- **Cause:** Rampage's Paragon swipes are animated for point-blank range. At the windows his hands sit between 87 cm behind and 13 cm in front of his centre, while he attacked from 273–280 cm, so every claw window missed.
- **Fix:** each claw window keeps its hand-bone sphere and also strikes a target in front of him (facing ≥ 0.35) within capsule + claw radius (260 cm). That's the same rule as his Grab. Claw Combo's maximum start range is 300 → 260 cm, so he steps in first.
- **Unchanged:** damage (9 / 9 / 11), timings and cooldowns.
- **Result** (`Tools/Tests/boss_claw.txt`): 8 of 8 claw windows hit from 177–246 cm, including one true claw-bone contact. Charge, Earthshaker and Crushing Grasp land too.
- **Dodging** works by geometry (step out of the arc or behind him); it hasn't been verified live.
- **Data fix:** `Tools/GOTA_BuildBosses.py` deleted and recreated the boss definition, but the delete fails while the boss Blueprint references it, so the script had stopped early and boss data changes weren't saving. It now updates the asset in place.

### Multiplayer and packaged validation (2026-10-31)

| Check | Result |
|---|---|
| Listen server + client, Horde (`horde_mp_host.txt` / `horde_mp_client.txt`) | ✅ The client rebuilds every minion body from the replicated ID (`remote`) and sees Echo on both players |
| Packaged Win64, Fight AI full match | ✅ Feng Mao champion every round, Echo won 2–1, match completed |
| Packaged, Zephyr (DevHeroes) | ✅ Kwang body, hands 5–8 cm; Feng Mao opponent |
| Packaged, Horde | ✅ 32 enemies dressed across waves 1–5 and the boss wave; no delegate error |
| Packaged, Quarry Behemoth | ✅ Rampage body; 12 of 12 claw windows hit |

## Status summary

| Character | Asset | Status |
|---|---|---|
| UPA Warden (default player) | Windwalker Echo | **Playable**, Packaged Tested |
| Drakon | Greystone | Integrated, Combat Tested, Packaged Tested; In Development |
| Nyx | Countess | Integrated, Combat Tested; In Development |
| Varoth | Sevarog @ 0.68 | Integrated, Combat Tested; In Development. Hands now 6–9 cm |
| Zephyr | Kwang | Integrated, Combat Tested, Packaged Tested; In Development (no abilities) |
| Lysara | Aurora | Integrated, Combat Tested; In Development (no abilities) |
| Kairos | Gideon | Integrated, Combat Tested; In Development (no abilities; the card's blade isn't shown) |
| Orion | Steel @ 0.75, shield hidden | Integrated, Combat Tested; In Development (hands about 30 cm off, needs IK) |
| Ayla | — | **Missing Asset**. Needs a dedicated model with silver-white hair, cybernetic arms and machine components |
| Fight AI champion | Feng Mao | Integrated, Packaged Tested |
| Melee, ranged and heavy enemies | Lane minions (Dawn) | Integrated, Packaged Tested (Horde) |
| Magic caster | SKM_Mage | Existing real mesh |
| Quarry Behemoth | Rampage | Claws fixed, Packaged Tested. Phase II, death and results still need a full-encounter pass |

## Remaining asset requirements

- **Ayla:** a dedicated character model. No installed pack resembles her.
- **Orion:** IK hand goals in the Steel retargeter. The card's gravity lance has no asset.
- **Kairos:** the card's chrono blade has no asset.
- **Lane minions:** hand-pose offsets (35–50 cm). These are cosmetic; their own weapons are shown.
- **Minions:** no Anim Blueprints or montages ship with them. If enemies ever move off the shared combat frame, those must be built.
- **Training dummy:** no dedicated actor exists. Add one only if a mode needs it; it's the one place the mannequin may appear.

### Polish pass, Priority 1: hand IK (2026-10-31)

**Tool:** `Tools/GOTA_AddHandIK.py` patches a body's IK rig and retargeter in place. Rerun it after `GOTA_BuildParagonRigs.py`, which rebuilds rigs from scratch.

**What it adds:**
1. A two-bone limb solver per arm (upperarm → hand) with a hand goal on the arm chain.
2. The retargeter's **Retarget IK Goals** op, placed before *Run IK Rig*, with arms at BlendToSource 1 (translation only). The goal sits on the gameplay hand; only the elbow bend adapts, so shoulders and silhouette are preserved.
3. For scaled bodies, a **Scale Source** op (factor 1/BodyScale). The body is a child of the gameplay mesh scaled by BodyScale, so without it the goals land BodyScale short.

| Body | Before | After (idle + combat samples) |
|---|---|---|
| Orion / Steel (0.75, `GOTA_BODY_SCALE=0.75`) | 21–40 cm | **0.0 cm** (9 samples) |
| Lane Melee minion | 40–41 cm | **0.0 cm** |
| Lane Ranged minion | 35–42 cm | **0.0 cm**; the gun is now shouldered |
| Lane Super minion (heavy brute) | ~50 cm | **0.0 cm** |
| Feng Mao (Fight AI champion) | 17–20 cm | **0–7 cm** |
| Echo, the other gods | — | Not changed |

Collision, capsules and damage are unchanged; this is purely visual.

### Polish pass, Priority 2: Quarry Behemoth full encounter (2026-10-31, editor)

| Check | Result |
|---|---|
| Natural fight (`boss_natural.txt`, challenger bot, boss AI, no forced attacks) | Intro, then all four attacks land: Claw Combo 6 windows, Charge, Earthshaker, Crushing Grasp. The bot took the boss to 680/900 and died; the match completed (boss wins). **0 boss attacks after the match ended.** |
| Victory path (`boss_victory.txt`, bot healed 40/s; boss untouched) | **Phase II at 48%** (threshold 50%, triggered on the hit that crossed it). Phase II used Earthshaker chains (5 casts, 13 hits) and Claw Combo. "Boss DEFEATED" then the death animation (`Death_A`) and the BossDefeated cue, round won, match complete (player wins) |
| Claw arc (`boss_claw_arc.txt`, player moved after the swing commits) | In front 180 cm: hit. Front-diagonal (facing 0.82): hit. **Behind, both sides, out of reach 340 cm, rear-diagonal: miss.** |
| **Bug found and fixed** | On the backswing his hand passes 87 cm behind him, and the 120 cm hand sphere struck targets at his back (facing −0.52). The sphere now only counts in his front half. After the fix: **0 of 55** claw hits on targets behind him |
| Avoidance by movement (`boss_avoid.txt`, `gota.Boss.PlaceOnAttack` moves the player once the attack has really started) | Earthshaker: leaving the 450 cm radius → miss; inside → hit. Crushing Grasp: beside/behind → miss (facing −0.30); in front → hit. Behemoth Charge: sidestep → "charge ended (missed)"; on the line → hit. Claw: behind or out of reach → miss |
| Dodge roll (i-frames) | Not verified. The test bot's dodge mode reads Blueprint montages and the boss plays raw sequences, and a scripted `DodgeInDirection` with no input barely moves the player. Boss damage goes through the player's own damage system, so its i-frames apply by design |
| Rewards | None implemented in the project |

### Polish pass, Priority 3: a complete Fight AI match per god (2026-10-31, editor)

- **Script:** `Tools/Tests/god_match_full.txt`. Titan's Quarry, best of 3, against the Feng Mao champion, with close-ups and a body probe about every 3 s.
- **Hand gap:** visible hand to gameplay hand.

| God | Body | Rounds (player:AI) | Match | Body re-applied | Hand gap median / max | Notes |
|---|---|---|---|---|---|---|
| Drakon | Greystone | 2:1 | complete | 4× | 11.3 / 23.5 cm | Sword and shield visible on the body |
| Nyx | Countess | 2:1 | complete | 4× | 8.7 / 9.8 cm | Knocked out in a round, respawned |
| Varoth | Sevarog @ 0.68 | 2:1 | complete | 4× | 7.8 / 15.2 cm | Was 50–85 cm before the scale fix |
| Zephyr | Kwang | 2:1 | complete | 4× | 7.9 / 12.8 cm | |
| Lysara | Aurora | 2:1 | complete | 4× | 8.4 / 10.9 cm | |
| Kairos | Gideon | 2:1 | complete | 4× | 2.8 / 5.7 cm | |
| Orion | Steel @ 0.75 + hand IK | 2:1 | complete | 4× | **0.0 / 4.8 cm** | Was 21–40 cm |

- **Every run:** mesh and materials render, locomotion and attacks animate, damage lands both ways (both fighters' health drops each round), deaths end rounds, respawns re-dress the body, and the match completes.
- **Status:** all gods except the UPA Warden stay **In Development**: no abilities yet for Zephyr, Lysara, Kairos and Orion, and a full ability pass is still pending for the rest.

**Open issue: renderer crash on the results screen (editor `-game` only).**
- 5 of the 7 consecutive runs (Drakon, Nyx, Varoth, Kairos, Orion) hit an access violation in `D3D12RHI` during the base-pass parallel draw, 1–4.5 min after the match completed, while idling on the results screen. No game code is on the stack.
- Two targeted repros (best of 1, 5+ min idle on results, with and without the body probe) did **not** crash, and no packaged run has shown it.
- Earlier long runs this session never hit it. Engine symbols aren't installed, so the dumps can't be read further.
- Watching for it; if it recurs, next steps are installing editor debug symbols and trying `-d3d11` or `r.RHICmdBypass=1` to isolate it.

### Polish pass, packaged Win64 (2026-10-31, built from the current source)

| Check | Result |
|---|---|
| Quarry Behemoth, victory path (`boss_victory_pkg.txt`) | Phase II at 48% → "Boss DEFEATED" (death animation and BossDefeated cue) → round won → match complete → **VICTORY results screen** ("Best of 1, final 1–0", Rematch / Return to Main Menu). Earthshaker 6 casts (4 in phase II, 14 hits), Claw Combo and Crushing Grasp in phase II. No crash |
| Quarry Behemoth, natural (bot unhealed) | All four attacks land; the bot dies; the match completes (boss wins) |
| Claw arc (`boss_claw_arc.txt`) | 53 hits, **0 on targets behind him**; misses from behind, the sides and out of reach |

Test-harness note: script `wait`s advance by frame delta capped at 0.1 s, so slow frames stretch them (up to 6× in some runs). Long scripts that need precise timing start their actions immediately.
