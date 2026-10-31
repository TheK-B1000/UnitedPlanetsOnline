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
3. **Hair:** `Echo_Hair` (hair cards on the same skeleton) is an extra mesh that follows Echo's pose (`SetLeaderPoseComponent`).
4. **Her own sword:** Echo's `M_echoSword` material slot has no material. It is assigned the pack's `M_hide`, so only the gameplay sword shows.
5. **Gameplay sword and shield stay visible** (`bHideAttachedWeaponMeshes = false` for the Warden).
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

| Check | Result |
|---|---|
| Echo mesh, skeleton and physics load in 5.8, no load errors | ✅ |
| Echo replaces the mannequin; no duplicate mannequin visible | ✅ (standalone, Titan's Quarry) |
| Gameplay sword and shield visible on Echo, shield follows the forearm, blocks shown | ✅ (screenshots) |
| Full Fight AI best-of-3 to the results screen, with respawn between rounds re-applying Echo | ✅ (the challenger bot lost 0–2 this run) |
| Packaged Win64 | see report |
| PIE / listen-server client | not yet run |
| Magic stance and spell socket alignment | not yet checked |

## Known limitations

- **Ponytail physics:** the hair follows the body pose (leader pose), so the ponytail's own physics AnimBP doesn't run on it. The scarf is Chaos cloth on the body mesh.
- **Animation source:** Echo's own animations aren't used. She plays the existing WarOfGods sword-and-shield set through the retarget.
- **Groom hair:** strand hair and eyebrows weren't migrated; the hair cards are used instead.
