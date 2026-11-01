"""Multiplayer session runner: one listen-server host plus N clients on this machine (each its own process).

Usage:
  python Tools/GOTA_MPSession.py --tag horde4 --url "/Game/Arenas/Maps/L_Arena_TitansQuarry?game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry" \
      --host-script mp_horde_host.txt --client-script mp_horde_client.txt --clients 3 [--client-heroes Nyx,Varoth,Kairos] \
      [--host-hero Drakon] [--client-exec "NetEmulation.PktLag 120,NetEmulation.PktLoss 3"] [--packaged] [--kill-client-at 40] [--kill-host-at 60]

Logs go to Saved/MPTests/<tag>_host.log and <tag>_client<N>.log. Test scripts live in Tools/Tests.
The editor binary runs each process with -game; --packaged uses the packaged Development build instead.
"""
import argparse
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EDITOR = os.environ.get("GOTA_EDITOR", r"K:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe")
PROJECT = os.path.join(ROOT, "WarOfGods.uproject")
PACKAGED = os.environ.get("GOTA_PACKAGED", os.path.join(ROOT, "Saved", "Packaged", "Windows", "WarOfGods.exe"))
LOGS = os.path.join(ROOT, "Saved", "MPTests")


def launch(packaged, url, script, log, extra_exec="", res=(960, 540), pos=(0, 0), nullrhi=False):
    exe = [PACKAGED] if packaged else [EDITOR, PROJECT, "-game"]
    if nullrhi:
        exe = exe + ["-nullrhi"]  # no rendering: several game windows on one GPU otherwise risk a device-removed crash
    cmds = "gota.Heroes.AllowInDevelopment 1"
    if extra_exec:
        cmds += "," + extra_exec
    cmds += ",gota.Test.Run Tools/Tests/" + script
    args = exe + [url, "-windowed", "-ResX=%d" % res[0], "-ResY=%d" % res[1], "-WinX=%d" % pos[0], "-WinY=%d" % pos[1],
                  "-nosplash", "-unattended", "-nop4", "-log", "-abslog=" + log, "-ExecCmds=" + cmds]
    return subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tag", required=True)
    ap.add_argument("--url", required=True, help="host map URL without ?listen")
    ap.add_argument("--host-script", required=True)
    ap.add_argument("--client-script", required=True)
    ap.add_argument("--clients", type=int, default=1)
    ap.add_argument("--host-hero", default="")
    ap.add_argument("--client-heroes", default="")
    ap.add_argument("--client-exec", default="")
    ap.add_argument("--packaged", action="store_true")
    ap.add_argument("--join-delay", type=float, default=14.0)
    ap.add_argument("--stagger", type=float, default=4.0, help="seconds between client joins")
    ap.add_argument("--late-join", type=float, default=0.0, help="extra delay before the LAST client joins (late join)")
    ap.add_argument("--kill-client-at", type=float, default=0.0, help="hard-kill the last client N s after it joined")
    ap.add_argument("--kill-host-at", type=float, default=0.0, help="hard-kill the host N s after launch")
    ap.add_argument("--timeout", type=float, default=420.0)
    ap.add_argument("--render-clients", type=int, default=99, help="only the first N clients render; the rest run -nullrhi")
    ap.add_argument("--host-nullrhi", action="store_true", help="host does not render (keeps the server at full speed)")
    a = ap.parse_args()
    os.makedirs(LOGS, exist_ok=True)
    heroes = [h for h in a.client_heroes.split(",") if h]
    host_url = a.url + ("?Hero=%s" % a.host_hero if a.host_hero else "") + "?DevHeroes=1?listen"
    host_log = os.path.join(LOGS, "%s_host.log" % a.tag)
    procs = [("host", launch(a.packaged, host_url, a.host_script, host_log, pos=(0, 0), nullrhi=a.host_nullrhi))]
    start = time.time()
    time.sleep(a.join_delay)
    last_join = start
    for i in range(a.clients):
        if i == a.clients - 1 and a.late_join > 0:
            time.sleep(a.late_join)
        hero = heroes[i] if i < len(heroes) else "UPAWarden"
        log = os.path.join(LOGS, "%s_client%d.log" % (a.tag, i + 1))
        pos = (960 * ((i + 1) % 2), 540 * ((i + 1) // 2))
        procs.append(("client%d" % (i + 1), launch(a.packaged, "127.0.0.1?Hero=%s" % hero, a.client_script, log, a.client_exec, pos=pos, nullrhi=i >= a.render_clients)))
        last_join = time.time()
        time.sleep(a.stagger)
    killed_client = killed_host = False
    while time.time() - start < a.timeout:
        if a.kill_client_at > 0 and not killed_client and time.time() - last_join >= a.kill_client_at:
            procs[-1][1].kill()
            killed_client = True
            print("killed %s at %.0fs" % (procs[-1][0], time.time() - start))
        if a.kill_host_at > 0 and not killed_host and time.time() - start >= a.kill_host_at:
            procs[0][1].kill()
            killed_host = True
            print("killed host at %.0fs" % (time.time() - start))
        if all(p.poll() is not None for _, p in procs):
            break
        if procs[0][1].poll() is not None and not killed_host:
            time.sleep(10)  # host finished its script: give clients a moment to log the disconnect, then stop them
            break
        time.sleep(1)
    for name, p in procs:
        if p.poll() is None:
            p.kill()
            print("timeout: killed", name)
    print("logs:", os.path.join(LOGS, a.tag + "_*.log"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
