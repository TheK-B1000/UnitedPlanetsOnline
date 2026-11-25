"""Builds /Game/Characters/Ayla/Data/DA_AylaBodyAnimSet: which of Ayla's own clips her body shows (Docs/GOTA_AYLA_PLAYABLE.md).

Gameplay stays on the shared combat frame (BP_Player); this set only chooses what her visible body draws. Travelling
clips use their in-place copies (GOTA_AylaInPlace.py) so the body never leaves the capsule. A combat montage that is not
listed here shows the retargeted combat frame.

Run: UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this> -unattended -nosplash -nullrhi
"""
import unreal

PATH = "/Game/Characters/Ayla/Data/DA_AylaBodyAnimSet"
A = "/Game/Characters/Ayla/Animations/AS_Ayla_"
IP = "/Game/Characters/Ayla/Animations/InPlace/AS_Ayla_"
EAL = unreal.EditorAssetLibrary

# Combat frame montage -> Ayla clip, upper body only?, timing keys [(montage s, clip s), ...] (empty = proportional).
# Montage_SwordCombo's hit notifies fire at 0.70, 1.29 and 2.58 s; Triple Combo's strikes peak at 0.77, 1.53 and
# 2.23 s (hand speed), so each strike is pinned to its hit.
COMBO_KEYS = [(0.0, 0.0), (0.70, 0.77), (1.29, 1.53), (2.58, 2.23), (3.53, 4.33)]
ACTIONS = [
    # The player's combat frame (BP_Player)
    ("/Game/Animations/Melee/Montage_SwordCombo", IP + "Triple_Combo_Attack_InPlace", False, COMBO_KEYS),
    ("/Game/Animations/Melee/Montage_Shield_Block", A + "Block5", True, []),
    ("/Game/Animations/Melee/Montage_Shield_Block_React", A + "Sword_Parry", False, []),
    ("/Game/Animations/Melee/Montage_Shield_Block_React_NoRootMotion", A + "Sword_Parry", False, []),
    ("/Game/Animations/Melee/Montage_Sword_Hit", IP + "Hit_Reaction_InPlace", True, []),
    ("/Game/Animations/Magic/Montage_Hadouken", A + "mage_soell_cast_3", False, []),
    # The AI fighter's frame (BP_Enemy_Melee), when an AI plays Ayla. Unsheath, sheath and the jump attack have no
    # matching clip and show the retargeted frame.
    ("/Game/Animations/Melee/Montage_Sword_Slash", IP + "Double_Combo_Attack_InPlace", False, []),
    ("/Game/Animations/Melee/Montage_Sword_SpinningAttack", A + "Axe_Spin_Attack", False, []),
    ("/Game/Animations/Melee/Montage_Melee_GroundSmash", A + "Grip_and_Throw_Down", False, []),
    ("/Game/Animations/Melee/Montage_Stagger", IP + "Hit_Reaction_InPlace", False, []),
    ("/Game/Animations/Melee/Montage_Sword_Block_React", A + "Sword_Parry", False, []),
]


def load(path):
    a = unreal.load_asset(path)
    if not a:
        raise RuntimeError("missing " + path)
    return a


if EAL.does_asset_exist(PATH):
    s = EAL.load_asset(PATH)
else:
    s = unreal.AssetToolsHelpers.get_asset_tools().create_asset(PATH.split("/")[-1], PATH.rsplit("/", 1)[0], unreal.GOTAHeroBodyAnimSet,
                                                                  unreal.DataAssetFactory())
s.set_editor_property("idle", load(A + "Idle_9"))
s.set_editor_property("walk", load(A + "Walking_Woman"))
s.set_editor_property("run", load(A + "Run_02"))
s.set_editor_property("jump", load(IP + "Regular_Jump_InPlace"))
s.set_editor_property("death", load(A + "Dead"))

# Run plays at its authored rate at the combat frame's top ground speed; walk below a third of it.
pawn = unreal.get_default_object(unreal.load_class(None, "/Game/Player/BP_Player.BP_Player_C"))
move = pawn.get_editor_property("character_movement")
top = float(move.get_editor_property("max_walk_speed"))
s.set_editor_property("run_speed", top)
s.set_editor_property("walk_speed", round(top / 3.0))

maps = []
for montage, clip, upper, keys in ACTIONS:
    m = unreal.GOTABodyActionMap()
    m.set_editor_property("source_montage", load(montage))
    m.set_editor_property("sequence", load(clip))
    m.set_editor_property("upper_body_only", upper)
    m.set_editor_property("time_keys", [unreal.Vector2D(x, y) for x, y in keys])
    maps.append(m)
s.set_editor_property("actions", maps)
EAL.save_asset(PATH, only_if_is_dirty=False)
unreal.log("AYLA SET saved: %d actions, walk %.0f run %.0f cm/s (BP_Player max walk speed %.0f)" % (len(maps), round(top / 3.0), top, top))
