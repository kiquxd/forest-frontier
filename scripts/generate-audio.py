#!/usr/bin/env python3
"""Compose original loopable ambient scores and effects using standard Python.
No sampled music or external assets; output is owned by this project.
"""
from pathlib import Path
import array, math, random, wave
ROOT = Path(__file__).resolve().parents[1] / 'assets/audio'
ROOT.mkdir(parents=True, exist_ok=True)
RATE = 22050

def write(name, samples):
    data = array.array('h', (int(max(-1, min(1, value)) * 32767) for value in samples))
    import sys
    if sys.byteorder != 'little':
        data.byteswap()
    with wave.open(str(ROOT / (name + '.wav')), 'wb') as out:
        out.setnchannels(1); out.setsampwidth(2); out.setframerate(RATE); out.writeframes(data.tobytes())

def note(samples, midi, start, duration, gain, bell=False):
    frequency = 440 * 2 ** ((midi - 69) / 12)
    offset = int(start * RATE)
    for i in range(int(duration * RATE)):
        t = i / RATE
        envelope = min(1, t / .18) * min(1, (duration - t) / .65)
        if bell: envelope *= math.exp(-t * 1.8)
        tone = math.sin(t * frequency * math.tau) + .18 * math.sin(t * frequency * math.tau * 2) + .06 * math.sin(t * frequency * math.tau * 3)
        samples[(offset + i) % len(samples)] += tone * envelope * gain

for name, roots, style in [('forest', [48, 53, 55, 48, 57, 53, 55, 48], 0), ('harbor', [50, 55, 57, 50, 59, 55, 57, 50], 1), ('depths', [45, 48, 41, 45, 50, 48, 41, 45], 2)]:
    samples = [0.] * (RATE * 32)
    for bar, root in enumerate(roots):
        for interval in [0, 7, 12, 16 if style != 2 else 15]:
            note(samples, root + interval, bar * 4, 5.2, .033)
        melody = [12, 19, 16 if style != 2 else 15, 24, 19, 12, 7, 19]
        for beat, interval in enumerate(melody):
            note(samples, root + interval, bar * 4 + beat * .5, 1.3, .075 if style == 1 else .045, True)
    write(name, samples)

random.seed(742019)
for name, length, frequency in [('step_grass', .14, 90), ('step_stone', .13, 180), ('step_water', .24, 130), ('swing', .24, 400), ('hit', .18, 70), ('gather', .22, 220), ('ui', .16, 620), ('bubbles', .5, 300), ('forest_air', 4, 70), ('water_air', 4, 120), ('fire_air', 4, 200), ('wolf', .5, 65)]:
    samples=[]; smooth=0
    for i in range(int(length * RATE)):
        t=i/RATE; smooth=smooth*.93+random.uniform(-1,1)*.07
        envelope=math.sin(math.pi*t/length)**2
        tone=math.sin(math.tau*frequency*t*(1+.2*t))
        value = smooth * .8 if name.endswith('air') else (tone*.22+smooth*.7)
        samples.append(value*envelope*.65)
    write(name, samples)
print('Generated three original 32-second music loops and twelve effects.')
