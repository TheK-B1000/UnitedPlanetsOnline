# Paragon prototype bodies for the gods

**Status:** Stage 1 (bodies). Drakon, Nyx and Varoth wear Paragon bodies over the existing combat frame. All three remain **In Development**: they can't be selected in the menu, and they can only be spawned for testing.

Paragon meshes are **prototype bodies, not final god designs**.
- Epic's Paragon assets are free to use in Unreal projects, including commercially.
- The Paragon name must not be used to market the game.
- Don't redistribute the original packs.
- Check each Fab listing's terms before shipping.

## Architecture

```
UGOTAHeroDefinition (DA_Hero_<Id>)      identity, lore, card stats, portrait, availability
  ├─ GameplayPawnClass = BP_Player      the combat frame: Mannequin skeleton, montages, hit windows, notifies
  └─ BodyMesh + BodyRetargeter          cosmetic prototype body (Paragon)
          │
ADuelGameMode::SetPlayerDefaults ──► UGOTAHeroBodyComponent::ApplyToPawn   (server)
          │  HeroId replicates
          ▼
UGOTAHeroBodyComponent::ApplyBody   (every machine)
  - hides the pawn mesh, which keeps ticking and refreshing bones (AlwaysTickPoseAndRefreshBones)
  - attaches a USkeletalMeshComponent with the Paragon mesh to the pawn mesh
  - UGOTARetargetAnimInstance: a native proxy whose only node is FAnimNode_RetargetPoseFromMesh
    (source = parent mesh), using IK retargeter RTG_GOTA_Manny_<Body>
  - render-hides attached weapon meshes (the shared prototype sword); collision is untouched
```

**Why this design:** every combat behaviour still runs on the original skeleton. That includes attacks, parry, dodge, the Divine Counter, momentum, director reads, damage, death and round flow. So nothing in combat, server validation or the `HeroId` registry changed. The body is purely visual.

## Assets

| God | Body | Mesh | Retargeter | Notes |
|---|---|---|---|---|
| Drakon (Warfall Titan) | Greystone | `/Game/ParagonGreystone/.../Meshes/Greystone` | `/Game/Heroes/Paragon/Rigs/RTG_GOTA_Manny_Greystone` | Card weapon is a two-handed axe; Greystone carries sword and shield |
| Nyx (Shadow Weaver) | Countess | `/Game/ParagonCountess/.../Meshes/SM_Countess` | `RTG_GOTA_Manny_Countess` | |
| Varoth (Inferno Colossus) | Sevarog | `/Game/ParagonSevarog/.../Meshes/Sevarog` | `RTG_GOTA_Manny_Sevarog` | Sevarog has no toe bones; his leg chains end at the foot |

IK rigs: `/Game/Heroes/Paragon/Rigs/IK_GOTA_<Body>`.
- Each rig has 9 FK chains: Spine, Neck, Head, both clavicles, both arms, both legs.
- The source rig is the project's `IK_Mannequin`.

## Tools

- `Tools/GOTA_BuildParagonRigs.py` builds the IK rigs and retargeters. Use `GOTA_BODIES=Greystone,...` to build only some.
  - Engine bug, 5.8: `apply_auto_generated_retarget_definition` / `apply_auto_fbik` and `snap_bone_to_ground` with a missing bone crash the editor on Paragon skeletons.
  - The tool uses manual chains and checks that bones exist before using them.
- `Tools/GOTA_HeroContent.py` → `BODIES` holds the mapping and status, which is the single source of truth.
- `Tools/GOTA_BuildHeroBodies.py` applies `BODIES` to the hero definitions. `GOTA_BuildMenuUI.py` applies it too when it rebuilds them.
- `Tools/Tests/god_body_check.txt` is the test script. It takes close-ups, runs the Fight AI bot, and plays a full best-of-3. Run it with `?Hero=<Id>?DevHeroes=1`.
- `Tools/GOTA_PIESmoke.py` runs a play-in-editor smoke test.

## Testing switch

`?DevHeroes=1` on the duel URL, or `gota.Heroes.AllowInDevelopment 1`, lets **In Development** heroes that have a gameplay pawn spawn.
- Not available in shipping builds.
- `GetDefaultHero` still picks only Playable heroes, and the menu still blocks In Development heroes.

## Verification (2026-10-31)

**Test setup.**
- Standalone `-game` at 1600×900, Titan's Quarry, Fight AI on Veteran (the default).
- URL: `?Hero=<Id>?DevHeroes=1`.
- Script: `Tools/Tests/god_body_check.txt`, with the challenger bot driving the player.

| Check | Drakon / Greystone | Nyx / Countess | Varoth / Sevarog |
|---|---|---|---|
| Server accepts the HeroId (dev switch on) | ✅ | ✅ | ✅ |
| Body applied on spawn and on the round-2 respawn | ✅ | ✅ (cloth simulates) | ✅ |
| Locomotion and attacks follow the combat frame | ✅ | ✅ | ✅, but pose quality is poor (see below) |
| Damage dealt and taken | ✅ | ✅ | ✅ |
| Full best-of-3 to the results screen | ✅ 2–0 win | ✅ 2–0 win | ✅ 2–0 win, VICTORY screen shows "VAROTH · INFERNO COLOSSUS" |
| Menu selection | blocked (In Development), as intended | blocked | blocked |
| Listen-server / client replication | not yet tested | — | — |
| PIE | PIE runs on the authored arena, but the map's default mode is not Duel. The gods were validated in standalone runs. | — | — |
| Packaged Win64 Development build | ✅ full match 2–0 on Shattered Dominion; materials and body render | not run | not run |

**Known issues.** These are the reasons none of the three is marked Playable.
- **Sevarog** retargets with a hunched spine and awkward arm poses. His proportions, height and reference pose are far from the Mannequin's. He needs retarget-pose tuning, or his own montages (stage 2).
- **Shared moveset:** all three use the shared Warden moveset and sword timings, so each god's identity comes only from the body.
- **Weapons:** each body's own weapon is part of its Paragon mesh. Greystone has a sword and shield; Countess has her blades. The shared prototype sword is hidden.
- **Menu availability:** the gods can't be picked from the menu until they're verified as Playable, by design.

## Next stages (not started)

1. **Per-god moveset:** replace the shared Mannequin attack montages with retargeted Paragon montages for each god, via `IKRetargetBatchOperation` and the existing notify and hit-window conventions.
2. **Divine abilities:** each god's card abilities as server-authoritative abilities, with their own VFX and cooldowns.
3. **Weapon match:** Drakon's card calls for an axe, so either swap the weapon or accept Greystone's sword as the prototype.
4. **Proportions:** tune `BodyScale` and capsule size for large bodies such as Sevarog.
5. **Remaining gods:**
   - Lysara → Aurora
   - Kairos → Wraith
   - Orion → Steel
   - Zephyr → Kwang (pack now present)
   - Ayla → Serath, as a temporary donor only. Ayla's identity (silver hair, cybernetic arms, metallokinesis) needs her own model.
