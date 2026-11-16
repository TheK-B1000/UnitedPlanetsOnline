"""List enemy hits on players (and moves) logged after that enemy's own death (TELEM enemy_death), per host log.

  python Tools/GOTA_PostDeathHits.py rc12_full_e4 rc11p_aival_A
"""
import datetime
import os
import re
import sys

LOGS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Saved", "MPTests")
STAMP = re.compile(r"^\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\]")


def when(line):
    m = STAMP.match(line)
    return datetime.datetime.strptime(m.group(1), "%Y.%m.%d-%H.%M.%S:%f") if m else None


for tag in sys.argv[1:]:
    path = tag if os.path.exists(tag) else os.path.join(LOGS, tag + "_host.log")
    deaths = {}
    late = []
    for line in open(path, encoding="utf-8", errors="ignore"):
        if "TELEM spawn " in line:
            m = re.search(r"enemy=(\S+)", line)
            deaths.pop(m.group(1), None)  # actor names are reused once a corpse is destroyed
        elif "TELEM enemy_death" in line:
            m = re.search(r"enemy=(\S+)", line)
            deaths.setdefault(m.group(1), when(line))
        elif "TELEM hit " in line or "TELEM attack " in line:
            m = re.search(r"(?:by|enemy)=(\S+)", line)
            t = when(line)
            if m and m.group(1) in deaths and t:
                dt = (t - deaths[m.group(1)]).total_seconds()
                if dt > 0.05:
                    late.append((dt, line.strip()[30:200]))
    print("%s: %d enemy deaths, %d hit/attack lines from enemies already dead" % (tag, len(deaths), len(late)))
    for dt, l in late[:12]:
        print("  +%.1fs %s" % (dt, l))
