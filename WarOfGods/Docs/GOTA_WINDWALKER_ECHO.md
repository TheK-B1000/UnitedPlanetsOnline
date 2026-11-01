# Windwalker Echo as the default player appearance

Echo replaces the UE mannequin's look on the default prototype player (the UPA Warden hero, `BP_Player`). It is a **visual replacement only**: the pawn, controller, movement, combat, sword, shield, magic, damage, input and multiplayer are unchanged.

## Source and migration

| | |
|---|---|
| Source project | `B:\Unreal\WindwalkerEcho\WindwalkerEcho.uproject`, the Epic sample. UE 5.2 association, 1.6 GB, 332 assets, opened read-only by 5.8. |
| Destination | `B:\Unreal\UnitedPlanetsOnline\WarOfGods`, UE 5.8 |
| Method | `AssetTools.migrate_packages` (Unreal's Migrate) via `Tools/GOTA_MigrateEcho.py`, run inside the source project |
| Migrated | 144 packages, 1.3 GB. Original paths kept: `/Game/EchoContent/Characters/Echo/...` |
| Not migrated | Maps, `BP_Echo` and the game mode, input, LiveLink/ARKit face rigs, environment, and groom strands (would need the HairStrands plugin) |
| Git | `WarOfGods/Content/EchoContent/` is not committed. The remote is public and these are Epic's uncooked source assets. Instead it's reproducible and verified: run `Tools/GOTA_MigrateEcho.py`, then check against the SHA-256 manifest `Tools/ThirdPartyManifests/EchoContent.txt` (144 files) with `python Tools/GOTA_ThirdPartyAssets.py verify`. See `Docs/GOTA_THIRD_PARTY_ASSETS.md` |

The 144 migrated packages break down as:
- 2 skeletal meshes: `Echo` and `Echo_Hair`
- 1 skeleton
- 7 physics assets, including scarf cloth
- 15 materials, 13 material instances, 5 material functions, 3 MPCs, 6 subsurface profiles
- 69 textures and 1 cube map
- Dependencies of the meshes' post-process setup: 2 small AnimBPs, 2 control rigs, 9 pose assets and 9 corrective sequences

**Plugins:** none added. ControlRig and ChaosCloth are already enabled by default.

## How it is displayed

Echo's skeleton has the full UE5 Mannequin bone set (root, pelvis, spine_01–05, neck_01–02, metacarpal fingers, ik_* bones), plus scarf, ponytail and shoulder-pad bones. It is a different skeleton asset, so the project's proven runtime retarget path is used. This is the same path the Paragon god bodies use.

1. **The mannequin stays the gameplay mesh.**
   - `BP_Player` keeps the hidden `SKM_Quinn_Simple` mesh, which still animates and refreshes bones.
   - All montages, notifies, hit traces, the sword and shield sockets, and spell sockets remain on it.
2. **Echo is the visible body.**
   - `UGOTAHeroBodyComponent` attaches `Echo` and drives it with `UGOTARetargetAnimInstance`, a native Retarget Pose From Mesh node.
   - The retargeter is `RTG_GOTA_Manny_Echo`, with IK rig `IK_GOTA_Echo`.
   - The rig has 19 chains: spine, neck, head, clavicles, arms, legs, and all ten finger chains, so the grip on the gameplay sword holds.
   - It was built by `Tools/GOTA_BuildParagonRigs.py` with `GOTA_BODIES=Echo`, using manual chains rather than the crashing auto-generation.
3. **Hair:** `Echo_Hair` (hair cards on the same skeleton) is an extra mesh that copies Echo's pose through `UGOTACopyPoseAnimInstance` (Copy Pose From Mesh). Leader pose isn't used because it skips evaluation; this way the mesh's own Post Process Anim Blueprint `Echo_Hair_AnimBP` runs, which is Epic's rigid-body ponytail on 8 bones (`PA_Echo_Ponytail`). No new simulation was added.
4. **Her own sword:** Echo's `M_echoSword` material slot has no material. It is assigned the pack's `M_hide`, so only the gameplay sword shows.
5. **Gameplay sword and shield stay visible** (`bHideAttachedWeaponMeshes = false` for the Warden).
   - `UGOTAHeroBodyComponent::FollowBodyWithWeapons` moves them from the hidden mannequin onto Echo's matching bone, keeping the mannequin socket's offset.
   - It re-applies the same frame the Blueprint re-attaches them: equip, stance change, respawn.
   - Melee damage is a forward sphere sweep from the character (`SphereTraceDamage`, radius 20, length 200), not a blade trace. Echo's visible hand stays 5–15 cm from the gameplay hand, inside that radius.
   - Spells spawn at the gameplay mesh's `hand_l`, measured 6–8 cm from Echo's visible left hand.
6. **Configuration lives in the hero definition:**
   - The `DA_Hero_UPAWarden` body fields are `BodyMesh`, `BodyRetargeter`, `BodyExtraMeshes`, `BodyHiddenMaterialSlots` and `BodyHiddenMaterial`.
   - The source of truth is `Tools/GOTA_HeroContent.py` → `BODIES["UPAWarden"]`, applied by `Tools/GOTA_BuildHeroBodies.py`.
   - Clearing `BodyMesh` restores the mannequin.
7. **Where it applies:**
   - `AArenaGameMode::SetPlayerDefaults` (Duel and Horde): the accepted HeroId, or the default hero when there's no selection.
   - `AGOTAStoryGameMode::SetPlayerDefaults` (Story): the default hero.
   - Selected gods (Drakon, Nyx, Varoth) keep their own bodies. Enemies, including Rampage, are untouched.
   - HeroId replicates on the body component, so remote clients build the same body. No extra pawn and no new replicated identity.

## Verification

Run on 2026-10-31. Test scripts are in `Tools/Tests`: `echo_magic_equipment.txt`, `echo_mp_host.txt`/`echo_mp_client.txt`, `echo_hair.txt` and `god_regress.txt`. The commands `gota.Test.Body` and `gota.Test.WatchSpawns` log hand gaps, weapon parents and spell spawn points.

| Check | Result |
|---|---|
| Echo loads in 5.8, no missing dependencies | ✅ editor, PIE, standalone and packaged |
| Packaged Win64 (`B:\Unreal\GOTA_Builds\Pass2_Paragon`) | ✅ builds from current source. Full Fight AI match to the results screen, with Echo re-applied every round |
| Packaged materials | ✅ after fixing the Costume3 textures (see below). Before, the pants, leather and buckle were pink in cooked builds only |
| Sword and shield on Echo's hands: idle, attack, block, stance change (Unarmed→Magic→Melee), respawn | ✅ every check shows both on the body mesh and visible, with no duplicates |
| Sword hits | ✅ standalone vs frozen AI (220 → 200) and listen-server PvP (100 → 80) |
| Magic | ✅ `BP_Projectile_DarkEnergy` spawns 6–8 cm from the visible left hand and hits (260 → 240 → 220; PvP 80 → 60) |
| PIE (Titan's Quarry, ArenaGameMode) | ✅ Echo applied, gameplay mesh hidden, enemies untouched |
| Listen server + client | ✅ both machines show Echo and her equipment on both players. The client sees the host's spell from the hand. Damage, death, respawn and match end all work |
| Drakon / Nyx / Varoth / Rampage | ✅ unchanged: own bodies, weapons stay on their gameplay mesh, full matches. Rampage charge and slam land in the package |
| Ponytail physics | ✅ rigid-body ponytail active (tip moves 10–30 cm in head space during combat), packaged and standalone |

### Fixes made during validation

- **Echo Costume3 textures:** these six were the only non-virtual textures in the set, but the cloth materials sample them as virtual.
  - The editor tolerates the mismatch; the cook rendered them pink.
  - `Tools/GOTA_FixThirdPartyBlueprints.py` marks them virtual, and the manifest was re-recorded.
- **Duel slots:** `ADuelGameMode::AssignDuelSlots` re-issued slot 0 to a joining client, so both humans spawned on the same start, fighter 1 didn't exist, and every PvP round ended instantly. Taken slots are now skipped.
- **PvP melee:** every `BP_Player` is `TeamNumber` 0, and melee skips team-mates, so swords never hurt in human-vs-human duels. Duel PvP now sets each player's team to their duel slot. Fight AI, Horde and Story are unchanged.

## Known limitations

- **Ponytail physics:** active (see above). The scarf is Chaos cloth on the body mesh.
- **Rampage claws** still miss (a boss reach issue, unrelated to Echo).
- **Varoth's hands** sit about 50 cm from the gameplay hands (the known hunched-pose issue, on the roster pass list).
- **Animation source:** Echo's own animations aren't used. She plays the existing WarOfGods sword-and-shield set through the retarget.
- **Groom hair:** strand hair and eyebrows weren't migrated; the hair cards are used instead.
