"""Resource health for timing-sensitive test runs (GOTA_AbilityRegression.py, GOTA_MPSession.py).

Other workloads on the same PC (a local LLM server holding most of the video memory at 100% GPU, another project's test
corpus, builds) starve the game processes: on 2026-11-25 single-player cases fell from about 50 fps to 2-5 fps and hosted
multiplayer sessions logged 30-80 s gaps. Dodge, parry and multiplayer timing checks then fail for reasons that have
nothing to do with the game.

What it measures, attributing load to processes that are not ours (UnrealEditor, WarOfGods, shader workers, trace and
cache servers):
  * GPU: per-process engine use from the Windows "GPU Engine" counters, and total video memory (nvidia-smi);
  * CPU: per-process use (psutil), as a share of the whole machine;
  * memory: available RAM.

  gate (before a case or session): wait until the machine has been quiet for QUIET_SECONDS (policy "defer"), or only
      report ("warn"). If it is still busy after the wait limit, the outcome is explicit: "skip" (the default) does not
      run the case and reports it DEFERRED; "run" runs it with the busy start recorded.
  monitor (during a case): samples the same figures every MONITOR_SECONDS and keeps the worst. Contention evidence is
      a busy start or a sample above the thresholds while the case ran.

Classification rule (in the callers): a FAIL becomes INCONCLUSIVE only with contention evidence. A low frame rate with
no external load is NOT excused: it stays FAIL and is flagged as a possible performance regression.

Every gate decision and every case's evidence is appended to Saved/AbilityRegression/resource_gate.jsonl.
Thresholds (environment): GOTA_FOREIGN_GPU_PCT 30 (one other process; an animated desktop wallpaper alone draws about 20%),
GOTA_VRAM_BUSY_MB 6000 (with no game of ours running), GOTA_FOREIGN_CPU_PCT 40 (all other processes together), GOTA_MIN_FREE_RAM_GB 6, GOTA_MIN_FPS 15. Nothing here stops or changes another process.
"""
import datetime
import json
import os
import re
import subprocess
import threading
import time

try:
    import psutil
except ImportError:  # the gate still works from GPU figures alone
    psutil = None

FOREIGN_GPU_PCT = float(os.environ.get("GOTA_FOREIGN_GPU_PCT", "30"))
VRAM_BUSY_MB = float(os.environ.get("GOTA_VRAM_BUSY_MB", "6000"))
FOREIGN_CPU_PCT = float(os.environ.get("GOTA_FOREIGN_CPU_PCT", "40"))
MIN_FREE_RAM_GB = float(os.environ.get("GOTA_MIN_FREE_RAM_GB", "6"))
MIN_FPS = float(os.environ.get("GOTA_MIN_FPS", "15"))
QUIET_SECONDS = float(os.environ.get("GOTA_QUIET_SECONDS", "60"))
MONITOR_SECONDS = float(os.environ.get("GOTA_MONITOR_SECONDS", "30"))
OURS = re.compile(r"^(UnrealEditor|UnrealEditor-Cmd|WarOfGods|ShaderCompileWorker|UnrealTraceServer|zenserver|"
                  r"CrashReportClient|CrashReportClientEditor)(\.exe)?$", re.I)
SYSTEM = re.compile(r"^(System Idle Process|System|Idle|dwm|csrss|Registry|Memory Compression|MsMpEng)(\.exe)?$", re.I)
# Named for the report only (detection is by measured load, so an unnamed workload is caught too).
KNOWN = re.compile(os.environ.get("GOTA_COMPETING", r"llama-server|ollama|cortado|comfyui|stable-diffusion|koboldcpp|lm ?studio"), re.I)
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JOURNAL = os.path.join(ROOT, "Saved", "AbilityRegression", "resource_gate.jsonl")


def _run(args, timeout=15):
    try:
        return subprocess.run(args, capture_output=True, text=True, timeout=timeout,
                              creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0)).stdout
    except (OSError, subprocess.SubprocessError):
        return ""


def journal(event, **fields):
    try:
        os.makedirs(os.path.dirname(JOURNAL), exist_ok=True)
        with open(JOURNAL, "a", encoding="utf-8") as f:
            f.write(json.dumps(dict(time=datetime.datetime.now().isoformat(timespec="seconds"), event=event, **fields)) + "\n")
    except OSError:
        pass


def _proc_table():
    """pid -> (name, is_ours, label) for every process."""
    table = {}
    if psutil:
        for p in psutil.process_iter(["pid", "name", "cmdline"]):
            name = p.info.get("name") or "?"
            cmd = " ".join(p.info.get("cmdline") or [])
            base = re.sub(r"\.exe$", "", name, flags=re.I)
            ours = bool(OURS.match(name)) and not KNOWN.search(cmd)
            label = base
            k = KNOWN.search(cmd) or KNOWN.search(name)
            if k:
                label = "%s(%s)" % (base, k.group(0).lower())
            table[p.info["pid"]] = (name, ours, label)
    return table


def _gpu_by_pid():
    """pid -> GPU engine use % (busiest engine type), from the Windows GPU Engine counters."""
    # Only the engine types work runs on (the full wildcard took 17 s; these take about 2 s).
    ps = ("$s=(Get-Counter '\\GPU Engine(*engtype_3D)\\Utilization Percentage','\\GPU Engine(*engtype_Compute*)\\Utilization Percentage',"
          "'\\GPU Engine(*engtype_Cuda)\\Utilization Percentage' -ErrorAction SilentlyContinue).CounterSamples;"
          "$s | Where-Object { $_.CookedValue -gt 0.5 } | ForEach-Object { $_.InstanceName + '=' + [math]::Round($_.CookedValue,1) }")
    use = {}
    for line in _run(["powershell", "-NoProfile", "-NonInteractive", "-Command", ps], timeout=20).splitlines():
        m = re.match(r"pid_(\d+)_.*engtype_([^=]+)=([\d.]+)", line.strip())
        if m:
            pid, eng, v = int(m.group(1)), m.group(2), float(m.group(3))
            per = use.setdefault(pid, {})
            per[eng] = per.get(eng, 0.0) + v
    return {pid: max(v.values()) for pid, v in use.items()}


def _vram_mb():
    m = re.match(r"\s*(\d+)", _run(["nvidia-smi", "--query-gpu=memory.used", "--format=csv,noheader,nounits"]))
    return float(m.group(1)) if m else None


class _CpuSampler:
    """Per-process CPU between successive calls, as % of the whole machine."""

    def __init__(self):
        self.prev = {}
        self.t = None
        self.sample()

    def sample(self):
        if not psutil:
            return {}
        now, cur = time.time(), {}
        for p in psutil.process_iter(["pid"]):
            try:
                t = p.cpu_times()
                cur[p.info["pid"]] = t.user + t.system
            except (psutil.Error, OSError):
                pass
        out = {}
        if self.t is not None and now > self.t:
            span = (now - self.t) * (psutil.cpu_count() or 1)
            for pid, v in cur.items():
                if pid in self.prev and v >= self.prev[pid]:
                    out[pid] = 100.0 * (v - self.prev[pid]) / span
        self.prev, self.t = cur, now
        return out


def snapshot(cpu=None):
    """Measure the machine. Returns a dict with foreign_gpu, foreign_cpu, vram_mb, free_ram_gb, ours, top, busy, reasons, text."""
    cpu = cpu or _CpuSampler()
    time.sleep(1.5)
    table = _proc_table()
    cpu_use = cpu.sample()
    gpu_use = _gpu_by_pid()
    vram = _vram_mb()
    free_ram = psutil.virtual_memory().available / 2 ** 30 if psutil else None
    foreign = lambda pid: pid in table and not table[pid][1] and not SYSTEM.match(table[pid][0])  # noqa: E731
    f_gpu = max([v for pid, v in gpu_use.items() if foreign(pid)] or [0.0])
    f_cpu = sum(v for pid, v in cpu_use.items() if foreign(pid))
    ours = sum(1 for pid, (n, o, _) in table.items() if o and re.match(r"^(UnrealEditor|WarOfGods)", n, re.I))
    load = {}
    for pid in set(gpu_use) | set(cpu_use):
        if foreign(pid):
            lab = table[pid][2]
            g, c = load.get(lab, (0.0, 0.0))
            load[lab] = (max(g, gpu_use.get(pid, 0.0)), c + cpu_use.get(pid, 0.0))
    top = sorted(load.items(), key=lambda kv: -(kv[1][0] + kv[1][1]))[:4]
    top_txt = ", ".join("%s gpu %d%% cpu %d%%" % (k, g, c) for k, (g, c) in top if g >= 2 or c >= 2) or "none"
    reasons = []
    if f_gpu >= FOREIGN_GPU_PCT:
        reasons.append("other GPU work %d%%" % f_gpu)
    if vram is not None and vram >= VRAM_BUSY_MB and ours == 0:
        reasons.append("video memory %.1f GB in use" % (vram / 1024))
    if f_cpu >= FOREIGN_CPU_PCT:
        reasons.append("other CPU work %d%%" % f_cpu)
    if free_ram is not None and free_ram < MIN_FREE_RAM_GB:
        reasons.append("free RAM %.1f GB" % free_ram)
    text = "other GPU %d%%, other CPU %d%%, video memory %s, free RAM %s; top other load: %s%s" % (
        f_gpu, f_cpu, "?" if vram is None else "%.1f GB" % (vram / 1024), "?" if free_ram is None else "%.1f GB" % free_ram,
        top_txt, "; %d game process(es) of ours running" % ours if ours else "")
    return {"foreign_gpu": f_gpu, "foreign_cpu": f_cpu, "vram_mb": vram, "free_ram_gb": free_ram, "ours": ours,
            "top": top_txt, "busy": bool(reasons), "reasons": reasons, "text": text}


def gate(policy="defer", max_wait=10800.0, label="", timeout_action="skip", log=print):
    """Before a timing-sensitive step. Returns {'run': bool, 'busy_start': bool, 'waited': s, 'outcome', 'reasons', 'text'}."""
    if policy == "off":
        return {"run": True, "busy_start": False, "waited": 0, "outcome": "unchecked", "reasons": [], "text": "not checked"}
    cpu = _CpuSampler()
    snap = snapshot(cpu)
    if not snap["busy"]:
        journal("gate", case=label, outcome="quiet", waited=0, **_ev(snap))
        return {"run": True, "busy_start": False, "waited": 0, "outcome": "quiet", "reasons": [], "text": snap["text"]}
    log("RESOURCES busy before %s: %s | %s" % (label or "this step", ", ".join(snap["reasons"]), snap["text"]))
    if policy == "warn":
        journal("gate", case=label, outcome="busy-warned", waited=0, **_ev(snap))
        return {"run": True, "busy_start": True, "waited": 0, "outcome": "busy-warned", "reasons": snap["reasons"], "text": snap["text"]}
    journal("defer", case=label, **_ev(snap))
    start = last_note = time.time()
    quiet_since = None
    while time.time() - start < max_wait:
        time.sleep(20)
        snap = snapshot(cpu)
        if snap["busy"]:
            quiet_since = None
            if time.time() - last_note >= 600:
                log("RESOURCES still busy after %.0f min: %s" % ((time.time() - start) / 60, snap["text"]))
                journal("still-busy", case=label, waited=round(time.time() - start), **_ev(snap))
                last_note = time.time()
            continue
        quiet_since = quiet_since or time.time()
        if time.time() - quiet_since >= QUIET_SECONDS:  # quiet for a while, not a lull between another tool's jobs
            waited = time.time() - start
            log("RESOURCES quiet for %.0f s after a %.0f min wait: %s" % (QUIET_SECONDS, waited / 60, snap["text"]))
            journal("gate", case=label, outcome="quiet-after-wait", waited=round(waited), **_ev(snap))
            return {"run": True, "busy_start": False, "waited": waited, "outcome": "quiet-after-wait", "reasons": [], "text": snap["text"]}
    waited = time.time() - start
    run = timeout_action == "run"
    log("RESOURCES still busy after the %.0f min limit (%s): %s" % (
        waited / 60, "running anyway, busy start recorded" if run else "NOT RUN, reported DEFERRED", snap["text"]))
    journal("gate", case=label, outcome="timeout-run" if run else "timeout-deferred", waited=round(waited), **_ev(snap))
    return {"run": run, "busy_start": True, "waited": waited, "outcome": "timeout-run" if run else "timeout-deferred",
            "reasons": snap["reasons"], "text": snap["text"]}


def _ev(snap):
    return {k: (round(v, 1) if isinstance(v, float) else v) for k, v in snap.items() if k in
            ("foreign_gpu", "foreign_cpu", "vram_mb", "free_ram_gb", "ours", "top", "reasons")}


class Monitor:
    """Samples other workloads while a case runs; .stop() returns the worst figures and whether they were contention."""

    def __init__(self):
        self.worst = {"foreign_gpu": 0.0, "foreign_cpu": 0.0, "free_ram_gb": None, "top": "none", "samples": 0}
        self.reasons = set()
        self._stop = threading.Event()
        self._cpu = None
        self._t = threading.Thread(target=self._loop, daemon=True)
        self._t.start()

    def _loop(self):
        self._cpu = _CpuSampler()
        while not self._stop.wait(MONITOR_SECONDS):
            s = snapshot(self._cpu)
            w = self.worst
            w["samples"] += 1
            if s["foreign_gpu"] + s["foreign_cpu"] > w["foreign_gpu"] + w["foreign_cpu"]:
                w["top"] = s["top"]
            w["foreign_gpu"] = max(w["foreign_gpu"], s["foreign_gpu"])
            w["foreign_cpu"] = max(w["foreign_cpu"], s["foreign_cpu"])
            if s["free_ram_gb"] is not None:
                w["free_ram_gb"] = s["free_ram_gb"] if w["free_ram_gb"] is None else min(w["free_ram_gb"], s["free_ram_gb"])
            # Video memory and free RAM are not judged here: our own game fills them while it runs.
            self.reasons.update(r for r in s["reasons"] if r.startswith("other "))

    def stop(self):
        self._stop.set()
        self._t.join(timeout=60)
        w = self.worst
        text = "worst other load while running: GPU %d%%, CPU %d%%, free RAM %s (%d samples; top: %s)" % (
            w["foreign_gpu"], w["foreign_cpu"], "?" if w["free_ram_gb"] is None else "%.1f GB" % w["free_ram_gb"], w["samples"], w["top"])
        return {"contended": bool(self.reasons), "reasons": sorted(self.reasons), "text": text,
                **{k: (round(v, 1) if isinstance(v, float) else v) for k, v in w.items()}}


def classify(ok, gate_result, monitor_result, fps_values):
    """Final verdict and reason. INCONCLUSIVE needs contention evidence; low fps alone stays FAIL (flagged)."""
    rates = [r for r in fps_values if r is not None]
    fps = min(rates) if rates else None
    if ok:
        return "PASS", ""
    evidence = []
    if gate_result.get("busy_start"):
        evidence.append("busy at start: " + ", ".join(gate_result.get("reasons") or []))
    if monitor_result and monitor_result.get("contended"):
        evidence.append("during run: " + ", ".join(monitor_result["reasons"]))
    fps_txt = "" if fps is None else "%.1f fps" % fps
    if evidence:
        return "INCONCLUSIVE", "(raw FAIL; resource contention: %s%s) " % ("; ".join(evidence), "; " + fps_txt if fps_txt else "")
    if fps is not None and fps < MIN_FPS:
        return "FAIL", "(%s with no external load measured: possible performance regression) " % fps_txt
    return "FAIL", ""


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
