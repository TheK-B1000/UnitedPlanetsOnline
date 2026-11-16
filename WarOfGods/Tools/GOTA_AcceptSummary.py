"""Summarize a four-player Horde acceptance run: outcome lines, ensures, kill credits, player deaths, and for each
client whether it crashed and how long it stayed up after the victory.

  python Tools/GOTA_AcceptSummary.py rc10_accept1 [rc10_accept2 ...]
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


def text(path):
    return open(path, encoding="utf-8", errors="ignore").read().splitlines() if os.path.exists(path) else []


for tag in sys.argv[1:]:
    host = text(os.path.join(LOGS, tag + "_host.log"))
    outcomes = [l for l in host if "Horde match complete outcome=" in l]
    victory = when(outcomes[0]) if outcomes else None
    parts = []
    for i in (1, 2, 3):
        lines = text(os.path.join(LOGS, "%s_client%d.log" % (tag, i)))
        last = next((when(l) for l in reversed(lines) if when(l)), None)
        crit = sum("Critical error" in l for l in lines)
        rhi = sum("LogRHI: Error" in l for l in lines)
        after = (last - victory).total_seconds() if (last and victory) else float("nan")
        parts.append("c%d: crash=%d rhiErrors=%d up %.0fs after victory" % (i, 1 if crit else 0, rhi, after))
    print("%s: outcomes=%d [%s] ensures=%d kills=%d playerDeaths=%d | %s" % (
        tag, len(outcomes), ",".join(re.search(r"outcome=(\w+)", l).group(1) for l in outcomes),
        sum("Ensure condition failed" in l for l in host), sum("kill credit" in l for l in host),
        sum(bool(re.search(r"Combatant BP_Player\S+ defeated", l)) for l in host), "; ".join(parts)))
