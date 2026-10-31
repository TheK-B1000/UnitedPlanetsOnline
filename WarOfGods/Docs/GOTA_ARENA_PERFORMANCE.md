# Arena Performance

## Setup

| Item | Value |
|---|---|
| Hardware | NVIDIA GeForce RTX 5070 (dev PC), Windows 11 |
| Build | WarOfGodsEditor Development, standalone `-game`, 1600×900 windowed |
| Settings | Project defaults: Lumen GI + reflections, virtual shadow maps, Nanite, VSync on |
| Scenario | Fight AI duel; the test bot (`gota.Duel.Bot fight`) in live combat with the Sovereign Champion, so movement, montages, AI and combat cues are all active |
| Probe | `gota.Test.Perf 12 combat` samples 12 s (~720 frames). It reports per-frame delta, game-thread time, render-thread time (`GRenderThreadTime`), GPU time (`RHIGetGPUFrameCycles`) and process memory. |

## Results (2026-10-31, after the visual fixes)

| Arena | Frame avg / p95 / max (ms) | FPS | Game thread avg / p95 | GPU avg / p95 | Memory used |
|---|---|---|---|---|---|
| Titan's Quarry | 16.67 / 17.39 / 21.88 | 60 | 3.03 / 3.48 | 6.78 / 7.63 | 4,170 MB |
| Ashfall Crucible | 16.67 / 17.52 / 21.01 | 60 | 3.69 / 4.41 | 7.61 / 8.19 | 3,412 MB |
| Frostbound Citadel | 16.67 / 17.41 / 20.87 | 60 | 3.11 / 3.76 | 6.62 / 7.20 | 3,224 MB |
| Verdant Sanctuary | 16.67 / 17.48 / 20.36 | 60 | 3.54 / 4.15 | **10.46 / 11.51** | 4,800 MB |
| Sunscar Expanse | 16.67 / 17.54 / 19.65 | 60 | 3.08 / 3.62 | 6.70 / 7.26 | 3,178 MB |
| Stormbreak Coast | 16.67 / 17.52 / 20.97 | 60 | 3.39 / 3.95 | 7.36 / 8.06 | 3,144 MB |
| Shattered Dominion | 16.66 / 17.54 / 19.09 | 60 | 3.27 / 3.89 | 7.74 / 8.42 | 4,852 MB |

### Reading the numbers

- **Frame time is VSync-locked at 60 fps.** The render-thread figure (~16.6 ms) includes the VSync wait, so it is omitted.
- **GPU time is the real cost.** Every arena leaves 40–60 % headroom at 60 fps on this GPU.
  - Verdant is the heaviest (about 10.5 ms): it has 40 large Nanite trees with foliage overdraw and strong volumetric fog.
- **The game thread** is around 3–4 ms in combat, with no arena-specific spikes.
- **Max frames** of 19–22 ms are single-frame hitches, at most about one per 12 s window. They come from streaming in new particle and material permutations on first use.
- **Memory** is process-wide (editor-hosted `-game`) and includes editor modules. It is not a shipping budget.
- **Non-Nanite virtual shadow map overflow** (seen in the first Quarry pass) disappeared after the kitbash primitives moved to Nanite copies.

## Uncapped baseline (VSync off, `t.MaxFPS 0`; `Tools/Tests/arena_<key>_perf.txt`, 15 s of live combat)

Measured 2026-10-31 before Environment Pass 2. This is the reference for regression checks.

| Arena | Frame avg / p95 / max (ms) | FPS | Game thread avg | Render thread avg | GPU avg / p95 |
|---|---|---|---|---|---|
| Titan's Quarry | 8.88 / 9.88 / 13.60 | 112.6 | 3.85 | 8.87 | 6.37 / 7.10 |
| Ashfall Crucible | 8.91 / 10.36 / 15.76 | 112.3 | 3.95 | 8.89 | 6.69 / 7.35 |
| Frostbound Citadel | 8.21 / 9.22 / 13.82 | 121.8 | 3.65 | 8.20 | 5.79 / 6.40 |
| Verdant Sanctuary | 11.05 / 12.84 / 25.92 | 90.5 | 3.67 | 11.03 | 10.25 / 11.88 |
| Sunscar Expanse | 8.46 / 9.60 / 15.72 | 118.2 | 3.50 | 8.45 | 5.95 / 6.35 |
| Stormbreak Coast | 8.52 / 9.93 / 15.09 | 117.4 | 3.43 | 8.50 | 6.51 / 7.06 |
| Shattered Dominion | 8.98 / 10.37 / 16.89 | 111.4 | 3.61 | 8.96 | 7.08 / 7.91 |

**Bottleneck: the render thread (CPU), not the GPU.**
- Frame time tracks render-thread time (8.2–9.0 ms) while the GPU finishes in 5.8–7.1 ms.
- Probable cost: per-actor draw submission. There are 400–1,200 individual StaticMeshActors per level.
- Merging repeated pieces into instanced or merged meshes is the main lever.
- Verdant is the exception: there the GPU is the limit (foliage, volumetrics).

## Not measured yet (honest)

- **No Unreal Insights traces.** Draw-call counts, shadow cost and texture-streaming pool usage are not broken down per pass.
- **No packaged build.** Shipping/Test-config numbers will be lower overhead than editor-hosted `-game`.
- **No lower scalability presets** (Medium / Low), no 1440p or 4K runs, and no lower-end GPU.
- **No multiplayer frame times.** Listen server plus clients was not run on the new arenas.
- **Level load time** is not measured: the open plus first-frame time per arena.

## Levers if a target platform needs more headroom

1. **Verdant:**
   - Cap the tree count at about 25, or swap mid-distance trees to the impostor LODs the packs ship with.
   - Lower volumetric fog extinction.
2. **Fog:** `r.VolumetricFog.GridPixelSize 16` on Medium.
3. **Far meshes:** PhotoR horizon and far mountains are already `NoCollision`, no-shadow and static.
4. **Ambient particles:** already reduced to small counts; they can be culled by distance via scalability.
