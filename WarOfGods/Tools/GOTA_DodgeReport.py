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
        # The dodged attack's own impact is at lead (logged since RC14; older logs say -1, then use 0.8 s). A hit by the
        # same enemy by then is "hit anyway" even if a perfect line follows (a healing harness can mask the drop).
        lead = float(d.get("lead", "-1"))
        limit = lead + 0.15 if lead >= 0 else 0.8
        hit = any(h[1].get("player") == p and h[1].get("by") == d.get("vs") and 0 <= (h[0] - t).total_seconds() <= limit for h in hits)
        ok = any(q[1].get("player") == p and 0 <= (q[0] - t).total_seconds() <= 1.2 for q in perfect)
        if lead >= 0:
            stats[key]["hit anyway" if hit else ("perfect" if ok else "neither")] += 1
        else:
            stats[key]["perfect" if ok else ("hit anyway" if hit else "neither")] += 1
    print(path)
    for (who, p), c in sorted(stats.items()):
        print("  %-6s %-22s dodges %3d  armed %3d  perfect %3d  hit anyway %3d  neither %3d" % (
            who, p, c["dodges"], c["armed"], c["perfect"], c["hit anyway"], c["neither"]))
    print("  perfect_dodge lines: %d" % len(perfect))
    text = open(path, encoding="utf-8", errors="ignore").read()
    iframes = sorted(float(m.group(1)) for m in re.finditer(r"Dodge i-frames \(server-edge\) lasted ([\d.]+)s", text))
    if iframes:
        print("  Blueprint dodge i-frames on the server (host/AI): n=%d min %.2f median %.2f max %.2f s" % (
            len(iframes), iframes[0], iframes[len(iframes) // 2], iframes[-1]))

    # By the server-measured time to impact at the dodge (RC14 logs: lead is logged for unarmed dodges too).
    bins = [(0.0, 0.10), (0.10, 0.20), (0.20, 0.25), (0.25, 0.30), (0.30, 0.36), (0.36, 9.0)]
    table = collections.defaultdict(collections.Counter)
    for t, d in dodges:
        lead = float(d.get("lead", "-1"))
        if lead < 0:
            continue
        who = "remote" if d.get("remote") == "1" else "host"
        b = next(x for x in bins if x[0] <= lead < x[1])
        hit = any(h[1].get("player") == d.get("player") and h[1].get("by") == d.get("vs")
                  and 0 <= (h[0] - t).total_seconds() <= lead + 0.15 for h in hits)
        c = table[(who, b)]
        c["n"] += 1
        c["armed"] += d.get("armed") == "1"
        c["hit"] += hit
        c["armed hit"] += hit and d.get("armed") == "1"
    if table:
        print("  by lead (s)      host: dodges armed hit-by-that-attack | remote: dodges armed hit-by-that-attack")
        for b in bins:
            h, r = table[("host", b)], table[("remote", b)]
            print("   %4.2f-%-5s    %4d %4d %4d            | %4d %4d %4d" % (
                b[0], "%.2f" % b[1] if b[1] < 9 else "+", h["n"], h["armed"], h["hit"], r["n"], r["armed"], r["hit"]))

    # Guards: the server refused exactly the dodged impact. Failure = that attacker's hit still landed at that moment.
    guards = []
    for line in text.splitlines():
        if "Dodge guard: player=" in line:
            m = re.search(r"player=(\S+) refuses (\S+) (\S+)@([\d.]+) \(invincible=(\d)\)", line)
            if m:
                guards.append((when(line), m.group(1), m.group(2), m.group(5)))
    if guards or "Dodge guard expired" in text:
        failed = sum(1 for t, p, a, _ in guards if any(
            h[1].get("player") == p and h[1].get("by") == a and 0 <= (h[0] - t).total_seconds() <= 0.05 for h in hits))
        other = sum(1 for t, p, a, _ in guards for h in hits if h[1].get("player") == p and h[1].get("by") != a
                    and -0.35 <= (h[0] - t).total_seconds() <= 0.15)
        follow = sum(1 for t, p, a, _ in guards if any(
            h[1].get("player") == p and h[1].get("by") == a and 0.05 < (h[0] - t).total_seconds() <= 2.0 for h in hits))
        print("  guards: %d (already invincible: %d); failed (that hit landed anyway): %d; expired: %d" % (
            len(guards), sum(1 for g in guards if g[3] == "0"), failed, text.count("Dodge guard expired")))
        print("  not blanket: other enemies' hits landing within 0.35 s before to 0.15 s after a guarded impact: %d;"
              " guarded attacker's follow-up hits within 2 s: %d" % (other, follow))
