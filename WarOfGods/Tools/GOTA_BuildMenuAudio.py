# Gods of the Arena — original procedural UI cues + menu ambience (offline, NOT run in Unreal).
# Output: Tools/MenuAudioSource/*.wav (48 kHz, 16-bit). Imported by GOTA_BuildMenuUI.py.
# All sounds are synthesized here from scratch — project-owned, no third-party audio.
#   python Tools/GOTA_BuildMenuAudio.py

import os
import wave

import numpy as np

SR = 48000
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Tools", "MenuAudioSource")
os.makedirs(OUT, exist_ok=True)
rng = np.random.default_rng(7)


def write(name, data, stereo=False):
    data = np.clip(data, -1, 1)
    pcm = (data * 32767 * 0.95).astype(np.int16)
    with wave.open(os.path.join(OUT, name), "wb") as w:
        w.setnchannels(2 if stereo else 1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


def env(n, attack, release, curve=3.0):
    t = np.linspace(0, 1, n)
    a = np.clip(t * n / max(1, attack), 0, 1)
    r = (1 - t) ** curve
    return a * r


def tone(freq, dur, harmonics=((1, 1.0),), detune=0.0):
    n = int(SR * dur)
    t = np.arange(n) / SR
    sig = np.zeros(n)
    for mult, amp in harmonics:
        sig += amp * np.sin(2 * np.pi * freq * mult * t * (1 + detune))
    return sig


def lowpass(x, cutoff):
    # one-pole low-pass; cutoff may be an array (sweep)
    cutoff = np.broadcast_to(cutoff, x.shape)
    y = np.zeros_like(x)
    acc = 0.0
    for i in range(len(x)):
        a = 1 - np.exp(-2 * np.pi * cutoff[i] / SR)
        acc += a * (x[i] - acc)
        y[i] = acc
    return y


bell = ((1, 1.0), (2.01, 0.35), (3.02, 0.12), (4.1, 0.05))

# Focus: short, soft metallic tick (distinct but subtle).
n = int(SR * 0.09)
focus = tone(2350, 0.09, bell) * env(n, 40, 1, 6) * 0.35
focus += lowpass(rng.standard_normal(n) * 0.25, 4000) * env(n, 10, 1, 14)
write("S_GOTA_UI_Focus.wav", focus * 0.6)

# Confirm: rising two-note gold chime (fifth).
a = tone(880, 0.32, bell) * env(int(SR * 0.32), 60, 1, 3)
b = tone(1320, 0.42, bell) * env(int(SR * 0.42), 60, 1, 3)
confirm = np.zeros(int(SR * 0.5))
confirm[: len(a)] += a * 0.45
confirm[int(SR * 0.07): int(SR * 0.07) + len(b)] += b * 0.45
write("S_GOTA_UI_Confirm.wav", confirm * 0.7)

# Back: falling soft fourth, darker timbre.
a = tone(990, 0.22, ((1, 1), (2, 0.2))) * env(int(SR * 0.22), 50, 1, 4)
b = tone(742, 0.3, ((1, 1), (2, 0.2))) * env(int(SR * 0.3), 50, 1, 4)
back = np.zeros(int(SR * 0.38))
back[: len(a)] += a * 0.4
back[int(SR * 0.06): int(SR * 0.06) + len(b)] += b * 0.4
write("S_GOTA_UI_Back.wav", back * 0.65)

# Error: low muted double pulse (minor second) — clear, not harsh.
n = int(SR * 0.16)
p = (tone(196, 0.16, ((1, 1), (2, 0.4), (3, 0.2))) + tone(207.6, 0.16, ((1, 0.6),))) * env(n, 30, 1, 4)
err = np.zeros(int(SR * 0.4))
err[:n] += p * 0.45
err[int(SR * 0.17): int(SR * 0.17) + n] += p * 0.38
write("S_GOTA_UI_Error.wav", lowpass(err, 1800) * 0.9)

# Transition: filtered noise swell + low sub thump (deployment).
n = int(SR * 1.1)
t = np.linspace(0, 1, n)
noise = rng.standard_normal(n)
sweep = lowpass(noise, 300 + 5200 * t ** 1.5) * (t ** 1.2) * (1 - t) ** 0.4
sub = np.sin(2 * np.pi * 55 * np.arange(n) / SR) * env(n, 2000, 1, 2) * np.clip((t - 0.7) * 6, 0, 1)
write("S_GOTA_UI_Transition.wav", (sweep * 0.35 + sub * 0.4) * 0.9)

# Ambience loop (24 s, seamless): starship hum + slow air + distant engine swells, stereo.
dur = 24.0
n = int(SR * dur)
t = np.arange(n) / SR
loop_hz = 1 / dur  # every modulation completes whole cycles -> seamless
hum = (np.sin(2 * np.pi * 41 * t) * 0.35 + np.sin(2 * np.pi * 61.5 * t) * 0.18 + np.sin(2 * np.pi * 82 * t) * 0.08)
hum *= 0.75 + 0.25 * np.sin(2 * np.pi * loop_hz * 3 * t)
air_l = lowpass(rng.standard_normal(n), 520) * 0.9
air_r = lowpass(rng.standard_normal(n), 520) * 0.9
swell = 0.5 + 0.5 * np.sin(2 * np.pi * loop_hz * 2 * t - 1.2)
engine = np.sin(2 * np.pi * 118 * t + 2 * np.sin(2 * np.pi * 0.25 * t)) * 0.06 * swell
left = hum + air_l * (0.35 + 0.25 * swell) + engine
right = hum * 0.96 + air_r * (0.35 + 0.25 * (1 - swell)) + engine * 0.8
# crossfade tail into head so the noise beds loop without a click
xf = int(SR * 2.0)
fade = np.linspace(0, 1, xf)
for ch in (left, right):
    ch[:xf] = ch[:xf] * fade + ch[-xf:] * (1 - fade)
left, right = left[:-xf], right[:-xf]
peak = max(np.abs(left).max(), np.abs(right).max())
stereo = np.stack([left, right], axis=1).reshape(-1) / peak * 0.6
write("S_GOTA_Menu_Ambience.wav", stereo, stereo=True)

print("Audio written to", OUT, sorted(os.listdir(OUT)))
