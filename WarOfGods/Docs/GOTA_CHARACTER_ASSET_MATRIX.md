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
