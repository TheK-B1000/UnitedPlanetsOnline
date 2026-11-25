"""Resource health for timing-sensitive test runs (GOTA_AbilityRegression.py, GOTA_MPSession.py).

Another GPU workload (for example a local LLM server holding most of the video memory at 100% use) starves the game
processes: on 2026-11-25 single-player cases fell from about 50 fps to 2-5 fps and hosted multiplayer sessions logged
30-80 s gaps. Dodge, parry and multiplayer timing checks then fail for reasons that have nothing to do with the game.

Two checks, both lightweight:
  * before a case: is the machine quiet? (GPU use and video memory from nvidia-smi, with no game of ours running;
    known competing processes are named in the report). Policy "defer" waits for quiet (up to a limit), "warn" only
    reports, "off" skips the check.
  * after a case: the game's own frame rate from its log. A failing case that ran starved is reported INCONCLUSIVE.

Thresholds: GOTA_GPU_BUSY_PCT (default 30, lowest of three samples), GOTA_VRAM_BUSY_MB (default 6000),
GOTA_MIN_FPS (default 15). Nothing here stops or changes another process.
"""
import datetime
import os
import re
import subprocess
import time

GPU_BUSY_PCT = float(os.environ.get("GOTA_GPU_BUSY_PCT", "30"))
VRAM_BUSY_MB = float(os.environ.get("GOTA_VRAM_BUSY_MB", "6000"))
MIN_FPS = float(os.environ.get("GOTA_MIN_FPS", "15"))
COMPETING = re.compile(os.environ.get("GOTA_COMPETING", r"^(llama-server|ollama_llama_server|ollama runner|comfyui|"
                                                         r"stable-diffusion|koboldcpp|lm studio|lmstudio)"), re.I)
OUR_GAMES = re.compile(r"^(UnrealEditor|WarOfGods)", re.I)


def _run(args, timeout=10):
    try:
        return subprocess.run(args, capture_output=True, text=True, timeout=timeout,
                              creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0)).stdout
    except (OSError, subprocess.SubprocessError):
        return ""


def _processes():
    names = []
    for line in _run(["tasklist", "/fo", "csv", "/nh"]).splitlines():
        m = re.match(r'"([^"]+)"', line)
        if m:
            names.append(re.sub(r"\.exe$", "", m.group(1), flags=re.I))
    return names


def snapshot(samples=3, gap=0.7):
    """{'gpu': lowest GPU % of the samples or None, 'vram': MB or None, 'competing': [...], 'ours': n, 'busy': bool, 'text'}"""
    utils, vram = [], None
    for i in range(samples):
        out = _run(["nvidia-smi", "--query-gpu=utilization.gpu,memory.used", "--format=csv,noheader,nounits"])
        m = re.match(r"\s*(\d+)\s*,\s*(\d+)", out)
        if m:
            utils.append(float(m.group(1)))
            vram = float(m.group(2)) if vram is None else min(vram, float(m.group(2)))
        if i + 1 < samples:
            time.sleep(gap)
    procs = _processes()
    competing = sorted({p for p in procs if COMPETING.search(p)})
    ours = sum(1 for p in procs if OUR_GAMES.search(p))
    gpu = min(utils) if utils else None
    reasons = []
    if gpu is not None and gpu >= GPU_BUSY_PCT:
        reasons.append("GPU %d%%" % gpu)
    if vram is not None and vram >= VRAM_BUSY_MB:
        reasons.append("video memory %.1f GB" % (vram / 1024))
    # With one of our own games still running the figures may be ours, unless a known competitor is also present.
    busy = bool(reasons) and (ours == 0 or bool(competing))
    text = "GPU %s, video memory %s, competing processes: %s%s" % (
        "?" if gpu is None else "%d%%" % gpu, "?" if vram is None else "%.1f GB" % (vram / 1024),
        ", ".join(competing) or "none", ", %d game process(es) of ours running" % ours if ours else "")
    return {"gpu": gpu, "vram": vram, "competing": competing, "ours": ours, "busy": busy,
            "reasons": reasons, "text": text}


def wait_quiet(policy="defer", max_wait=1800.0, label="", log=print):
    """Check before a timing-sensitive step. Returns the snapshot taken at launch time (busy=True if still busy)."""
    if policy == "off":
        return {"busy": False, "text": "not checked", "reasons": []}
    snap = snapshot()
    if not snap["busy"]:
        return snap
    log("RESOURCES busy before %s: %s (%s)" % (label or "this step", ", ".join(snap["reasons"]), snap["text"]))
    if policy != "defer":
        return snap
    start = time.time()
    last_note = start
    while snap["busy"] and time.time() - start < max_wait:
        time.sleep(30)
        snap = snapshot()
        if snap["busy"] and time.time() - last_note >= 300:
            log("RESOURCES still busy after %.0f min: %s" % ((time.time() - start) / 60, snap["text"]))
            last_note = time.time()
    if snap["busy"]:
        log("RESOURCES still busy after waiting %.0f min: running anyway; failures are reported INCONCLUSIVE" % (max_wait / 60))
    else:
        log("RESOURCES quiet after %.0f s: %s" % (time.time() - start, snap["text"]))
    return snap


_STAMP = re.compile(r"^\[(\d{4})\.(\d\d)\.(\d\d)-(\d\d)\.(\d\d)\.(\d\d):(\d{3})\]\[\s*(\d+)\]", re.M)


def log_fps(text, skip_frames=5):
    """Average frames per second of a game log, from its line stamps and frame counters (None if too short)."""
    # The log prints the frame counter modulo 1000: sum the forward steps between lines instead.
    first = last = None
    frames = 0
    started = False
    for m in _STAMP.finditer(text):
        y, mo, d, h, mi, s, ms, fr = map(int, m.groups())
        if not started:
            if fr < skip_frames:
                continue  # start-up lines are all logged on frame 0
            started = True
        t = datetime.datetime(y, mo, d, h, mi, s, ms * 1000)
        if last is not None:
            frames += (fr - last[1]) % 1000
        else:
            first = t
        last = (t, fr)
    if first is None or last is None:
        return None
    secs = (last[0] - first).total_seconds()
    return frames / secs if secs >= 3 and frames > 0 else None
