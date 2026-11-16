"""Summarize Unreal CSV profiles (csvprofile start/stop): mean, median, p95 and max of the frame, game thread, render
thread, RHI thread and GPU times, the memory columns, and frames over 50 / 100 ms.

  python Tools/GOTA_PerfCsv.py <profile.csv> [<profile.csv> ...]
"""
import csv
import statistics
import sys

COLUMNS = ["FrameTime", "GameThreadTime", "RenderThreadTime", "RHIThreadTime", "GPUTime",
           "MemoryFreeMB", "PhysicalUsedMB", "VirtualUsedMB"]


def pct(values, p):
    s = sorted(values)
    return s[min(len(s) - 1, int(round(p / 100.0 * (len(s) - 1))))]


for path in sys.argv[1:]:
    with open(path, newline="", encoding="utf-8", errors="ignore") as f:
        rows = list(csv.reader(f))
    header = rows[0]
    data = []
    frame_col = header.index("FrameTime") if "FrameTime" in header else 0
    for r in rows[1:]:
        try:
            float(r[frame_col])
        except (ValueError, IndexError):
            continue  # metadata / footer lines
        data.append(r)
    print("%s: %d frames" % (path, len(data)))
    for col in COLUMNS:
        if col not in header:
            continue
        i = header.index(col)
        vals = []
        for r in data:
            try:
                vals.append(float(r[i]))
            except (ValueError, IndexError):
                pass
        if not vals:
            continue
        print("  %-17s mean %8.2f  median %8.2f  p95 %8.2f  max %8.2f" % (
            col, statistics.mean(vals), statistics.median(vals), pct(vals, 95), max(vals)))
        if col == "FrameTime":
            print("  %-17s >50 ms %d  >100 ms %d" % ("hitches", sum(v > 50 for v in vals), sum(v > 100 for v in vals)))
