"""Regression suite for the signature abilities: Gale Rush (Zephyr), Gravity Well (Lysara), Time Lash (Kairos),
Gravity Slam (Orion), Titan Charge (Drakon), Shadow Step (Nyx), Inferno Slam (Varoth).

For every ability it checks, from the game logs: successful activation, cooldown refusal, an invalid target
(out of range / out of radius / no line of sight), death during the cast, and multiplayer (listen server + client).
Each case launches the game with a Tools/Tests script and matches expected (and forbidden) log lines.

  python Tools/GOTA_AbilityRegression.py                 # editor -game (uses the compiled editor build)
  python Tools/GOTA_AbilityRegression.py --packaged      # the packaged Win64 build (GOTA_PACKAGE_DIR)
  python Tools/GOTA_AbilityRegression.py --only GaleRush # one ability
  python Tools/GOTA_AbilityRegression.py --check-only    # re-evaluate the last logs without launching anything

Exit code 1 if any check fails. Logs go to Saved/AbilityRegression/.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UPROJECT = os.path.join(ROOT, "WarOfGods.uproject")
EDITOR = os.environ.get("GOTA_EDITOR", r"K:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe")
PACKAGE_DIR = os.environ.get("GOTA_PACKAGE_DIR", r"B:\Unreal\GOTA_Builds\Pass2_Paragon\Windows")
LOGS = os.environ.get("GOTA_REGRESSION_LOGS", os.path.join(ROOT, "Saved", "AbilityRegression"))
QUARRY = "/Game/Arenas/Maps/L_Arena_TitansQuarry?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3"

# (name, url-options, script, [(regex, expect)])  expect: True = must appear, False = must NOT appear, int = exact count, (lo, hi) = count in range.
CASES = {
    "GaleRush": [
        ("hit+cooldown", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "ability_galerush.txt",
         [(r"GaleRush hit BP_Enemy\S+ for 16\.0 \(landed=1\)", True), (r"GaleRush denied \(Cooldown\)", True)]),
        ("sidestep miss (invalid target)", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "ability_galerush_side.txt",
         [(r"GaleRush \(Charge\) cast", 2), (r"GaleRush hit", 1)]),  # sidestep misses, the control dash hits
        ("death during cast", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "ability_death_cast.txt",
         [(r"GaleRush cancelled", True), (r"GaleRush hit", False)]),
    ],
    "GravityWell": [
        ("hit+cooldown+range+fizzle", "?Opponent=AI?Hero=Lysara?DevHeroes=1", "ability_gravitywell.txt",
         [(r"GravityWell hit BP_Enemy\S+ for 10\.0 \(landed=1\)", True), (r"GravityWell denied \(Cooldown\)", True),
          (r"GravityWell denied \(OutOfRange\)", True), (r"pull target lost .* fizzled", True)]),
        ("line of sight + wall", "?Opponent=AI?Hero=Lysara?DevHeroes=1", "ability_gravitywell_los.txt",
         [(r"GravityWell denied \(NoLineOfSight\)", True), (r"ENCROACH .*inGeometry=1", False)]),
        ("death during cast", "?Opponent=AI?Hero=Lysara?DevHeroes=1", "ability_death_cast.txt",
         [(r"GravityWell cancelled", True), (r"GravityWell hit", False)]),
    ],
    "TimeLash": [
        ("hit+slow+restore+cooldown+radius", "?Opponent=AI?Hero=Kairos?Foe=Zephyr?DevHeroes=1", "ability_timelash_reg.txt",
         [(r"TimeLash hit BP_Enemy\S+ for 6\.0", True), (r"status Slowed 3\.0s x0\.40", True),  # exact walk values: status_slowed.txt (deterministic)
          (r"TimeLash denied \(Cooldown\)", True), (r"meshAnimRate=(?!1\.00)", False)]),
        ("slowed mechanics (stance change, restore)", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "status_slowed.txt",
         [(r"ABILITY f0 move walk=120 ", True), (r"GaleRush \(Charge\) cast", True)]),
        ("death during cast", "?Opponent=AI?Hero=Kairos?DevHeroes=1", "ability_death_cast.txt",
         [(r"TimeLash cancelled", True), (r"status Slowed", False)]),
    ],
    "GravitySlam": [
        ("hit+cooldown+radius+escape+interrupt", "?Opponent=AI?Hero=Orion?DevHeroes=1", "ability_gravityslam.txt",
         [(r"GravitySlam hit BP_Enemy\S+ for 20\.0 \(landed=1\)", True), (r"GravitySlam denied \(Cooldown\)", True),
          (r"GravitySlam cancelled \(interrupted\)", True)]),
        ("unblockable vs blocking player (AI)", "?Opponent=AI?Hero=UPAWarden?Foe=Orion?DevHeroes=1", "ability_gravityslam_ai.txt",
         [(r"GravitySlam hit BP_Player\S+ for 20\.0 \(landed=1\)", True)]),
        ("death during cast", "?Opponent=AI?Hero=Orion?DevHeroes=1", "ability_death_cast.txt",
         [(r"GravitySlam cancelled", True), (r"GravitySlam hit", False)]),
    ],
    # Card-aligned signatures of the Pass 1 gods.
    "TitanCharge": [
        ("hit+cooldown+sidestep", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "ability_titancharge.txt",
         [(r"TitanCharge \(Charge\) cast", 3), (r"TitanCharge hit BP_Enemy\S+ for 14\.0 \(landed=1\)", 2),
          (r"TitanCharge denied \(Cooldown\)", True)]),
        ("lands through a frame stall at launch", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "ability_titancharge_stall.txt",
         [(r"TitanCharge hit BP_Enemy\S+ for 14\.0 \(landed=1\)", 1), (r"TitanCharge missed", False)]),
        ("breaks guard (AI vs blocking player)", "?Opponent=AI?Hero=UPAWarden?Foe=Drakon?DevHeroes=1", "ability_titancharge_block.txt",
         [(r"TitanCharge hit BP_Player\S+ for 14\.0 \(landed=1\)", True)]),
        ("death during cast", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "ability_death_cast_s1.txt",
         [(r"TitanCharge cancelled", True), (r"TitanCharge hit", False)]),
    ],
    "ShadowStep": [
        ("blink behind + cooldown + forward blink", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "ability_shadowstep.txt",
         [(r"ShadowStep \(Teleport\) cast", 2), (r": teleport V\(", 2), (r"ShadowStep denied \(Cooldown\)", True)]),
        ("death during cast", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "ability_death_cast.txt",
         [(r"ShadowStep cancelled", True), (r": teleport V\(", False)]),
    ],
    "InfernoSlam": [
        ("hit+cooldown+radius", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "ability_infernoslam.txt",
         [(r"InfernoSlam hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1), (r"InfernoSlam denied \(Cooldown\)", True),
          (r"InfernoSlam \(AreaBurst\) cast", 2)]),
        ("death during cast", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "ability_death_cast.txt",
         [(r"InfernoSlam cancelled", True), (r"InfernoSlam hit", False)]),
    ],
    # GAS status effects (Docs/GOTA_GAS_ENHANCEMENT_PLAN.md). Dread: walk x0.65, hero-ability damage x0.825.
    "Dread": [
        ("walk+damage once+Time Lash overlap+expiry", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "status_dread.txt",
         [(r"status Dread 7\.0s x0\.35 .*\[GAS walk x0\.65 dmg x0\.8[23]\]", True),
          (r"MoltenStrike hit BP_Enemy\S+ for 18\.[12] \(landed=1\)", 1),     # 22 x 0.825, scaled exactly once
          (r"GAS move scale=0\.65", True), (r"GAS move scale=0\.60", True),
          (r"GAS move scale=0\.(39|3[0-9]|2)", False),                          # the two slows never compound
          (r"gas=\[Dread [0-9.]+s x0\.35\] walkx=0\.65", True),               # Time Lash expired first, Dread still up
          (r"GAS move restored", True), (r"MoltenStrike hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),
          (r"cue GameplayCue\.GOTA\.Status\.Dread on", True), (r"cue GameplayCue\.GOTA\.Status\.Dread removed", True)]),
        ("Nyx Soul Drain applies it; reset clears it", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "status_dread_soul.txt",
         [(r"SoulDrain hit BP_Enemy\S+ for 18\.0 \(landed=1\)", True), (r"status Dread 4\.0s x0\.35", True),
          (r"cleared 1 status effect\(s\) \(reset\)", True)]),
        ("Varoth World Burn applies it; it expires", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "status_dread_burn.txt",
         [(r"WorldBurn hit BP_Enemy\S+ for 5\.0 \(landed=1\)", True), (r"status Dread 4\.0s x0\.35", True),
          (r"cue GameplayCue\.GOTA\.Status\.Dread removed", True)]),
        ("death clears every status", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "status_death.txt",
         [(r"cleared 2 status effect\(s\) \(death\)", True)]),
    ],
    # Veiled: untargetable by abilities + Blueprint IsInvincible; coexists with Dread.
    "Veiled": [
        ("untargetable+invincible+Dread coexists+expiry", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "status_veiled.txt",
         [(r"status Veiled 3\.0s x0\.00", True), (r"Veiled on -> IsInvincible=1", True), (r"Veiled off -> IsInvincible=0", True),
          (r"gas=\[(Dread [0-9.]+s x0\.35 Veiled|Veiled [0-9.]+s x0\.00 Dread)", True),    # both at once (the old slot replaced Dread)
          # hp unchanged across the veiled Molten Strike and Blueprint hit, then the post-veil Blueprint hit lands
          (r"(?s)ABILITY f1 BP_Enemy\S+ hp=([0-9.]+)/.*?ABILITY f1 BP_Enemy\S+ hp=\1/.*?ABILITY f1 BP_Enemy\S+ hp=\1/.*?ABILITY f1 BP_Enemy\S+ hp=(?!\1/)", True),
          (r"gas=\[Dread [0-9.]+s x0\.35\] walkx=0\.65", True),                            # Dread outlived the veil
          (r"MoltenStrike hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),                     # none while veiled, one after
          (r"cue GameplayCue\.GOTA\.Status\.Veiled on", True), (r"cue GameplayCue\.GOTA\.Status\.Veiled removed", True)]),
        ("Nyx Void Shield applies it", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "status_veiled_shield.txt",
         [(r"status Veiled 1\.3s", True), (r"Veiled on -> IsInvincible=1", True), (r"Veiled off -> IsInvincible=0", True)]),
    ],
    # Unbreakable: Blueprint IsInterruptible off, refund of Magnitude x each hit, no cast interrupt; coexists with Slowed.
    "Unbreakable": [
        ("refund+no interrupt+Slowed coexists+expiry", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "status_unbreakable.txt",
         [(r"status Unbreakable 4\.0s x0\.50", True), (r"Unbreakable on -> IsInterruptible=0", True),
          (r"Unbreakable absorbed 10\.0 of 20\.0", True), (r"Unbreakable absorbed 5\.0 of 10\.0", True),
          (r"gas=\[(Unbreakable [0-9.]+s x0\.50 Slowed|Slowed [0-9.]+s x0\.40 Unbreakable)", True),
          (r"InfernoSlam hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),                      # the armoured cast went through
          (r"InfernoSlam cancelled \(interrupted\)", 1),                                    # the unarmoured one did not
          (r"Unbreakable off -> IsInterruptible=1", True), (r"gas=\[Slowed [0-9.]+s x0\.40\] walkx=0\.60", True)]),
        ("Varoth Colossus Form applies it", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "status_colossus.txt",
         [(r"status Unbreakable 6\.0s x0\.50", True), (r"Unbreakable absorbed 10\.0 of 20\.0", True)]),
    ],
    # Boss status policy (Quarry Behemoth: immune to Slow and Stun). No multiplayer case: a boss duel is one player.
    "Boss": [
        ("Time Lash slow resisted + feedback, Dread weakens only", "?Opponent=AI?Hero=Kairos?DevHeroes=1?Boss=QuarryBehemoth", "boss_policy.txt",
         [(r"status policy QuarryBehemoth immune=\[Slow Stun\]", True),
          (r"TimeLash hit BP_Boss\S+ for 6\.0 \(landed=1\)", True),                         # the damage still lands
          (r"BP_Boss\S*: status Slowed resisted \(whole movement \) from BP_Player", True),
          (r"FEEDBACK THE QUARRY BEHEMOTH RESISTS SLOW", True),
          (r"BP_Boss\S*: status Slowed [0-9]", False), (r"BP_Boss\S*: GAS move", False),     # never slowed
          (r"BP_Boss\S*: status Dread 9\.0s x0\.35 .*\[GAS walk x1\.00 dmg x0\.8[23]\]", True),
          (r"Boss ClawCombo hit BP_Player\S+ for 9\.0 \(landed=1\)", True),                   # before Dread
          (r"Boss ClawCombo hit BP_Player\S+ for 7\.4 \(landed=1\)", True),                   # 9 x 0.825 with Dread
          (r"ABILITY f1 gas=\[Dread [0-9.]+s x0\.35\] immune=\[Slow Stun\] walkx=1\.00 dmgx=0\.8[23]", True)]),
        ("Divine Counter, parry and dodge unchanged", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "slice_mechanics.txt",
         [(r"Boss STUNNED by Divine Counter from \S+ for 2\.6s", 2), (r"DIVINE COUNTER by \S+ on BP_Boss\S+: 90 damage \(landed=1", 2),
          (r"Boss PARRIED by", True), (r"perfect dodge", True), (r"status policy QuarryBehemoth immune=\[Slow Stun\]", True)]),
        ("boss death clears statuses, keeps policy", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "boss_death.txt",
         [(r"BP_Boss\S*: cleared 1 status effect\(s\) \(death\)", True), (r"Combatant BP_Boss\S+ defeated", True)]),
    ],
    # Burning (Fire): damage over time through the Blueprint damage system. 2.0 a tick, one tick a second, no tick on
    # application. "Burning tick on" lines carry the health before and after, which is the proof of the damage.
    "Burning": [
        ("Molten Strike ignites: impact once, three exact ticks", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "burn_molten.txt",
         [(r"MoltenStrike hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),
          (r"BP_Enemy\S+: status Burning 3\.0s x2\.00 \(from BP_Player", 1),
          (r"Burning applied: 2\.0 per tick every 1\.00s for 3\.0s, owner BP_Player", 1),
          (r"BP_Player\S+: Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\) hp 238\.0 -> 236\.0 \[GOTA\.Element\.Fire\]", 1),
          (r"Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\) hp 236\.0 -> 234\.0", 1),
          (r"Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\) hp 234\.0 -> 232\.0", 1),
          (r"Burning tick on", 3),                                                          # no tick on application, no fourth
          (r"Momentum BP_Player\S+ \+3 \(hit landed\)", 1),                                  # the strike; ticks are not hits
          (r"ABILITY f0 BP_Player\S+ hp=\S+ momentum=5 ", 3),                                # 3 + 2 from the strike, unchanged by the burn
          (r"ABILITY f1 BP_Enemy\S+ hp=238\.0/", 1), (r"ABILITY f1 BP_Enemy\S+ hp=232\.0/", 2),   # still 232 two seconds later
          (r"ABILITY f1 gas=\[Burning [0-9.]+s x2\.00\]", 1),
          (r"GAS standalone ascs=2 withStatus=1 burning=1 flames=1 burnTicks=0", 1),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=3 burnDamage=6\.0", 1),
          (r"cue GameplayCue\.GOTA\.Status\.Burning on BP_Enemy", True), (r"cue GameplayCue\.GOTA\.Status\.Burning removed from BP_Enemy", 1),
          (r"burn fx P_Fire_Small on BP_Enemy", 1), (r"burn fx removed from BP_Enemy", 1), (r"burn fx missing", False)]),
        ("re-application refreshes, never compounds; repeated cycles clean up", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "burn_refresh.txt",
         [(r"Burning applied: 2\.0 per tick", 5), (r"Burning refreshed: 2\.0 per tick", 1), (r"\(hit landed\)", False),
          (r"GAS standalone ascs=2 withStatus=1 burning=1 flames=1 burnTicks=1 ", 1),        # one effect, one flame after the refresh
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=4 burnDamage=8\.0", 1),
          (r"ABILITY f1 BP_Enemy\S+ hp=252\.0/", 1),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=12 burnDamage=24\.0", 1),
          (r"ABILITY f1 BP_Enemy\S+ hp=236\.0/", 1),
          (r"burn fx P_Fire_Small on BP_Enemy", 5), (r"burn fx removed from BP_Enemy", 5)]),
        ("coexists with Dread and Slowed; obeys Veiled, Unbreakable; no cast interrupt", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "burn_status.txt",
         [(r"ABILITY f1 gas=\[(?=[^\]]*Burning)(?=[^\]]*Dread)(?=[^\]]*Slowed)[^\]]*\] immune=\[\] walkx=0\.60", 1),   # A
          (r"GAS move scale=0\.(39|3[0-9]|2)", False),
          (r"Burning tick on BP_Enemy\S+ for 2\.0 \(landed=\d\) hp ([0-9.]+) -> \1 ", 2),     # B: two ticks under the veil do nothing
          (r"Veiled on -> IsInvincible=1", 1), (r"Veiled off -> IsInvincible=0", 1),
          (r"Unbreakable absorbed 1\.0 of 2\.0", 3),                                         # C
          (r"MoltenStrike hit BP_Enemy\S+ for 18\.[12] \(landed=1\)", 1),                     # D: impact weakened by Dread
          (r"BP_Player\S+: Burning tick on BP_Enemy\S+ for 2\.0 ", 15),                       # A3 + B3 + C3 + D3 + E3 (his strike ignites again), all 2.0
          (r"Burning tick on BP_Player\S+ for 2\.0 \(landed=1\)", 3),                         # E: Drakon burns
          (r"MoltenStrike hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),                        # E: his cast went through
          (r"MoltenStrike cancelled", False),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=18 burnDamage=32\.0", 1)]),   # 18 ticks, two blocked by the veil
        ("death by burning: kill credit, cleanup, no tick after death", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "burn_death.txt",
         [(r"ABILITY f1 BP_Enemy\S+ hp=3\.0/", 1),
          (r"Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\) hp 3\.0 -> 1\.0", 1),
          (r"Burning tick on", 2),
          (r"Combatant BP_Enemy\S+ defeated \(by BP_Player", 1),
          (r"BP_Enemy\S*: cleared 1 status effect\(s\) \(death\)", 1),
          (r"burn fx removed from BP_Enemy", 1),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=2 ", 1)]),
        ("boss: burns by default; Burn immunity and resistance + feedback", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "burn_boss.txt",
         [(r"status policy QuarryBehemoth immune=\[Slow Stun\]", True),
          (r"MoltenStrike hit BP_Boss\S+ for 22\.0 \(landed=1\)", 3),                         # the impact never changes
          (r"BP_Boss\S*: status Burning 3\.0s x2\.00", 1),
          (r"Burning tick on BP_Boss\S+ for 2\.0 \(landed=1\)", 3),
          (r"BP_Boss\S*: status Burning resisted \(whole burn \) from BP_Player", 1),
          (r"FEEDBACK THE QUARRY BEHEMOTH RESISTS BURNING", 1),
          (r"BP_Boss\S*: status Burning resisted \(reduced\) from BP_Player", 1),
          (r"FEEDBACK THE QUARRY BEHEMOTH SHRUGS OFF PART OF BURNING", 1),
          (r"BP_Boss\S*: status Burning 1\.5s x1\.00", 1),
          (r"Burning tick on BP_Boss\S+ for 1\.0 \(landed=1\)", 1),
          (r"Burning tick on", 4),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=4 burnDamage=7\.0", 1)]),
    ],
    # GAS milestone A: hard crowd control (Stunned, Frozen). One lock (GOTA.State.Incapacitated) on server and client.
    "Control": [
        ("Stunned: cast cut, abilities/counter/melee refused, not extended, recovery", "?Opponent=AI?Hero=Drakon?Foe=Drakon?DevHeroes=1", "control_stun.txt",
         [(r"MoltenStrike cancelled \(incapacitated\)", 1),                                   # the AI's wind-up cut short
          (r"status Stunned 1\.5s x0\.00", 2), (r"status Stunned 1\.0s x0\.00", 1),
          (r"CONTROL lock on \(block ended=\d, montages cut=\d+, ai paused=1\)", True),
          (r"GAS move scale=0\.00", True),
          (r"denied \(Incapacitated\)", 2),                                                   # AI slot 1, player slot 0
          (r"status Stunned refused \(already incapacitated, Stunned [0-9.]+s left; not extended\)", 1),
          (r"status Frozen refused \(already incapacitated", 1),                              # one hard CC at a time
          (r"CONTROL lock off \(ai resumed=1, recovery immunity 2\.0s\)", True),
          (r"control recovery: immune to Stun/Freeze for 2\.0s", True),
          (r"status Stunned resisted \(whole control \)", 1),                                 # during the recovery
          (r"FEEDBACK TARGET RESISTS STUN", 1),
          (r"Divine Counter refused \(incapacitated\)", 1),
          (r": MeleeAttack refused \(incapacitated\)", 1),
          (r"CONTROL input disabled \(local\)", 1), (r"CONTROL input enabled \(local\)", 1),
          (r"FEEDBACK STATUS STUNNED on", 1), (r"FEEDBACK STATUS STUNNED off", 1),
          (r"-> M_GOTA_Overlay_Stunned on", True),
          (r"MoltenStrike \(MeleeArc\) cast", 2)]),                                           # AI's, then the player's after the stun
        ("Frozen: held, burn does not shatter, a hit does, expiry, death", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "control_frozen.txt",
         [(r"status Frozen 4\.0s x0\.00", 2), (r"status Frozen 1\.0s x0\.00", 1),
          (r"control cue GameplayCue\.GOTA\.Status\.Frozen on \S+: \d+ montage\(s\) cut, animation held", True),
          (r"-> M_GOTA_Overlay_Frozen on", True),
          (r"(?s)Burning tick on .*Burning tick on .*Frozen shattered by a hit", True),       # two ticks, still frozen, then the hit
          (r"Frozen shattered by a hit", 1),
          (r"animation released", 3),                                                          # shatter, expiry, death
          (r"CONTROL lock on \(.*ai paused=1\)", 3),
          (r"CONTROL lock off \(ai resumed=0, recovery immunity 0\.0s, defeated\)", 1),
          (r"cleared \d+ status effect\(s\) \(death\)", 1)]),
        ("Boss immune to Stun and Freeze + feedback", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "control_boss.txt",
         [(r"BP_Boss\S*: status Stunned resisted \(whole control \)", 1), (r"BP_Boss\S*: status Frozen resisted \(whole control \)", 1),
          (r"FEEDBACK THE QUARRY BEHEMOTH RESISTS STUN", 1), (r"FEEDBACK THE QUARRY BEHEMOTH RESISTS FREEZE", 1),
          (r"CONTROL lock on", False)]),
        ("Mind Spike's last strike stuns (Nyx)", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "control_mindspike.txt",
         [(r"MindSpike hit \S+ for 9\.0 \(landed=1\)", 3), (r"status Stunned 0\.8s x0\.00", 1),
          (r"CONTROL lock on", 1), (r"CONTROL lock off", 1)]),
    ],
    # GAS milestone B: shields (over-health), Fortified / Vulnerable on every source, Empowered, Weakened, Hasted.
    "StatusHUD": [],  # multiplayer only (MP_STATUS): what a remote player is shown
    "StatusHUDDeath": [],  # multiplayer only: chips and shield clear on death, the next pawn starts clean
    "StatusHUDHorde": [],  # multiplayer only: a remote player's shared health bar, behind a shield
    "StatusHUDStory": [],  # the same in Story
    "Defense": [
        ("Shielded: absorbs Blueprint and ability hits, breaks, refresh, stronger, expiry, survives a big hit",
         "?Opponent=AI?Hero=Drakon?DevHeroes=1", "defense_shield.txt",
         [(r"ABILITY f1 \S+ hp=290\.0/290\.0", True), (r"shield=30\.0", True),
          (r"shield absorbed 20\.0 of 20\.0 \(10\.0 left\)", 1),                          # Blueprint hit
          (r"ABILITY f1 \S+ hp=270\.0/270\.0", True),
          (r"shield absorbed 10\.0 of 22\.0 \(0\.0 left\)", 1), (r"shield broken", 2),     # Molten Strike, then the big hit
          (r"ABILITY f1 \S+ hp=248\.0/260\.0", 2),                                        # after the break, after expiry
          (r"ABILITY f1 \S+ hp=288\.0/300\.0", True),
          (r"status Shielded refreshed \(keeps x40\.00; x25\.00 is not stronger\)", 1),
          (r"status Shielded replaced \(x40\.00 -> x60\.00, stronger\)", 1),
          (r"shield ended with 60\.0 left", 1),                                            # expiry takes the unused part back
          (r"shield absorbed 100\.0 of 300\.0 \(0\.0 left\)", 1),
          (r"ABILITY f1 \S+ hp=48\.0/260\.0", True),                                      # 248 + 100 shield - 300: alive
          (r"defeated", False)]),
        ("Fortified, Vulnerable, Empowered, Hasted, Weakened: every source, stacking",
         "?Opponent=AI?Hero=Drakon?DevHeroes=1", "defense_mods.txt",
         [(r"incoming x0\.50 on a Blueprint hit: 20\.0 -> 10\.0 \(refund 10\.0\)", 1),
          (r"incoming x0\.50: 22\.0 -> 11\.0", 1),
          (r"status Fortified refreshed \(keeps x0\.50; x0\.30 is not stronger\)", 1),
          (r"ABILITY f1 gas=\[.*\] immune=\[\] walkx=1\.00 dmgx=1\.00 inx=1\.00", True),  # best reduction x0.5 + vulnerability +0.5
          (r"incoming x1\.50 on a Blueprint hit: 20\.0 -> 30\.0 \(extra 10\.0\)", 1),
          (r"incoming x1\.50: 22\.0 -> 33\.0", 1),
          (r"MoltenStrike hit \S+ for 33\.0 \(landed=1\)", 1), (r"incoming x1\.50: 33\.0 -> 49\.5", 1),   # Empowered x1.5
          (r"GAS move scale=1\.30", True), (r"GAS move scale=0\.90", True),               # Hasted, then Hasted + Slowed
          (r"ABILITY f0 gas=\[.*\] immune=\[\] walkx=0\.90 dmgx=1\.00", True)]),          # Empowered + Weakened
    ],
    # GAS milestone D: hero ability enhancements (card effects) on the shared GAS rules.
    "Heroes": [
        ("Nyx Void Shield: usable while stunned, breaks free, veils, then shields", "?Opponent=AI?Hero=Nyx?Foe=Drakon?DevHeroes=1", "hero_voidshield.txt",
         [(r"ShadowStep denied \(Incapacitated\)", 1), (r"VoidShield \(SelfGuard\) cast", 1),
          (r"VoidShield cleansed 1 control effect\(s\)", 1), (r"BP_Player\S*: CONTROL lock off", 1),
          (r"status Veiled 1\.3s", 1), (r"status Shielded 4\.0s x25\.00", 1), (r"shield up 25\.0", 1)]),
        ("Nyx Soul Drain steals momentum", "?Opponent=AI?Hero=Nyx?Foe=Drakon?DevHeroes=1", "hero_souldrain.txt",
         [(r"SoulDrain hit \S+ for 18\.0 \(landed=1\)", 1), (r"SoulDrain stole 20 momentum from", 1), (r"status Dread 4\.0s x0\.35", 1)]),
        ("Varoth: World Burn ignites, Colossus Form empowers", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "hero_varoth.txt",
         [(r"WorldBurn hit \S+ for 5\.0 \(landed=1\)", 1), (r"BP_Enemy\S*: status Burning 3\.0s x2\.00", True),  # milestone E: the zone refreshes it
          (r"BP_Enemy\S*: status Dread 4\.0s x0\.35", 1),
          (r"status Unbreakable 6\.0s x0\.50", 1), (r"status Empowered 6\.0s x0\.25", 1),
          (r"InfernoSlam hit \S+ for 27\.5 \(landed=1\)", 1)]),
        ("Zephyr Gale Rush shocks (Lightning)", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "hero_elements.txt",
         [(r"GaleRush hit \S+ for 16\.0 \(landed=1\)", 1), (r"BP_Enemy\S*: status Shocked 3\.0s x0\.20", 1)]),
        ("Lysara Gravity Well chills (Frost)", "?Opponent=AI?Hero=Lysara?DevHeroes=1", "hero_elements.txt",
         [(r"GravityWell hit \S+ for 10\.0 \(landed=1\)", 1), (r"BP_Enemy\S*: status Chilled 3\.0s x0\.30", 1)]),
    ],
    # GAS milestone C: element statuses, the follow-up, the interaction table, element resistance.
    "Element": [
        ("Frost chill + deep freeze, Fire thaws, Frost extinguishes, Lightning shocks, Shadow dreads",
         "?Opponent=AI?Hero=Drakon?DevHeroes=1", "element_table.txt",
         [(r"ELEMENT hit GOTA\.Element\.Frost 10\.0 \(x1\.00 element\) -> 10\.0 \(landed=1\)", 3),
          (r"status Chilled 3\.0s x0\.30", 1), (r"GAS move scale=0\.70", True),
          (r"ELEMENT Frost follow-up: Chilled -> Frozen 1\.5s", 1), (r"status Frozen 1\.5s x0\.00", 1), (r"CONTROL lock on", 1),
          (r"ELEMENT Fire thawed Chilled/Frozen \(no Burning from this hit\)", 1), (r"Frozen shattered", False),
          (r"status Burning 3\.0s x2\.00", 1),
          (r"ELEMENT Frost extinguished Burning \(no Chilled from this hit\)", 1),
          (r"status Shocked 3\.0s x0\.20", 1), (r"ABILITY f1 gas=\[.*\] immune=\[\] walkx=1\.00 dmgx=1\.00 inx=1\.20", True),
          (r"status Dread 3\.0s x0\.25", 1), (r"incoming x1\.20: 10\.0 -> 12\.0", 1)]),
        ("Element resistance: elemental hit, ability hit, burn ticks; full resistance",
         "?Opponent=AI?Hero=Drakon?DevHeroes=1", "element_resist.txt",
         [(r"element resistance GOTA\.Element\.Fire x0\.50", 1),
          (r"ELEMENT hit GOTA\.Element\.Fire 20\.0 \(x0\.50 element\) -> 10\.0 \(landed=1\)", 1),
          (r"Burning tick on \S+ for 1\.0 \(landed=1\)", True),
          (r"MoltenStrike element GOTA\.Element\.Fire x0\.50", 1), (r"MoltenStrike hit \S+ for 11\.0 \(landed=1\)", 1),
          (r"ELEMENT hit GOTA\.Element\.Lightning 20\.0 \(x0\.00 element\) -> 0\.0 \(landed=0\)", 1),
          (r"status Shocked", False)]),
        ("Boss: Frost resisted as a slow, never deep-frozen; Lightning shocks",
         "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "element_boss.txt",
         [(r"BP_Boss\S*: status Chilled resisted \(whole movement \)", 2), (r"FEEDBACK THE QUARRY BEHEMOTH RESISTS CHILL", 2),
          (r"follow-up", False), (r"status Frozen", False), (r"BP_Boss\S*: status Shocked 3\.0s x0\.20", 1)]),
    ],
    # GAS milestone E: projectiles and zones, server-decided, through the shared hit path.
    "Delivery": [
        ("Projectile: hit, Veiled passes through, resistance, max range, cleared on reset",
         "?Opponent=AI?Hero=Drakon?DevHeroes=1", "delivery_projectile.txt",
         [(r"PROJECTILE TestPower launched", 5), (r"ELEMENT hit GOTA\.Element\.Lightning 10\.0 \(x1\.00 element\) -> 10\.0 \(landed=1\)", 1),
          (r"status Shocked 3\.0s x0\.20", 1), (r"PROJECTILE TestPower ended \(max range\)", 2),
          (r"ELEMENT hit GOTA\.Element\.Frost 10\.0 \(x0\.00 element\) -> 0\.0 \(landed=0\)", 1), (r"status Chilled", False),
          (r"PROJECTILE TestPower ended \(cleared\)", 1)]),
        ("Zone: pulses on its interval, radius, ends on time, cleared on reset",
         "?Opponent=AI?Hero=Drakon?DevHeroes=1", "delivery_zone.txt",
         [(r"ZONE TestPower started", 3), (r"ELEMENT hit GOTA\.Element\.Shadow 4\.0 \(x1\.00 element\) -> 4\.0 \(landed=1\)", 4),
          (r"ZONE TestPower pulse: 0 target\(s\)", 2), (r"ZONE TestPower ended \(duration\)", 2), (r"ZONE TestPower ended \(cleared\)", 1)]),
        ("World Burn leaves a burning zone", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "delivery_worldburn.txt",
         [(r"WorldBurn hit \S+ for 5\.0 \(landed=1\)", 1), (r"ZONE WorldBurn started by \S+: radius 500, 4\.0s, every 1\.00s, damage 0\.0", 1),
          (r"ZONE WorldBurn pulse: 1 target\(s\)", 4), (r"ZONE WorldBurn ended \(duration\)", 1),
          (r"BP_Player\S*: Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\)", (6, 7))]),  # provisional: 14 burn damage at most, credited to the caster. The last tick can lose a same-frame race with expiry
        ("Overlapping zones do not stack; attribution; shield absorbs the burn", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "delivery_overlap.txt",
         [(r"ZONE TestPower started", 2), (r"BP_Player\S*: Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\)", (4, 5)), (r"Burning tick on \S+ for (?!2\.0)", False),
          (r"shield absorbed 2\.0 of 2\.0", (4, 5))]),
        ("Caster death ends its zone and projectile", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "delivery_cleanup.txt",
         [(r"ZONE TestPower ended \(source gone or round over\)", 1), (r"PROJECTILE TestPower ended \(source gone or round over\)", 1),
          (r"PROJECTILE TestPower hit", False)]),
    ],
    # GAS milestone E: the Quarry Behemoth's supernatural attacks, through the same delivery and damage path.
    "EnemyPower": [
        ("Molten Boulder: hits and ignites, sidestep misses, shield absorbs", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "power_boulder.txt",
         [(r"PROJECTILE MoltenBoulder launched by BP_Boss\S+: damage 12\.0", 3), (r"PROJECTILE MoltenBoulder hit BP_Player\S+ after \d+ \(landed=1\)", 2),
          (r"ELEMENT hit GOTA\.Element\.Fire 12\.0 \(x1\.00 element\) -> 12\.0 \(landed=1\)", 2), (r"BP_Player\S*: status Burning 3\.0s x2\.00", True),
          (r"PROJECTILE MoltenBoulder (ended \(max range\)|stopped by)", True), (r"shield absorbed 12\.0 of 12\.0", 1)]),
        ("Molten Boulder: a timed dodge is protected, an early dodge is not", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "power_dodge.txt",
         [(r"PROJECTILE MoltenBoulder launched", 2), (r"Dodge arm \(\S+\): projectile lead=0\.\d+", 1),
          (r"PROJECTILE MoltenBoulder hit BP_Player\S+ after \d+ \(landed=1\)", 1)]),
        ("Scorched Earth: warned, burns who stays, avoidable, cleared on boss death", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "power_zone.txt",
         [(r"ZONE ScorchedEarth started by BP_Boss\S+: radius 300, 5\.0s, every 1\.00s, damage 0\.0, arms in 1\.0s", 3), (r"ZONE ScorchedEarth armed", 2),
          (r"ZONE ScorchedEarth pulse: 1 target\(s\)", True), (r"ZONE ScorchedEarth pulse: 0 target\(s\)", True), (r"ZONE ScorchedEarth ended \(duration\)", 2),
          (r"ZONE ScorchedEarth ended \(cleared\)", 1), (r"BP_Boss\S*: Burning tick on BP_Player\S+ for 2\.0 \(landed=1\)", True)]),
    ],
}
# Multiplayer: host hero, host script, host-log regex (the ability landing on the remote player), client animation.
MP = {
    "GaleRush": ("Zephyr", "gale_mp_host.txt", r"GaleRush hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Ability_RMB"),
    "GravityWell": ("Lysara", "grav_mp_host.txt", r"GravityWell hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Ability_R_InPlace"),
    "TimeLash": ("Kairos", "timelash_mp_host.txt", r"TimeLash hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Burden"),
    "GravitySlam": ("Orion", "slam_mp_host.txt", r"GravitySlam hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Steel_Ability_GroundSmash_End"),
    "TitanCharge": ("Drakon", "titancharge_mp_host.txt", r"TitanCharge hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Attack_RMB"),
    "ShadowStep": ("Nyx", "shadowstep_mp_host.txt", r": teleport V\(", "Ability_RMB"),
    "InfernoSlam": ("Varoth", "infernoslam_mp_host.txt", r"InfernoSlam hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Swing3_Slow"),
}
# Multiplayer for statuses: host hero, host script, host-log checks, client-log checks (the remote player's own view).
MP_URL = {
    "StatusHUDHorde": "/Game/Arenas/Maps/L_Arena_TitansQuarry?game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry",
    "StatusHUDStory": "/Game/Story/Maps/L_GOTA_Story_Prologue?game=/Script/WarOfGods.GOTAStoryGameMode?Mode=Story",
}
# The remote player's shared health bar: present, and full while the shield takes the hit.
# Enemies may land hits in Horde, so the bar is compared with this player's real health rather than a fixed value.
HUD_BAR_CHECKS = [(r"FEEDBACK CHIPS local=\[Shielded [^\]]*\] shield=\d+/30 hp=(\d+)/100 .* bar=\1\b", True),
                  (r"FEEDBACK CHIPS .*hp=(\d+)/100 .* bar=(?!\1\b)", False)]
# The host's bar must be the host's: never a remote player's health, never a second HUD on its screen.
HOST_BAR_CHECKS = [(r"shield absorbed", True), (r"FEEDBACK CHIPS .*hp=(\d+)/100 .* bar=(?!\1\b)", False),
                   (r"HUD: removed extra W_PlayerHUD\S* \(for BP_Player", True), (r"FEEDBACK CHIPS .* bar=-4\b", False)]

MP_STATUS = {
    "Delivery": ("Drakon", "delivery_mp_host.txt",
                 [(r"PROJECTILE TestPower hit BP_Player\S+ after \d+ \(landed=1\)", 1), (r"ZONE TestPower started", 1),
                  (r"ZONE TestPower pulse: 1 target\(s\)", True), (r"ZONE TestPower ended \(duration\)", 1)],
                 [(r"DELIVERY client sees projectile", 1), (r"DELIVERY client sees zone", 1), (r"DELIVERY client impact", 1),
                  (r"DELIVERY client removed projectile", 1), (r"DELIVERY client removed zone", 1),
                  (r"cue GameplayCue\.GOTA\.Status\.Shocked on BP_Player", True),
                  (r"ELEMENT hit", False), (r"PROJECTILE TestPower hit", False), (r"ZONE TestPower pulse", False)]),  # hits are server-only
    "Element": ("Drakon", "element_mp_host.txt",
                [(r"status Chilled 3\.0s x0\.30", 1), (r"ELEMENT Frost follow-up: Chilled -> Frozen 1\.5s", 1)],
                [(r"cue GameplayCue\.GOTA\.Status\.Chilled on BP_Player", True), (r"overlay \S+ -> M_GOTA_Overlay_Chilled on BP_Player", True),
                 (r"BP_Player\S*: GAS move scale=0\.70", True),
                 (r"cue GameplayCue\.GOTA\.Status\.Frozen on BP_Player", True), (r"FEEDBACK STATUS FROZEN on", 1), (r"FEEDBACK STATUS FROZEN off", 1)]),
    "Defense": ("Drakon", "defense_mp_host.txt",
                [(r"shield up 30\.0", 1), (r"shield absorbed 22\.0 of 22\.0 \(8\.0 left\)", 1), (r"shield ended with 8\.0 left", 1),
                 (r"status Vulnerable 4\.0s x0\.50", 1)],
                [(r"cue GameplayCue\.GOTA\.Status\.Shielded on BP_Player", True), (r"overlay \S+ -> M_GOTA_Overlay_Shielded on BP_Player", True),
                 (r"cue GameplayCue\.GOTA\.Status\.Shielded removed from BP_Player", True),
                 (r"cue GameplayCue\.GOTA\.Status\.Vulnerable on BP_Player", True), (r"cue GameplayCue\.GOTA\.Status\.Vulnerable removed from BP_Player", True)]),
    "StatusHUD": ("Drakon", "f_status_hud_mp_host.txt",
                  [(r"shield absorbed 22\.0 of 22\.0 \(8\.0 left\)", 1), (r"STATUS TIMERS \(server\) \[.*Vulnerable", True),
                   (r"status Vulnerable refreshed|status Vulnerable 3\.0s", True)],
                  # what the remote player was shown: its own chips and shield, then the host's timer as a target
                  [(r"STATUS TIMERS \(client\) \[.*Shielded", True),
                   (r"FEEDBACK CHIPS local=\[Shielded [0-9.]+/6\.0\] shield=30/30", True),
                   (r"FEEDBACK CHIPS local=\[Shielded [0-9.]+/6\.0\] shield=8/30", True),
                   (r"FEEDBACK CHIPS .*target=\S+\[Vulnerable (2\.[5-9]|3\.0)/3\.0\]", True),   # first seen, near full
                   (r"STATUS TIMERS \(client\) \[(Shielded (6\.[1-9]|[7-9])|Vulnerable (3\.[1-9]|[4-9]))", False),  # never more than the duration
                   (r"target=\S+\[Vulnerable 0\.[89]/3\.0\]", False),                            # the host's timer ends on time here
                   (r"FEEDBACK CHIPS .* hp=(?!100/100|0/0)", False),                              # the shield took the hit: health never moved
                   (r"FEEDBACK CHIPS .* bar=(?!-[123]\b)", False),                               # Duel has its own bars: no second HUD on a remote player
                   (r"FEEDBACK CHIPS local=\[.*Weakened [0-9.]+/2\.0.*Slowed [0-9.]+/2\.0", True),   # priority order, both shown
                   (r"FEEDBACK CHIPS local=\[\] shield=0/0 hp=\S+ target=\S+\[\]", True)]),          # everything expired
    "StatusHUDHorde": ("Drakon", "f_status_hud_modes_mp_host.txt", HOST_BAR_CHECKS, HUD_BAR_CHECKS),
    "StatusHUDStory": ("Drakon", "f_status_hud_modes_mp_host.txt", HOST_BAR_CHECKS, HUD_BAR_CHECKS),
    "StatusHUDDeath": ("Drakon", "f_status_hud_death_mp_host.txt",
                       [(r"cleared 3 status effect\(s\) \(death\)", 1), (r"status Slowed 3\.0s x0\.30", 1)],
                       [(r"FEEDBACK CHIPS local=\[Shielded [0-9.]+/20\.0 Dread [0-9.]+/20\.0 Burning [0-9.]+/20\.0\] shield=30/30", True),
                        (r"FEEDBACK STATUS BURNING off", 1),
                        (r"FEEDBACK CHIPS local=\[\] shield=0/0", True),
                        (r"FEEDBACK CHIPS local=\[Slowed [0-9.]+/3\.0\] shield=0/0", True),     # the new pawn: only what it was given
                        (r"FEEDBACK CHIPS local=\[[^\]]*(Shielded|Dread|Burning)[^\]]*Slowed", False)]),
    "Control": ("Drakon", "control_mp_host.txt",
                [(r"status Stunned 2\.0s x0\.00", 1), (r"CONTROL lock on", 2), (r": MeleeAttack refused \(incapacitated\)", 1),
                 (r"status Frozen 4\.0s x0\.00", 1), (r"Frozen shattered by a hit", 1)],
                # the remote player's own machine: input locked and released, its line, the overlay, the ice
                [(r"CONTROL input disabled \(local\)", 2), (r"CONTROL input enabled \(local\)", 2),
                 (r"FEEDBACK STATUS STUNNED on", 1), (r"FEEDBACK STATUS FROZEN on", 1), (r"FEEDBACK STATUS FROZEN off", 1),
                 (r"overlay \S+ -> M_GOTA_Overlay_Stunned on BP_Player", True),
                 (r"control cue GameplayCue\.GOTA\.Status\.Frozen on BP_Player\S+: \d+ montage\(s\) cut, animation held", True),
                 (r"control cue GameplayCue\.GOTA\.Status\.Frozen removed from BP_Player\S+: animation released", True)]),
    "Dread": ("Nyx", "dread_mp_host.txt",
              [(r"SoulDrain hit BP_Player\S+ for [0-9.]+ \(landed=1\)", True), (r"status Dread 4\.0s x0\.35", True)],
              [(r"cue GameplayCue\.GOTA\.Status\.Dread on BP_Player", True), (r"overlay \S+ -> M_GOTA_Overlay_Dread on BP_Player", True), (r"BP_Player\S*: GAS move scale=0\.65", True),
               (r"BP_Player\S*: GAS move scale=0\.60", True), (r"BP_Player\S*: GAS move restored", True),
               (r"GAS move scale=0\.(39|3[0-9]|2)", False), (r"cue GameplayCue\.GOTA\.Status\.Dread removed from BP_Player", True)]),
    "Veiled": ("Drakon", "veiled_mp_host.txt",
               [(r"Veiled on -> IsInvincible=1", True), (r"MoltenStrike hit BP_Player\S+ for 22\.0 \(landed=1\)", 1)],
               [(r"cue GameplayCue\.GOTA\.Status\.Veiled on BP_Player", True), (r"overlay \S+ -> M_GOTA_Overlay_Veiled on BP_Player", True),
                (r"overlay M_GOTA_Overlay_Veiled -> None on BP_Player", True), (r"cue GameplayCue\.GOTA\.Status\.Veiled removed from BP_Player", True)]),
    "Unbreakable": ("Drakon", "unbreakable_mp_host.txt",
                    [(r"Unbreakable on -> IsInterruptible=0", True), (r"Unbreakable absorbed 11\.0 of 22\.0", True)],
                    [(r"cue GameplayCue\.GOTA\.Status\.Unbreakable on BP_Player", True), (r"overlay \S+ -> M_GOTA_Overlay_Unbreakable on BP_Player", True),
                     (r"overlay M_GOTA_Overlay_Unbreakable -> M_GOTA_Overlay_Slowed on BP_Player", True),   # priority hand-off at expiry
                     (r"BP_Player\S*: GAS move scale=0\.60", True),
                     (r"cue GameplayCue\.GOTA\.Status\.Unbreakable removed from BP_Player", True)]),
    "Burning": ("Drakon", "burn_mp_host.txt",
                [(r"MoltenStrike hit BP_Player\S+ for 22\.0 \(landed=1\)", 1), (r"BP_Player\S+: status Burning 3\.0s x2\.00", 1),
                 (r"Burning tick on BP_Player\S+ for 2\.0 \(landed=1\)", 3),
                 (r"GAS listen-server ascs=2 withStatus=0 burning=0 flames=0 burnTicks=3 burnDamage=6\.0", 1)],
                [(r"cue GameplayCue\.GOTA\.Status\.Burning on BP_Player", True), (r"burn fx P_Fire_Small on BP_Player", 1),
                 (r"FEEDBACK STATUS BURNING on", 1), (r"FEEDBACK STATUS BURNING off", 1),
                 (r"burn fx removed from BP_Player", 1), (r"cue GameplayCue\.GOTA\.Status\.Burning removed from BP_Player", 1),
                 (r"Burning tick on", False)]),                                               # ticks are server-only
}


def exe_and_args(packaged):
    if packaged:
        return [os.path.join(PACKAGE_DIR, "WarOfGods.exe")], []
    return [EDITOR, UPROJECT], ["-game"]


def launch(packaged, url, script, log, extra=(), timeout=420, wait=True):
    exe, mode = exe_and_args(packaged)
    args = exe + [url] + mode + ["-windowed", "-ResX=1280", "-ResY=720", "-nosplash", "-unattended", "-nop4", "-log",
                                 "-abslog=" + log, '-ExecCmds=gota.Heroes.AllowInDevelopment 1,gota.Test.Run Tools/Tests/' + script] + list(extra)
    if os.path.exists(log):
        os.remove(log)
    p = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if not wait:
        return p
    try:
        p.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        p.kill()
    return p


def read(log):
    return open(log, encoding="utf-8", errors="ignore").read() if os.path.exists(log) else ""


def check(text, expectations):
    out = []
    for rx, expect in expectations:
        if isinstance(expect, bool):
            out.append((rx, expect, (re.search(rx, text) is not None) == expect))
        elif isinstance(expect, tuple):
            n = len(re.findall(rx, text))
            out.append(("%s (x%d-%d, got %d)" % (rx, expect[0], expect[1], n), True, expect[0] <= n <= expect[1]))
        else:
            n = len(re.findall(rx, text))
            out.append(("%s (x%d, got %d)" % (rx, expect, n), True, n == expect))
    return out


STAMP_RX = re.compile(r"^\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\]", re.M)


def to_s(stamp):
    """HH.MM.SS:mmm -> seconds."""
    return int(stamp[0:2]) * 3600 + int(stamp[3:5]) * 60 + int(stamp[6:8]) + int(stamp[9:12]) / 1000.0


def first_time(text, rx):
    """Timestamp of the first log line matching rx, or '-'."""
    for line in text.splitlines():
        if re.search(rx, line):
            m = STAMP_RX.match(line)
            return m.group(1)[11:] if m else "?"
    return "-"


def keep_failure(run_dir, *logs):
    """Copy a failing case's logs to Saved/AbilityRegression/failures/<run>/ so a rerun cannot overwrite them."""
    os.makedirs(run_dir, exist_ok=True)
    for log in logs:
        if os.path.exists(log):
            shutil.copy2(log, run_dir)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--packaged", action="store_true")
    ap.add_argument("--only", help="comma-separated groups to run")
    ap.add_argument("--skip", default="", help="comma-separated groups to leave out (e.g. groups newer than the build)")
    ap.add_argument("--check-only", action="store_true")
    ap.add_argument("--mp-only", action="store_true", help="only the multiplayer (listen server + client) cases")
    ap.add_argument("--repeat", type=int, default=1, help="run the selected cases N times in a row")
    a = ap.parse_args()
    total_fails = 0
    for rep in range(max(1, a.repeat)):
        if a.repeat > 1:
            print("\n=== pass %d of %d ===" % (rep + 1, a.repeat))
        total_fails += run_suite(a)
    return 1 if total_fails else 0


def run_suite(a):
    run_dir = os.path.join(LOGS, "failures", time.strftime("%Y%m%d_%H%M%S"))
    os.makedirs(LOGS, exist_ok=True)
    if a.packaged and not a.check_only:  # the packaged game reads test scripts from its own project folder
        dst = os.path.join(PACKAGE_DIR, "WarOfGods", "Tools", "Tests")
        os.makedirs(dst, exist_ok=True)
        for f in os.listdir(os.path.join(ROOT, "Tools", "Tests")):
            shutil.copy2(os.path.join(ROOT, "Tools", "Tests", f), dst)
    mode = "packaged" if a.packaged else "editor"
    fails = 0
    for ability, cases in CASES.items():
        if (a.only and ability not in a.only.split(",")) or (a.skip and ability in a.skip.split(",")):
            continue
        for name, opts, script, exp in ([] if a.mp_only else cases):
            log = os.path.join(LOGS, "%s_%s_%s.log" % (mode, ability, re.sub(r"\W+", "_", name)))
            if not a.check_only:
                launch(a.packaged, QUARRY + opts, script, log)
            text = read(log)
            results = check(text, exp) + [("no crash", True, "Fatal error" not in text)]
            ok = all(r[2] for r in results)
            fails += 0 if ok else 1
            if not ok:
                keep_failure(run_dir, log)
            print("%-5s %-12s %-42s %s" % ("PASS" if ok else "FAIL", ability, name, "" if ok else
                  "; ".join(("missing " if must else "unexpected ") + rx for rx, must, good in results if not good)))
            sys.stdout.flush()
        if ability not in MP_STATUS and ability not in MP:
            continue
        if ability in MP_STATUS:
            hero, host_script, host_checks, client_checks = MP_STATUS[ability]
        else:
            hero, host_script, host_rx, anim = MP[ability]
            host_checks = [(host_rx, True)]
            client_checks = [(r"body plays " + anim + r" \(ok", True), (r"BODY \[client\].*hero=" + hero, True)]
        host_log = os.path.join(LOGS, "%s_%s_mp_host.log" % (mode, ability))
        client_log = os.path.join(LOGS, "%s_%s_mp_client.log" % (mode, ability))
        if not a.check_only:
            host = launch(a.packaged, MP_URL.get(ability, QUARRY) + "?Hero=%s?DevHeroes=1?listen" % hero, host_script, host_log, wait=False)
            # Start the client once the host is listening (a cold host start took 49 s; a fixed 15 s launched the
            # client too early and it never joined). The host script itself waits for the client (waitplayers).
            deadline = time.time() + 150
            time.sleep(5)
            while time.time() < deadline and not re.search(r"(GameNetDriver.*listening|Listening on port|IpNetDriver listening)", read(host_log)):
                time.sleep(1)
            time.sleep(2)
            launch(a.packaged, "127.0.0.1?Hero=UPAWarden", "gale_mp_client.txt", client_log, extra=["-LogCmds=LogGOTAAbility Verbose"], timeout=300)
            try:
                host.wait(timeout=180)  # host scripts stay up 75 s after their action so a slow client can finish checking
            except subprocess.TimeoutExpired:
                host.kill()
        h, c = read(host_log), read(client_log)
        results = check(h, host_checks) + check(c, client_checks) + [("no crash", True, "Fatal error" not in h + c)]
        ok = all(r[2] for r in results)
        fails += 0 if ok else 1
        # Connection timeline: when the host listened, when the client logged in and finished loading, when the client
        # script ran. A missing client line with an early or absent join points at timing, not the ability.
        timing = "host listen %s, client join %s, client map %s, client script %s, client body %s" % (
            first_time(h, r"(GameNetDriver.*listening|Listening on port|IpNetDriver listening)"),
            first_time(h, r"Join succeeded"), first_time(c, r"LoadMap: .*L_Arena_TitansQuarry"),
            first_time(c, r"Running .*gale_mp_client"), first_time(c, r"BODY \[client\]"))
        # A starved client stretches its script clock (frame time, capped per frame): report the real length of its
        # first 18 s wait. Far above 18 s means the client process was not getting frames.
        t0, t1 = first_time(c, r"\[script\] wait 18"), first_time(c, r"\[script\] > gota.Test.Body")
        if t0 not in ("-", "?") and t1 not in ("-", "?"):
            timing += ", client 'wait 18' took %.1fs" % (to_s(t1) - to_s(t0))
        # A client whose 18 s script wait took over twice as long was starved of frames (machine load): its client-side
        # checks prove nothing either way. Report INCONCLUSIVE instead of PASS / FAIL (it still fails the run).
        starved_s = None
        if t0 not in ("-", "?") and t1 not in ("-", "?"):
            starved_s = to_s(t1) - to_s(t0)
        elif t0 not in ("-", "?") and first_time(c, r"\[script\] complete") == "-":
            starved_s = float("inf")
        # A host that stalled (one frame of 15 s or more, e.g. compiling shaders for a hero body on first use) runs its
        # timed steps after the client's script has already finished: also INCONCLUSIVE, never PASS.
        stalls = [float(x) for x in re.findall(r"Very long time between ticks\. DeltaTime: ([\d.]+)", h)]
        host_stall = max(stalls) if stalls else 0.0
        if host_stall:
            timing += ", host's longest frame %.1fs" % host_stall
        why = ""
        if not ok and starved_s is not None and starved_s > 36:
            why = "(client starved) "
        elif not ok and host_stall >= 15:
            why = "(host stalled %.0fs) " % host_stall
        verdict = "PASS" if ok else ("INCONCLUSIVE" if why else "FAIL")
        if not ok:
            keep_failure(run_dir, host_log, client_log)
        print("%-5s %-12s %-42s %s" % (verdict, ability, "multiplayer (listen server + client)", "" if ok else
              why +
              "; ".join(("missing " if must else "unexpected ") + rx for rx, must, good in results if not good)))
        print("      %-12s timing: %s" % ("", timing))
        sys.stdout.flush()
    print("\n%s: %s" % (mode, "ALL PASS" if fails == 0 else "%d FAILED" % fails))
    return fails


if __name__ == "__main__":
    sys.exit(main())
