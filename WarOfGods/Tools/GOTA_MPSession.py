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
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import GOTA_ResourceHealth as RH  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EDITOR = os.environ.get("GOTA_EDITOR", r"K:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe")
PROJECT = os.path.join(ROOT, "WarOfGods.uproject")
PACKAGED = os.environ.get("GOTA_PACKAGED", os.path.join(ROOT, "Saved", "Packaged", "Windows", "WarOfGods.exe"))
LOGS = os.path.join(ROOT, "Saved", "MPTests")


def packaged_exe(path):
    """The staged bootstrap spawns Binaries/Win64/WarOfGods.exe and exits. Killing the stub
    leaves the game running, so disconnect/forfeit tests never see the client leave."""
    folder = os.path.dirname(os.path.abspath(path))
    inner = os.path.join(folder, "WarOfGods", "Binaries", "Win64", "WarOfGods.exe")
    if os.path.isfile(inner):
        return inner
    return path


PORT = os.environ.get("GOTA_PORT", "")


def launch(packaged, url, script, log, extra_exec="", res=(960, 540), pos=(0, 0), nullrhi=False, extra_args=()):
    exe = [packaged_exe(PACKAGED)] if packaged else [EDITOR, PROJECT, "-game"]
    if nullrhi:
        exe = exe + ["-nullrhi"]  # no rendering: several game windows on one GPU otherwise risk a device-removed crash
    cmds = "t.IdleWhenNotForeground 0,gota.Heroes.AllowInDevelopment 1"
    if extra_exec:
        cmds += "," + extra_exec
    cmds += ",gota.Test.Run Tools/Tests/" + script
    # ExecCmds runs after the host's own PostLogin, so DevHeroes and the idle throttle have to be
    # config-time. An unfocused packaged window otherwise sleeps, and the script clock caps each hitch at 0.5s.
    args = exe + [url, "-windowed", "-ResX=%d" % res[0], "-ResY=%d" % res[1], "-WinX=%d" % pos[0], "-WinY=%d" % pos[1],
                  "-nosplash", "-unattended", "-nop4", "-log", "-abslog=" + log,
                  "-ini:Engine:[ConsoleVariables]:gota.Heroes.AllowInDevelopment=1",
                  "-ini:Engine:[ConsoleVariables]:Slate.bAllowThrottling=0",
                  "-ExecCmds=" + cmds]
    if PORT and "listen" in url:
        args.append("-port=" + PORT)
    args += list(extra_args)
    return subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                            # its own process group: a console Ctrl+C/close event sent to the console this script runs in
                            # (other commands run alongside a suite) must not end the game (logged "ConsoleCtrl RequestExit")
                            creationflags=getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0))


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
    ap.add_argument("--menu", action="store_true", help="both processes start on the frontend map; scripts drive Host/Find/Join")
    ap.add_argument("--host-exec", default="", help="extra console commands for the host (e.g. NetEmulation.PktLag 120)")
    ap.add_argument("--kill-client", type=int, default=0, help="which client --kill-client-at kills (1-based; 0 = last)")
    ap.add_argument("--render-client-script", default="", help="script for rendered clients instead of --client-script")
    ap.add_argument("--render-client-args", default="", help="extra command-line switches for rendered clients only (e.g. \"-rhivalidation -gpucrashdebugging\")")
    ap.add_argument("--resources", choices=("defer", "warn", "off"), default=os.environ.get("GOTA_RESOURCES", "defer"),
                    help="before launch: defer = wait for a quiet machine (default), warn = report only, off = no check "
                         "(CPU, GPU, video memory and RAM used by other programs; Tools/GOTA_ResourceHealth.py)")
    ap.add_argument("--resource-wait", type=float, default=float(os.environ.get("GOTA_RESOURCE_WAIT", "10800")),
                    help="longest wait for a quiet machine (s, default 3 h)")
    ap.add_argument("--resource-timeout", choices=("skip", "run"), default=os.environ.get("GOTA_RESOURCE_TIMEOUT", "skip"),
                    help="still busy at the limit: skip = launch nothing, exit 3 (DEFERRED; default); run = launch anyway")
    a = ap.parse_args()
    os.makedirs(LOGS, exist_ok=True)
    # Other workloads starve the game processes and invalidate timing results (GOTA_ResourceHealth.py).
    say = lambda m: (print(m), sys.stdout.flush())  # noqa: E731
    g = RH.gate(a.resources, a.resource_wait, "session " + a.tag, a.resource_timeout, log=say)
    if not g["run"]:
        RH.journal("session", tag=a.tag, verdict="DEFERRED", gate=g["outcome"])
        say("DEFERRED: session %s not run (still busy after %s: %s)" % (a.tag, RH.dur(g["waited"]), ", ".join(g["reasons"])))
        return 3
    say("RESOURCES at launch (%s): %s%s" % (g["outcome"], g["text"], " -- BUSY START: timing results are not valid" if g["busy_start"] else ""))
    monitor = RH.Monitor() if a.resources != "off" else None
    heroes = [h for h in a.client_heroes.split(",") if h]
    if a.menu:
        host_url = a.url
    else:
        host_url = a.url + ("?Hero=%s" % a.host_hero if a.host_hero else "") + "?DevHeroes=1?listen"
    host_log = os.path.join(LOGS, "%s_host.log" % a.tag)
    # Old logs of the same tag would satisfy the listen check below before the new host has written anything.
    for name in os.listdir(LOGS):
        if name.startswith(a.tag + "_") and name.endswith(".log"):
            try:
                os.remove(os.path.join(LOGS, name))
            except OSError:
                pass
    procs = [("host", launch(a.packaged, host_url, a.host_script, host_log, a.host_exec, pos=(0, 0), nullrhi=a.host_nullrhi))]
    start = time.time()
    if not a.menu:
        # Clients join once the host is listening (a cold host start took 58 s; a fixed delay timed the clients out).
        deadline = time.time() + 180
        while time.time() < deadline and procs[0][1].poll() is None:
            try:
                with open(host_log, encoding="utf-8", errors="ignore") as f:
                    if re.search(r"(GameNetDriver.*listening|Listening on port|IpNetDriver listening)", f.read()):
                        break
            except OSError:
                pass
            time.sleep(1)
        print("host listening after %.0fs" % (time.time() - start))
        time.sleep(max(2.0, a.join_delay - (time.time() - start)))  # never earlier than --join-delay after the host
    else:
        time.sleep(a.join_delay)
    last_join = start
    for i in range(a.clients):
        if i == a.clients - 1 and a.late_join > 0:
            time.sleep(a.late_join)
        hero = heroes[i] if i < len(heroes) else "UPAWarden"
        log = os.path.join(LOGS, "%s_client%d.log" % (a.tag, i + 1))
        pos = (960 * ((i + 1) % 2), 540 * ((i + 1) // 2))
        if a.menu:
            client_url = a.url
        else:
            addr = ("127.0.0.1:%s" % PORT) if PORT else "127.0.0.1"
            client_url = "%s?Hero=%s" % (addr, hero)
        rendered = i < a.render_clients
        script = a.render_client_script if (rendered and a.render_client_script) else a.client_script
        procs.append(("client%d" % (i + 1), launch(a.packaged, client_url, script, log, a.client_exec, pos=pos, nullrhi=not rendered,
                                                   extra_args=a.render_client_args.split() if rendered else ())))
        last_join = time.time()
        time.sleep(a.stagger)
    killed_client = killed_host = False
    harness_kills = {}  # name -> why the harness ended that process
    while time.time() - start < a.timeout:
        if a.kill_client_at > 0 and not killed_client and time.time() - last_join >= a.kill_client_at:
            victim = procs[a.kill_client] if 0 < a.kill_client < len(procs) else procs[-1]
            victim[1].kill()
            killed_client = True
            harness_kills[victim[0]] = "--kill-client-at %.0f" % a.kill_client_at
            print("killed %s at %.0fs" % (victim[0], time.time() - start))
        if a.kill_host_at > 0 and not killed_host and time.time() - start >= a.kill_host_at:
            procs[0][1].kill()
            killed_host = True
            harness_kills["host"] = "--kill-host-at %.0f" % a.kill_host_at
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
            harness_kills.setdefault(name, "still running at the end of the session")
            print("timeout: killed", name)
    # Every process's end: exit code, and whether this harness ended it (a harness kill is not a crash).
    exits = []
    for name, p in procs:
        try:
            code = p.wait(timeout=30)
        except subprocess.TimeoutExpired:
            code = None
        exits.append("%s exit=%s %s" % (name, code, ("killed by harness (%s)" % harness_kills[name]) if name in harness_kills else "exited by itself"))
    with open(os.path.join(LOGS, a.tag + "_exits.txt"), "w") as f:
        f.write("\n".join(exits) + "\n")
    print("exits: " + "; ".join(exits))
    if monitor:
        m = monitor.stop()
        valid = not g["busy_start"] and not m["contended"]
        say("RESOURCES during session: %s -- %s" % (m["text"], "quiet: timing results valid" if valid else
            "CONTENDED (%s): timing results are not valid" % ", ".join(m["reasons"] + (["busy at start"] if g["busy_start"] else []))))
        RH.journal("session", tag=a.tag, verdict="ran", gate=g["outcome"], contended=not valid, during=m["text"])
    print("logs:", os.path.join(LOGS, a.tag + "_*.log"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
