# Ayla: Meshy import (checkpoint 1)

Ayla's Meshy model ("Ironfeather Valkyrie", exported 2026-11-24) imported into `/Game/Characters/Ayla`, with its own skeleton and all 22 exported animations playing in Unreal 5.8. Nothing outside that folder was changed. She is not yet playable, and her abilities are not started.

The existing Ayla assets are untouched: `/Game/Heroes/Ayla/MetaHuman/MH_Ayla`, `DA_Hero_Ayla` and her portrait textures.

## Source files

The three Desktop downloads are read only. Working copies are in `B:\Unreal\GOTA_SourceAssets\Ayla\Meshy_20261124` (outside the repository), with one folder per download.

| Download | FBX | What it holds |
|---|---|---|
| `..._All_animation.zip` | `..._All_animation.fbx` (57 MB) | Ayla's skinned mesh, 28-bone skeleton and 20 animation takes |
| | `..._character_meshyrig.fbx` (51 MB) | Unanimated Meshy rig with different bone names (`Hips`, `Spine01`, ...), 24 fps. Not used |
| `..._without_skin_Backflip.zip` | `..._Backflip.fbx` | Backflip only, skeleton without mesh |
| `..._without_skin_Mirror_Viewing.zip` | `..._Mirror_Viewing.fbx` | Mirror Viewing only, skeleton without mesh |

All three zips also contain the same four 4096x4096 PNG maps (base colour, metallic, normal, roughness) and the same `meshyrig` FBX, byte for byte (SHA-256 identical).

**Mesh.** 91,866 faces (82,579 quads; 181,515 triangles), 94,220 vertices, one material slot (`BPY_PBRSource`), 23 weighted bone groups. 161 cm tall in Blender. Unreal's bounds report 170 cm, because skeletal-mesh bounds are padded.

**Skeleton.** 28 bones with UE4-mannequin-style names: `pelvis` (root), `spine_01-03`, `neck_01`, `head`, `head_end`, `headfront`, `clavicle/upperarm/lowerarm/hand_l/r`, `LeftHand_End`, `RightHand_End`, `thigh/calf/foot/ball_l/r`, `LeftToe_end`, `RightToe_end`. There is no `root` bone, no fingers, and no twist or IK bones. The Backflip and Mirror Viewing skeletons match it bone for bone and in order.

## Animation audit

All animations are 30 fps, on `SKEL_Ayla`.

| Requested | Imported as `AS_Ayla_...` | Length | Notes |
|---|---|---|---|
| Axe Spin Attack | `Axe_Spin_Attack` | 2.50 s | |
| Block5 | `Block5` | 2.50 s | |
| Dead | `Dead` | 2.97 s | |
| Double Combo Attack | `Double_Combo_Attack` | 2.83 s | Starts with the pelvis 113 cm to the side: root motion is baked into the take |
| Grip and Throw Down | `Grip_and_Throw_Down` | 4.67 s | |
| Hit Reaction | `Hit_Reaction` | 1.63 s | |
| Idle 9 | `Idle_9` | 2.00 s | |
| **Jump Attack** | **missing** | | Not in any of the three exports |
| Magic Spell Cast 3 | `mage_soell_cast_3` | 3.33 s | Meshy's own spelling, kept |
| Motivational Cheer | `Motivational_Cheer` | 9.00 s | |
| Quick Step / Spin Dodge | `Quick_Step_and_Spin_Dodge` | 1.17 s | |
| Regular Jump | `Regular_Jump` | 1.87 s | |
| Run 2 | `Run_02` | 0.70 s | |
| Stand Dodge | `Stand_Dodge` | 1.40 s | |
| Sword Parry | `Sword_Parry` | 1.83 s | |
| Triple Combo Attack | `Triple_Combo_Attack` | 4.33 s | |
| Walking Woman | `Walking_Woman` | 0.97 s | |
| Backflip (separate export) | `Backflip` | 2.13 s | |
| Mirror Viewing (separate export) | `Mirror_Viewing` | 7.33 s | |
| Not requested | `Basic_Jump` | 5.87 s | In the combined export |
| Not requested | `mage_soell_cast_4` | 2.20 s | In the combined export |
| Meshy's automatic set | `Running`, `Walking` | 0.63 s, 1.03 s | In the combined export |

The combined file holds exactly 20 takes, Meshy's limit. 16 of the 17 requested clips are there; Jump Attack is not, while `Basic_Jump` and a second spell cast are. Nothing was invented or substituted. The combined file did not need splitting: Unreal's FBX importer made one Animation Sequence per take.

## The scale fix (Blender)

Imported as delivered, every animation shrank Ayla to 1/100 size: the bind pose showed correctly, but each animated copy was invisible in the test level. The cause:

- Meshy's FBX files are authored in metres.
- Unreal drops the Blender `Armature` node and folds its x100 unit scale into the `pelvis` bind pose, with the child bones left in metres.
- The animation keys carry no such scale, so playing any take set the pelvis scale back to 1.
- The two skeleton-only exports are also stored under a x100, rotated armature, so they were 100x smaller again. Unreal's unit-conversion option did not change any of this.

`Tools/Ayla/normalize_fbx.py` (headless Blender 5.1.1) re-exports each file:

1. Bakes the armature to identity.
2. Converts to centimetres: it scales the armature, mesh and location keys by the same factor and sets the scene unit to 1 cm.
3. Writes FBX with no node scaling and no leaf bones.

It does not retarget, rename, clean up or resample anything. After it, the main file's bind pose matches the original exactly (pelvis at 90.78 cm). All three files share that bind pose. Every take's pelvis sits 72 to 94 cm up with scale 1.

Normalized copies: `B:\Unreal\GOTA_SourceAssets\Ayla\Normalized\` (`Ayla_Character.fbx`, `Ayla_Backflip.fbx`, `Ayla_Mirror_Viewing.fbx`).

## Unreal assets (`/Game/Characters/Ayla`)

| Folder | Assets |
|---|---|
| `Mesh` | `SK_Ayla` (34 MB), `SKEL_Ayla` (her own), `PHYS_Ayla` (generated by the importer) |
| `Materials` | `M_Ayla`: the four maps wired as base colour, metallic (R), roughness (R) and normal; marked for skeletal meshes |
| `Textures` | `T_Ayla_BaseColor` (sRGB), `T_Ayla_Normal` (normal map), `T_Ayla_Metallic` and `T_Ayla_Roughness` (masks, linear), all 4096x4096 |
| `Animations` | the 22 `AS_Ayla_*` sequences above |
| `Test` | `L_Ayla_AnimTest`: a bind-pose copy plus one copy per animation in a grid, with a floor, sun and sky light |

Total: 31 files, 87.9 MB, all under 100 MB each. The repository has no Git LFS rules, so this follows the existing practice of committing Unreal content directly.

**Skeleton choice.** Ayla has her own skeleton. The 28-bone Meshy skeleton matches no skeleton in this project: the heroes use Paragon skeletons, and the only mannequin is UE4's, which has a `root` bone, fingers and twist bones. Her bone names follow the UE4 mannequin, so an IK Retargeter can map them later if shared animations are wanted.

## Verification (editor, `-game`, test level)

| Check | Result |
|---|---|
| Mesh displays, correct orientation | Yes. Like Unreal's mannequins, the mesh faces +Y, so a character Blueprint will need the usual -90° mesh yaw |
| Materials and textures | Yes, after the material was marked for skeletal-mesh use. Without that flag the game drew the grey default material |
| Idle 9, Walking Woman, Run 02, a combat clip (Triple Combo, Axe Spin) | Play |
| All 22 sequences play | Yes. A difference image of two grid frames 0.5 s apart lights up all 22 animated copies, and only the bind pose stays still (`Docs/AssetRefs/Ayla/ayla_import_motion_diff.png`) |
| Deformation | No tearing or collapse seen at the shoulders, hips, skirt or hair in the close-ups (`ayla_import_bind_pose.png`, `ayla_import_front_row_playing.png`). Checked from screenshots only, not frame by frame |
| Scale | Pelvis about 90 cm, height 161 cm (Meshy's target was about 170 cm) |
| Saved and reloaded | The test ran in a fresh `-game` process that loaded every asset from disk |

**Not checked:**

- The physics asset's bodies. The importer generated it, but it was not inspected or simulated.
- Whether the normal map's green channel convention (OpenGL or DirectX) is right. Lighting looked plausible.
- Every frame of every clip, and close-ups of combat poses.

## Known issues for checkpoint 2

- **No weapon.** Axe Spin Attack, Sword Parry and the combos are animated as if holding a weapon, but the mesh has none.
- **No root bone.** `pelvis` is the root. Unreal's root motion needs a `root` bone, so `Double_Combo_Attack`'s 113 cm sideways offset (and any travel in other clips) is pelvis motion. Decide on in-place or root motion before wiring movement.
- **Jump Attack is missing** and needs a separate Meshy export.
- **Hands are single bones.** There are no fingers, so grips cannot be posed.

## Reproduce

```
blender -b --factory-startup --python WarOfGods/Tools/Ayla/normalize_fbx.py -- <meshy fbx> <out fbx> mesh|anim [take name]
UnrealEditor-Cmd WarOfGods.uproject -run=pythonscript -script=WarOfGods/Tools/GOTA_ImportAyla.py -unattended -nosplash -nullrhi
UnrealEditor-Cmd WarOfGods.uproject -run=pythonscript -script=WarOfGods/Tools/GOTA_AylaAnimTestLevel.py -unattended -nosplash -nullrhi
UnrealEditor WarOfGods.uproject /Game/Characters/Ayla/Test/L_Ayla_AnimTest?game=/Script/Engine.GameModeBase -game "-ExecCmds=gota.Test.Run Tools/Tests/ayla_anim_shot.txt"
```

The import refuses to run if `/Game/Characters/Ayla` already has assets. `GOTA_AYLA_REPORT_ONLY=1` re-reads an existing import into `Saved/GOTA_ImportAyla.json`.
