# Ayla: playable (checkpoint 2)

Ayla is a playable, selectable GOTA fighter. Her own Meshy body and animation library sit on top of the shared combat frame. Her four signature abilities belong to checkpoint 3 and are not started: she uses the shared kit only.

## How she is built

GOTA heroes share one gameplay pawn, `BP_Player`. It owns movement, melee, block, parry, dodge and perfect dodge, Divine Counter, health and damage, death and respawn, and replication. Each hero wears a cosmetic body over that pawn. The pawn's own mesh, the mannequin, is hidden but keeps animating, so every montage, notify, hit window and socket runs unchanged. Ayla follows the same pattern, so none of those systems is duplicated or replaced.

| Piece | Asset | Notes |
|---|---|---|
| Hero definition | `/Game/Heroes/Data/DA_Hero_Ayla` (existing) | Now `PLAYABLE`, `BP_Player` pawn, body `SK_Ayla`, retargeter, clip set. Identity, card text and portrait unchanged |
| Body | `/Game/Characters/Ayla/Mesh/SK_Ayla` | Her own 28-bone skeleton, natural height (about 161 cm); not scaled, bind pose untouched |
| Retargeter | `/Game/Characters/Ayla/Rigs/IK_GOTA_Ayla`, `RTG_GOTA_Manny_Ayla` | Same method as the Paragon bodies (`Tools/Ayla/GOTA_BuildAylaRig.py`); the fallback for anything without her own clip |
| Her clips | `/Game/Characters/Ayla/Data/DA_AylaBodyAnimSet` | New data asset type `UGOTAHeroBodyAnimSet` (`Tools/Ayla/GOTA_BuildAylaAnimSet.py`) |
| In-place copies | `/Game/Characters/Ayla/Animations/InPlace/` | Six derived clips; the imported originals are unchanged |

**How her own clips play.** Hero bodies use a native anim graph: the pose is retargeted from the hidden mannequin, with a slot on top for divine abilities. A new layer sits between them and is active only for a body with a clip set:

- **Locomotion.** Her own Idle 9, Walking Woman and Run 02 blend by the capsule's real ground speed. Walking reaches its authored pace at 167 cm/s, and Run 02 at `BP_Player`'s top speed of 500 cm/s.
- **Jumping.** The airborne part of Regular Jump shows while the capsule is falling.
- **Death.** On defeat her Dead clip plays once and holds its last frame.
- **Combat.** Each mapped combat montage on the hidden frame shows her own clip at the same progress. Timing keys pin her strikes to the frame's hit notifies. Block and hit reactions affect only the spine, arms and head, so her legs keep moving with the capsule.
- **Anything else.** Any montage without a clip shows the retargeted frame, so no action is ever unanimated.

Every other hero has no clip set, and its body is unchanged.

| Combat frame montage | Ayla's clip | Notes |
|---|---|---|
| `Montage_SwordCombo` (player combo) | `Triple_Combo_Attack_InPlace` | The frame's hits at 0.70, 1.29 and 2.58 s are pinned to her strikes at 0.77, 1.53 and 2.23 s |
| `Montage_Shield_Block` | `Block5` | Upper body |
| `Montage_Shield_Block_React`, `..._NoRootMotion` | `Sword_Parry` | Blocked hit, parry |
| `Montage_Sword_Hit` | `Hit_Reaction_InPlace` | Upper body |
| `Montage_Hadouken` (magic stance) | `mage_soell_cast_3` | |
| `Montage_Sword_Slash` (AI frame) | `Double_Combo_Attack_InPlace` | When an AI plays Ayla |
| `Montage_Sword_SpinningAttack` (AI) | `Axe_Spin_Attack` | |
| `Montage_Melee_GroundSmash` (AI) | `Grip_and_Throw_Down` | |
| `Montage_Stagger` (AI) | `Hit_Reaction_InPlace` | |
| `Montage_Sword_Block_React` (AI) | `Sword_Parry` | |
| Sheathe, unsheathe, AI jump attack, dodge | Retargeted frame | No matching clip. Jump Attack was not in the Meshy export |

The dodge is not a montage. It is the Blueprint's own movement and invulnerability, so her body shows her own locomotion while the capsule dodges. Its protection is the shared frame's, unchanged.

## Root motion

Ayla's skeleton has no root bone, so travel baked into a clip moves her pelvis and would carry her body away from the capsule the server moves and hits. `Tools/Ayla/GOTA_AylaClipAudit.py` measured every clip's pelvis travel. `Tools/Ayla/GOTA_AylaInPlace.py` made in-place copies of the ones that travel. Each copy removes only the slow horizontal travel (a 1 s moving average) and keeps her sway, height and every rotation.

| Clip | Largest pelvis distance from bind: original → in-place |
|---|---|
| Triple Combo Attack | 151 cm → 16 cm |
| Double Combo Attack | 116 cm (starts 1.1 m to the side) → 9 cm |
| Hit Reaction | 93 cm → 25 cm (the quick flinch is kept) |
| Quick Step and Spin Dodge | 169 cm → 37 cm (still ends turned 46°, so it is not used) |
| Basic Jump | 119 cm → 16 cm |
| Regular Jump | Rise above standing height removed (the capsule carries her up), crouch kept |

The locomotion, block, parry, spell and spin clips were already in place. Dead keeps its natural 60 cm fall. A defeated fighter is no longer hit, and the drift check counts only living fighters.

Nothing else was changed to keep her on the capsule. There is no blanket invulnerability, no change to hit timing, and the capsule does not follow the animation.

**Measured** with `gota.Test.BodyDrift`: the largest horizontal distance between her visible pelvis and her capsule while she is alive. Single player, combo, block, hits and spell: Ayla 23 cm (mean 6). For comparison, the existing retargeted bodies measured Nyx 28 cm and UPA Warden 24 cm. Listen server with a remote Ayla, on the client: 8 cm.

## Weapon

Her card's weapon is **Dual Cybernetic Gauntlets**, and her model already has armoured cybernetic forearms and hands. She fights with them: no weapon was added or invented. The shared frame's prototype sword and shield stay hidden on her, as on every hero, and its hit traces are unchanged.

Her Meshy clips named for an axe or sword (Axe Spin Attack, Sword Parry) read as gauntlet strikes and guards, because nothing is in her hands. Her hands are single bones with no fingers, so grips cannot be posed. That is not needed for gauntlets.

## Hero selection

She is selected through the existing frontend: the menu's selection, then a request the server validates. Ayla is `PLAYABLE` because she was verified through a full match. Every other roster hero stays `IN_DEVELOPMENT`.

Before promotion, the menu sent `Hero=Ayla` and the server rejected her ("In development ... cannot be selected") and substituted UPA Warden. After promotion, with no developer flag, the server accepted her and her body and clips appeared.

## Verification

Regression cases in `Tools/GOTA_AbilityRegression.py`, all in the editor. The packaged results are recorded in a following docs commit, against the committed code.

| Case | What it checks | Result |
|---|---|---|
| Ayla combat (`ayla_combat.txt`) | Combo, quiet guard, guard under attack, blocked hit, hit, spell; her own clip for each; drift under 45 cm | Pass (quiet machine) |
| Ayla boss (`ayla_boss.txt`) | Boss dodge, hit and death clip, drift | Pass |
| Ayla perfect dodge (`power_dodge.txt`) | Shared perfect dodge and Divine Counter with her body | Pass after the boss stall fix (`12c4358`); it had failed only when a stalled frame dropped the boss's attack |
| Ayla whole match | A full Duel to a winner, her block and death clips | Pass |
| AylaMP (listen server, remote Ayla) | Body and clips on the client, drift 8 cm | Pass |
| AylaHorde (host plus Ayla clients) | Remote Ayla dies and comes back at the next wave | Pass, after its setup fix (host kept alive until the clients join) |
| CombatVisAyla (UPA Warden host, Ayla client) | Each sees the other's and its own attacks, guards and hits | Pass on the clean machine. Inconclusive earlier with a starved client |
| Menu selection (`ayla_menu_select.txt` from the frontend, no developer flag; a scripted run, not a suite case) | Server accepts `Hero=Ayla` and applies her body | Pass |

The clean-machine full suite (`GOTA_MULTIPLAYER_COMBAT_ANIMATION.md`) included every Ayla case above: none failed.

**Her guard check cannot hide a wrong clip.** Under load, an AI hit landing in the same frame as her block replaced the block montage with the block reaction, and the guard check was briefly loosened to accept either clip. It is now strict. The script first raises a quiet guard with nothing incoming, so there is no race. The check then requires `Montage_Shield_Block -> own clip AS_Ayla_Block5 (upper body)`. It fails on any line mapping the block to another Ayla clip, or the block reaction to anything but `Sword_Parry`.

## Known limits

- **Remote combat animation.** Found during this checkpoint: clients saw no player's combat montages, for every hero. Fixed separately in `3c1ec50` (`GOTA_MULTIPLAYER_COMBAT_ANIMATION.md`). Ayla's own clips now show on every client.
- **Her visible hands are 70 to 110 cm from the hidden frame's hands during her own clips.** Nothing is held, so nothing depends on it. A weapon-holding hero would need her retargeted pose instead.
- **Untested combat clips.** Grip and Throw Down, Axe Spin Attack and Double Combo are mapped for an AI-controlled Ayla, with proportional timing (no hit pinning yet).
- **Dodge.** Her body shows locomotion during the dodge, not a dodge clip (Stand Dodge is unused). The frame's dodge is movement, not a montage.
- **Jump Attack** is missing from the Meshy export.
- **No human has played her.** Feel, readability of her strikes and animation quality need a human eye.
