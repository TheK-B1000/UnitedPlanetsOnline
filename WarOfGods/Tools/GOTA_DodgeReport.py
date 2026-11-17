"""Dodge outcomes per player from a host log (TELEM dodge / perfect_dodge / hit), split listen host vs remote clients.

An armed dodge is one the server saw while an attack was about to land on the player. It ends as a perfect dodge
(no damage before it resolved) or as a hit by the same enemy within 0.8 s.

  python Tools/GOTA_DodgeReport.py rc13_dodge_e1
"""
import collections
import datetime
import os
import re
import sys

LOGS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Saved", "MPTests")
STAMP = re.compile(r"^\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\]")
KV = re.compile(r"(\w+)=(\S+)")


def when(line):
    m = STAMP.match(line)
    return datetime.datetime.strptime(m.group(1), "%Y.%m.%d-%H.%M.%S:%f") if m else None


for tag in sys.argv[1:]:
    path = tag if os.path.exists(tag) else os.path.join(LOGS, tag + "_host.log")
    dodges, perfect, hits = [], [], []
    for line in open(path, encoding="utf-8", errors="ignore"):
        i = line.find("TELEM ")
        if i < 0:
            continue
        kind = line[i + 6:].split(" ", 1)[0]
        d = dict(KV.findall(line[i + 6:]))
        t = when(line)
        if kind == "dodge":
            dodges.append((t, d))
        elif kind == "perfect_dodge":
            perfect.append((t, d))
        elif kind == "hit":
            hits.append((t, d))
    stats = collections.defaultdict(collections.Counter)
    for t, d in dodges:
        p = d.get("player")
        who = "remote" if d.get("remote") == "1" else "host"
        key = (who, p)
        stats[key]["dodges"] += 1
        if d.get("armed") != "1":
            continue
        stats[key]["armed"] += 1
        hit = any(h[1].get("player") == p and h[1].get("by") == d.get("vs") and 0 <= (h[0] - t).total_seconds() <= 0.8 for h in hits)
        ok = any(q[1].get("player") == p and 0 <= (q[0] - t).total_seconds() <= 1.2 for q in perfect)
        stats[key]["perfect" if ok else ("hit anyway" if hit else "neither")] += 1
    print(path)
    for (who, p), c in sorted(stats.items()):
        print("  %-6s %-22s dodges %3d  armed %3d  perfect %3d  hit anyway %3d  neither %3d" % (
            who, p, c["dodges"], c["armed"], c["perfect"], c["hit anyway"], c["neither"]))
    print("  perfect_dodge lines: %d" % len(perfect))
