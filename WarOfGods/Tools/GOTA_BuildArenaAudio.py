# Arena overhaul — original procedural ambience loops (offline, numpy). No third-party audio.
#   python Tools/GOTA_BuildArenaAudio.py      -> Tools/ArenaAudioSource/A_GOTA_Amb_*.wav (mono, 44.1 kHz, seamless)
# The level builder imports them into /Game/Arenas/Shared/Audio as looping SoundWaves.
import os
import wave

import numpy as np

SR = 44100
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ArenaAudioSource")
rng = np.random.default_rng(20261030)


def lowpass(x, cutoff):
    a = np.exp(-2.0 * np.pi * cutoff / SR)
    y = np.empty_like(x)
    acc = 0.0
    for i in range(len(x)):
        acc = (1 - a) * x[i] + a * acc
        y[i] = acc
    return y


def lp_fast(x, cutoff):
    # FFT brick-ish low-pass with soft knee (fast; fine for ambience beds).
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1.0 / SR)
    X *= 1.0 / (1.0 + (f / max(cutoff, 1.0)) ** 4)
    return np.fft.irfft(X, len(x))


def hp_fast(x, cutoff):
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1.0 / SR)
    X *= 1.0 - 1.0 / (1.0 + (f / max(cutoff, 1.0)) ** 4)
    return np.fft.irfft(X, len(x))


def bp_fast(x, lo, hi):
    return hp_fast(lp_fast(x, hi), lo)


def lfo(n, hz, depth, phase=0.0):
    t = np.arange(n) / SR
    return 1.0 - depth + depth * (0.5 + 0.5 * np.sin(2 * np.pi * hz * t + phase))


def smooth_noise(n, rate_hz):
    pts = max(4, int(n / SR * rate_hz) + 2)
    v = rng.uniform(0, 1, pts)
    v[-1] = v[0]
    return np.interp(np.linspace(0, pts - 1, n), np.arange(pts), v)


def loopify(x, xfade=1.5):
    # Seamless loop: crossfade the tail into the head.
    k = int(xfade * SR)
    head, tail = x[:k].copy(), x[-k:].copy()
    w = np.linspace(0, 1, k)
    out = x[:-k].copy()
    out[:k] = head * w + tail * (1 - w)
    return out


def norm(x, peak=0.7):
    m = np.max(np.abs(x)) or 1.0
    return x / m * peak


def write(name, x):
    os.makedirs(OUT, exist_ok=True)
    data = (np.clip(x, -1, 1) * 32767).astype(np.int16)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())
    print("wrote", name, round(len(x) / SR, 1), "s")


def events(n, rate_per_s, make, jitter=True):
    out = np.zeros(n)
    t = 0
    while True:
        t += int(rng.exponential(1.0 / rate_per_s) * SR) if jitter else int(SR / rate_per_s)
        if t >= n:
            break
        ev = make()
        e = min(len(ev), n - t)
        out[t:t + e] += ev[:e]
    return out


def decay(n, tau):
    return np.exp(-np.arange(n) / (tau * SR))


def main():
    D = 32
    n = (D + 2) * SR

    # Wind (quarry / desert / frost variants): band-limited noise with gusting envelopes.
    for name, lo, hi, gust, peak in [("Wind_Quarry", 120, 900, 0.07, 0.55), ("Wind_Desert", 200, 1800, 0.09, 0.5),
                                     ("Wind_Frost", 300, 2600, 0.12, 0.55), ("Wind_Storm", 90, 1400, 0.18, 0.7)]:
        w = bp_fast(rng.normal(0, 1, n), lo, hi) * (0.35 + 0.65 * smooth_noise(n, gust * 4))
        w += 0.3 * bp_fast(rng.normal(0, 1, n), hi * 0.8, hi * 2.2) * smooth_noise(n, gust * 9) ** 2
        write("A_GOTA_Amb_" + name, norm(loopify(w), peak))

    # Quarry machinery: deep drone with slow beating, distant chain clanks and stone rumbles.
    t = np.arange(n) / SR
    drone = 0.5 * np.sin(2 * np.pi * 41 * t) + 0.35 * np.sin(2 * np.pi * 41.7 * t) + 0.2 * np.sin(2 * np.pi * 82.3 * t)
    drone *= lfo(n, 0.05, 0.4)
    clank = events(n, 0.35, lambda: bp_fast(rng.normal(0, 1, int(0.6 * SR)), 900, 4200) * decay(int(0.6 * SR), 0.08) * rng.uniform(0.2, 0.6))
    rumble = events(n, 0.08, lambda: lp_fast(rng.normal(0, 1, int(3 * SR)), 120) * np.hanning(int(3 * SR)) * 2.5)
    write("A_GOTA_Amb_Quarry_Machinery", norm(loopify(lp_fast(drone, 300) * 0.5 + clank * 0.6 + rumble), 0.55))

    # Lava: low boiling rumble + bubble pops + crackle.
    bed = lp_fast(np.cumsum(rng.normal(0, 1, n)) * 0.002, 90) * lfo(n, 0.07, 0.5)
    bed = bed - np.mean(bed)
    pops = events(n, 2.5, lambda: lp_fast(rng.normal(0, 1, int(0.25 * SR)), rng.uniform(150, 500)) * decay(int(0.25 * SR), 0.05))
    crackle = events(n, 9.0, lambda: hp_fast(rng.normal(0, 1, int(0.03 * SR)), 2500) * rng.uniform(0.05, 0.25))
    write("A_GOTA_Amb_Lava", norm(loopify(norm(bed, 1.0) + pops * 0.8 + crackle * 0.6), 0.6))

    # Ice: wind bed is separate; this is the sparse creak/groan layer.
    def creak():
        m = int(rng.uniform(0.8, 2.2) * SR)
        f0 = rng.uniform(70, 160)
        tt = np.arange(m) / SR
        f = f0 * (1 + 0.15 * np.sin(2 * np.pi * rng.uniform(1, 3) * tt))
        s = np.sign(np.sin(2 * np.pi * np.cumsum(f) / SR)) * np.hanning(m)
        return bp_fast(s * (0.6 + 0.4 * rng.normal(0, 1, m)), 80, 1600) * 0.6
    write("A_GOTA_Amb_IceCreak", norm(loopify(events(n, 0.12, creak)), 0.45))

    # Waterfall / stream: steady pink-ish noise with gentle motion.
    pink = np.cumsum(rng.normal(0, 1, n))
    pink = hp_fast(pink, 40)
    water = bp_fast(rng.normal(0, 1, n), 300, 6000) * 0.6 + norm(lp_fast(pink, 900), 0.5)
    write("A_GOTA_Amb_Waterfall", norm(loopify(water * lfo(n, 0.11, 0.15)), 0.55))

    # Forest leaves rustle (birds come from StarterContent).
    rustle = hp_fast(rng.normal(0, 1, n), 1500) * smooth_noise(n, 0.6) ** 3
    write("A_GOTA_Amb_ForestLeaves", norm(loopify(rustle), 0.4))

    # Sand hiss moving across stone edges.
    sand = bp_fast(rng.normal(0, 1, n), 2500, 9000) * smooth_noise(n, 0.4) ** 2
    write("A_GOTA_Amb_SandDrift", norm(loopify(sand), 0.35))

    # Ocean surf: breaking waves every ~7–9 s with long foam tails.
    def wave_ev():
        m = int(6 * SR)
        env = np.concatenate([np.linspace(0, 1, int(0.9 * SR)) ** 2, np.exp(-np.arange(m - int(0.9 * SR)) / (1.6 * SR))])
        return bp_fast(rng.normal(0, 1, m), 120, 5000) * env
    surf = events(n, 0.13, wave_ev) + 0.25 * lp_fast(rng.normal(0, 1, n), 200)
    write("A_GOTA_Amb_Surf", norm(loopify(surf), 0.65))

    # Rain: dense high-band noise + droplet ticks.
    rain = hp_fast(rng.normal(0, 1, n), 1800) * 0.5 + events(n, 60, lambda: hp_fast(rng.normal(0, 1, 200), 4000) * rng.uniform(0.1, 0.5))
    write("A_GOTA_Amb_Rain", norm(loopify(rain * lfo(n, 0.05, 0.2)), 0.45))

    # Thunder: long loop with two distant rolls (sparse, never masks combat).
    thunder = np.zeros(n)
    for start in (int(6 * SR), int(21 * SR)):
        m = int(6 * SR)
        roll = lp_fast(rng.normal(0, 1, m), 160) * np.concatenate([np.linspace(0, 1, int(0.15 * SR)), np.exp(-np.arange(m - int(0.15 * SR)) / (1.4 * SR))])
        roll *= 1 + 0.6 * smooth_noise(m, 3)
        thunder[start:start + m] += roll
    write("A_GOTA_Amb_Thunder", norm(loopify(thunder), 0.6))

    # One-shot thunder clap + roll for AGOTALightningFlash (not looped).
    m = int(5.5 * SR)
    clap = hp_fast(rng.normal(0, 1, int(0.25 * SR)), 300) * decay(int(0.25 * SR), 0.05)
    roll = lp_fast(rng.normal(0, 1, m), 180) * np.concatenate([np.linspace(0, 1, int(0.2 * SR)), np.exp(-np.arange(m - int(0.2 * SR)) / (1.3 * SR))])
    roll *= 1 + 0.7 * smooth_noise(m, 4)
    one = roll * 0.9
    one[:len(clap)] += clap * 0.5
    write("A_GOTA_SFX_Thunder", norm(one, 0.7))


if __name__ == "__main__":
    main()
