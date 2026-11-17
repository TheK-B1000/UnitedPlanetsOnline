"""Turn the TELEM lines of a host log (UGOTACombatTelemetry, development builds) into a Horde balance report.

  python Tools/GOTA_HordeTelemetry.py rc11_ready_all            (Saved/MPTests/<tag>_host.log)
  python Tools/GOTA_HordeTelemetry.py path/to/WarOfGods.log     (any host log, e.g. a human playtest)

Reports readiness, each wave (duration, kills, player deaths, damage), each player (hits, damage, deaths, life),
each enemy class and attack (attempts, hit rate, damage per hit and per landed attack, attack rate), enemy
time-to-kill, simultaneous attackers, movement failures and stalls.
"""
import collections
import datetime
import os
import re
import statistics
import sys

LOGS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Saved", "MPTests")
STAMP = re.compile(r"^\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\]")
KV = re.compile(r"(\w+)=(\S+)")


def when(line):
    m = STAMP.match(line)
    return datetime.datetime.strptime(m.group(1), "%Y.%m.%d-%H.%M.%S:%f") if m else None


def num(v, default=0.0):
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


def fmt(values, digits=1):
    if not values:
        return "-"
    return "%.*f (median %.*f, n=%d)" % (digits, statistics.mean(values), digits, statistics.median(values), len(values))


def report(path):
    lines = open(path, encoding="utf-8", errors="ignore").read().splitlines()
    events = []
    readiness = []
    for line in lines:
        if "Horde readiness:" in line:
            readiness.append(line.split("Horde readiness:", 1)[1].strip())
        i = line.find("TELEM ")
        if i < 0:
            continue
        body = line[i + 6:]
        kind = body.split(" ", 1)[0]
        events.append((when(line), kind, dict(KV.findall(body))))

    print("=" * 100)
    print(os.path.basename(path))
    if readiness:
        print("Readiness:")
        for r in readiness:
            print("  " + r)

    waves = [e for e in events if e[1] == "wave_end"]
    print("\nWaves:")
    for _, _, d in waves:
        print("  wave %s%s: %s after %ss, planned %s, killed %s, player deaths %s, hits on players %s, damage to players %s, max engaged %s"
              % (d.get("wave"), " (boss)" if d.get("boss") == "1" else "", d.get("outcome"), d.get("dur"), d.get("planned"),
                 d.get("kills"), d.get("playerDeaths"), d.get("hitsOnPlayers"), d.get("dmgToPlayers"), d.get("maxEngaged")))
    for _, _, d in (e for e in events if e[1] == "match_end"):
        print("  match: %s in wave %s" % (d.get("outcome"), d.get("wave")))

    wave_players = [d for _, k, d in events if k == "wave_player"]
    if wave_players:
        print("\nEach player, each wave (avoided = blocks + parries + perfect dodges over everything that reached them):")
        print("  %-5s %-24s %6s %5s %6s %7s %6s %7s %7s %6s %10s %7s %5s" % (
            "wave", "player", "dmgIn", "hits", "blocks", "parries", "dodges", "perfect", "avoided", "deaths", "firstDeath", "dmgOut", "kills"))
        for d in wave_players:
            print("  %-5s %-24s %6s %5s %6s %7s %6s %7s %7s %6s %10s %7s %5s" % (
                d.get("wave"), d.get("player"), d.get("dmgTaken"), d.get("hits"), d.get("blocks"), d.get("parries"),
                d.get("dodges"), d.get("perfectDodges"), d.get("avoided"), d.get("deaths"),
                d.get("firstDeath") if d.get("firstDeath") != "-1.0" else "-", d.get("dmgDealt"), d.get("kills")))
    dodge_events = [d for _, k, d in events if k == "dodge"]
    if dodge_events:
        armed = collections.Counter("remote" if d.get("remote") == "1" else "host" for d in dodge_events if d.get("armed") == "1")
        perf = collections.Counter("remote" if d.get("remote") == "1" else "host" for _, k, d in events if k == "perfect_dodge")
        print("\nDodges: %d seen by the server; armed (attack about to land): host %d, remote %d; perfect: host %d, remote %d"
              % (len(dodge_events), armed["host"], armed["remote"], perf["host"], perf["remote"]))

    hits = [e for e in events if e[1] == "hit"]
    deaths = [e for e in events if e[1] == "death"]
    print("\nPlayers:")
    per = collections.defaultdict(lambda: {"hits": 0, "dmg": 0.0, "deaths": 0, "life": [], "att": 0, "nodmg": 0, "heal": 0.0})
    for _, _, d in hits:
        p = per[d.get("player")]
        p["hits"] += 1
        p["dmg"] += num(d.get("dmg"))
        p["att"] = max(p["att"], int(num(d.get("attackers"))))
    for _, _, d in deaths:
        p = per[d.get("player")]
        p["deaths"] += 1
        if num(d.get("life"), -1) >= 0:
            p["life"].append(num(d.get("life")))
    for _, k, d in events:
        if k == "nodamage":
            per[d.get("player")]["nodmg"] += 1
        elif k == "heal":
            hp = d.get("hp", "0->0").split("->")
            per[d.get("player")]["heal"] += num(hp[1]) - num(hp[0])
    for name, p in sorted(per.items()):
        print("  %-24s hits %3d  damage %5.0f  no-damage hits %3d  healed %4.0f  deaths %d  life %s  max attackers %d"
              % (name, p["hits"], p["dmg"], p["nodmg"], p["heal"], p["deaths"], fmt(p["life"]), p["att"]))
    death_wave_times = [num(d.get("waveTime"), -1) for _, _, d in deaths if num(d.get("waveTime"), -1) >= 0]
    print("  deaths, seconds into their wave: %s" % fmt(death_wave_times))

    print("\nEnemy attacks (regular enemies, one line per attack montage):")
    attacks = [d for _, k, d in events if k == "attack"]
    by_montage = collections.defaultdict(lambda: collections.Counter())
    dmg_montage = collections.defaultdict(list)
    for d in attacks:
        key = (d.get("class"), d.get("montage"))
        by_montage[key][d.get("result")] += 1
        if num(d.get("dmg")) > 0:
            dmg_montage[key].append(num(d.get("dmg")))
    for key, results in sorted(by_montage.items()):
        total = sum(results.values())
        landed = results["hit"] + (len(dmg_montage[key]) - results["hit"] if len(dmg_montage[key]) > results["hit"] else 0)
        print("  %-18s %-32s attempts %3d  %s  damage per landed attack %s"
              % (key[0], key[1], total, " ".join("%s %d" % kv for kv in sorted(results.items())), fmt(dmg_montage[key])))
    per_hit = collections.defaultdict(list)
    for _, _, d in hits:
        per_hit[d.get("class")].append(num(d.get("dmg")))
    print("  damage per hit: " + "; ".join("%s %s" % (c, fmt(v)) for c, v in sorted(per_hit.items())))

    spawns = {d.get("enemy"): t for t, k, d in events if k == "spawn"}
    edeaths = [(t, d) for t, k, d in events if k == "enemy_death"]
    print("\nEnemies:")
    ttk = collections.defaultdict(list)
    alive = collections.defaultdict(list)
    rate = collections.defaultdict(list)
    for t, d in edeaths:
        c = d.get("class")
        ttk[c].append(num(d.get("ttk")))
        alive[c].append(num(d.get("alive")))
        if num(d.get("alive")) > 5:
            rate[c].append(num(d.get("attacks")) * 60.0 / num(d.get("alive")))
    classes = sorted(set(d.get("class") for _, k, d in events if k == "spawn") | set(ttk))
    for c in classes:
        spawned = sum(1 for _, k, d in events if k == "spawn" and d.get("class") == c)
        print("  %-24s spawned %3d  killed %3d  time-to-kill (first hit to death) %s  lifetime %s  attacks/min %s"
              % (c, spawned, len(ttk[c]), fmt(ttk[c]), fmt(alive[c]), fmt(rate[c])))
    boss = [d for _, k, d in events if k == "boss_attack"]
    if boss:
        switches = sum(1 for d in boss if d.get("switched") == "1")
        ends = collections.Counter(d.get("result") for _, k, d in events if k == "boss_attack_end")
        print("  boss attacks %d, target switches %d, results %s" % (len(boss), switches, dict(ends)))
        for name, n in collections.Counter(d.get("attack") for d in boss).most_common():
            print("    %-24s %d" % (name, n))

    fails = collections.Counter((d.get("class"), d.get("result")) for _, k, d in events if k == "move_fail")
    stalls = [d for _, k, d in events if k == "stall"]
    print("\nNavigation and attack-state problems:")
    print("  move failures (logged lines): %s" % (dict(fails) or "none"))
    print("  stalls (alive, >300 cm from every player, not moving or attacking for 8 s): %d" % len(stalls))
    for d in stalls[:10]:
        print("    %s nearest player %s cm, focus %s, wave %s" % (d.get("enemy"), d.get("nearestPlayer"), d.get("focus"), d.get("wave")))
    engaged = [int(num(d.get("enemies"))) for _, k, d in events if k == "engaged"]
    print("  most enemies within 4 m of one player: %s" % (max(engaged) if engaged else "-"))
    starts = [i for i, e in enumerate(events) if e[1] == "summary_begin"]
    if starts:
        print("\nLast server summary (%s):" % events[starts[-1]][2].get("why"))
        for _, k, d in events[starts[-1] + 1:]:
            if k == "summary_end":
                break
            if k in ("class", "player") and not (k == "class" and d.get("spawned") == "0" and d.get("attacks") == "0"):
                print("  %s %s" % (k, " ".join("%s=%s" % kv for kv in d.items())))


for arg in sys.argv[1:]:
    p = arg if os.path.exists(arg) else os.path.join(LOGS, arg + "_host.log")
    report(p)
