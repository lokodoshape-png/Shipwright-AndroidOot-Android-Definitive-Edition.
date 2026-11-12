#!/usr/bin/env python3
"""OOT D.E.: original orchestral theme of the Definitive Edition main menu ("Tema do Menu Definitive").

The piece is composed here, note by note (it is not an arrangement of any existing music), written as MIDI and
played by the FluidR3 GM SoundFont (MIT license), then turned into a seamless loop and saved as Ogg Vorbis:
soh/assets/custom/ootde/audio/gDEMenuTheme.ogg (added to soh.otr by scripts/otr_add_custom.py).

    apt install fluidsynth fluid-soundfont-gm ffmpeg
    pip install mido numpy
    python3 scripts/ootde_menu_music.py

D major, 3/4, 88 beats per minute, 40 bars (about 82 s):
    intro (4)  harp and strings, a horn call
    A (8)      flute melody over strings and harp
    A' (8)     violins take the melody, horns answer, timpani
    B (8)      B minor, cellos, the brass build up
    C (8)      everyone: trumpets and violins, choir, cymbals
    outro (4)  the flute again, a borrowed G minor, back to the start
"""
import os
import random
import subprocess
import sys
import tempfile
import wave

import mido
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "soh", "assets", "custom", "ootde", "audio", "gDEMenuTheme.ogg")
SOUNDFONT = "/usr/share/sounds/sf2/FluidR3_GM.sf2"

BPM = 88
TPB = 480  # ticks per beat
BAR = 3 * TPB
RATE = 44100

NOTE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def n(name):
    """'F#5' -> MIDI note number (C4 = 60)."""
    pitch = NOTE[name[0]]
    i = 1
    while name[i] in "#b":
        pitch += 1 if name[i] == "#" else -1
        i += 1
    return pitch + 12 * (int(name[i:]) + 1)


# Chords of each bar: root and quality.
CHORDS = {
    "D": (n("D3"), (0, 4, 7)), "Bm": (n("B2"), (0, 3, 7)), "G": (n("G2"), (0, 4, 7)), "A": (n("A2"), (0, 4, 7)),
    "F#m": (n("F#2"), (0, 3, 7)), "Em": (n("E3"), (0, 3, 7)), "F#": (n("F#2"), (0, 4, 7)),
    "Gm": (n("G2"), (0, 3, 7)), "Asus": (n("A2"), (0, 5, 7)),
}
SECTIONS = [
    ("intro", ["D", "Bm", "G", "Asus"]),
    ("A", ["D", "A", "Bm", "F#m", "G", "D", "Em", "A"]),
    ("A2", ["D", "A", "Bm", "F#m", "G", "D", "G", "D"]),
    ("B", ["Bm", "G", "D", "A", "Bm", "G", "Em", "F#"]),
    ("C", ["G", "A", "F#m", "Bm", "G", "A", "D", "D"]),
    ("outro", ["G", "Gm", "D", "Asus"]),
]
# Melodies: one list of (note, beats) per bar; None is a rest.
MELODY = {
    "intro": [[(None, 3)], [(None, 3)], [(None, 3)], [(None, 3)]],
    "A": [
        [("F#5", 2), ("E5", 0.5), ("D5", 0.5)], [("E5", 1.5), ("C#5", 0.5), ("A4", 1)],
        [("B4", 1), ("D5", 1), ("F#5", 1)], [("C#5", 3)],
        [("B4", 1), ("D5", 1), ("G5", 1)], [("F#5", 1.5), ("E5", 0.5), ("D5", 1)],
        [("E5", 1), ("G5", 1), ("B5", 1)], [("A5", 2), ("E5", 1)],
    ],
    "A2": [
        [("F#5", 2), ("E5", 0.5), ("D5", 0.5)], [("E5", 1.5), ("C#5", 0.5), ("A4", 1)],
        [("B4", 1), ("D5", 1), ("F#5", 1)], [("A5", 2), ("C#5", 1)],
        [("B4", 1), ("D5", 1), ("G5", 1)], [("A5", 1.5), ("F#5", 0.5), ("D5", 1)],
        [("G5", 1.5), ("F#5", 0.5), ("E5", 1)], [("D5", 3)],
    ],
    "B": [
        [("B3", 2), ("C#4", 0.5), ("D4", 0.5)], [("E4", 1.5), ("D4", 0.5), ("B3", 1)],
        [("A3", 1), ("D4", 1), ("F#4", 1)], [("E4", 3)],
        [("F#4", 1.5), ("E4", 0.5), ("D4", 1)], [("G4", 1), ("F#4", 1), ("E4", 1)],
        [("E4", 1), ("G4", 1), ("B4", 1)], [("A#4", 1), ("C#5", 1), ("E5", 1)],
    ],
    "C": [
        [("B4", 1), ("D5", 1), ("G5", 1)], [("A5", 2), ("G5", 0.5), ("F#5", 0.5)],
        [("F#5", 1.5), ("E5", 0.5), ("C#5", 1)], [("D5", 3)],
        [("B5", 1.5), ("A5", 0.5), ("G5", 1)], [("A5", 1), ("B5", 1), ("C#6", 1)],
        [("D6", 3)], [("A5", 1.5), ("F#5", 0.5), ("D5", 1)],
    ],
    "outro": [
        [("D5", 1.5), ("B4", 0.5), ("G4", 1)], [("Bb4", 2), ("D5", 1)],
        [("A4", 3)], [("E5", 1.5), ("C#5", 0.5), ("A4", 1)],
    ],
}

# Channels and General MIDI programs.
FLUTE, OBOE, STRINGS, VIOLINS, HORNS, TRUMPETS, HARP, CELLOS, BASSES, TIMPANI, CHOIR, GLOCK = range(12)
DRUMS = 9
PROGRAMS = {
    FLUTE: 73, OBOE: 68, STRINGS: 49, VIOLINS: 48, HORNS: 60, TRUMPETS: 56, HARP: 46, CELLOS: 42, BASSES: 43,
    TIMPANI: 47, CHOIR: 52, GLOCK: 9,
}
CHANNEL = {k: (k if k < DRUMS else k + 1) for k in PROGRAMS}  # skip channel 10 (drums)

rng = random.Random(1998)
events = []  # (tick, order, message)


def note(ch, tick, pitch, beats, vel, legato=0.95):
    t = max(0, int(tick + rng.uniform(-8, 8)))
    length = max(30, int(beats * TPB * legato))
    v = max(1, min(127, int(vel + rng.uniform(-6, 6))))
    c = CHANNEL[ch]
    events.append((t, 1, mido.Message("note_on", channel=c, note=pitch, velocity=v)))
    events.append((t + length, 0, mido.Message("note_off", channel=c, note=pitch, velocity=0)))


def cc(ch, tick, control, value):
    events.append((int(tick), 0, mido.Message("control_change", channel=CHANNEL[ch], control=control,
                                              value=max(0, min(127, int(value))))))


def swell(ch, start, bars, v0, v1, steps=24):
    for i in range(steps + 1):
        cc(ch, start + bars * BAR * i / steps, 11, v0 + (v1 - v0) * i / steps)


def chord_tones(name, low, high):
    root, quality = CHORDS[name]
    tones = []
    for octave in range(-2, 6):
        for q in quality:
            p = root + q + 12 * octave
            if low <= p <= high:
                tones.append(p)
    return sorted(tones)


def melody_line(ch, start, bars, vel, octave=0, legato=0.97):
    t = start
    for bar in bars:
        for name, beats in bar:
            if name is not None:
                note(ch, t, n(name) + 12 * octave, beats, vel, legato)
            t += int(beats * TPB)


def compose():
    for ch, prog in PROGRAMS.items():
        events.append((0, 0, mido.Message("program_change", channel=CHANNEL[ch], program=prog)))
        cc(ch, 0, 7, 100)
        cc(ch, 0, 11, 100)
        cc(ch, 0, 91, 70)  # reverb send
    cc(TIMPANI, 0, 7, 110)
    cc(HARP, 0, 7, 92)
    cc(GLOCK, 0, 7, 70)

    tick = 0
    for section, chords in SECTIONS:
        start = tick
        dyn = {"intro": 0.55, "A": 0.65, "A2": 0.8, "B": 0.8, "C": 1.0, "outro": 0.55}[section]
        for i, name in enumerate(chords):
            t = tick + i * BAR
            root, _ = CHORDS[name]
            pad = chord_tones(name, n("D4") - 2, n("A4") + 1)[:4]
            # Strings: the harmony, held through the bar.
            for p in pad:
                note(STRINGS, t, p, 3, 58 * dyn + 20, 1.02)
            # Basses and cellos on the root.
            note(BASSES, t, root - 12 if root >= n("D3") else root, 3, 70 * dyn + 15, 1.0)
            if section in ("A2", "C", "B"):
                note(CELLOS, t, root, 2, 62 * dyn + 15)
                note(CELLOS, t + 2 * TPB, root + 7, 1, 58 * dyn + 12)
            # Harp: rising and falling arpeggio in eighths.
            if section != "C":
                arp = chord_tones(name, n("D3"), n("D6"))
                pattern = [0, 2, 4, 6, 4, 2] if len(arp) > 6 else [0, 1, 2, 3, 2, 1]
                for k, idx in enumerate(pattern):
                    note(HARP, t + k * TPB // 2, arp[min(idx, len(arp) - 1)], 1.2, 52 + 22 * dyn, 1.0)
            else:
                for k in range(3):
                    arp = chord_tones(name, n("G3"), n("G6"))
                    for j, p in enumerate(arp[k:k + 4]):
                        note(HARP, t + k * TPB + j * TPB // 8, p, 0.8, 70, 1.0)
            # Horns: long notes answering the melody.
            if section in ("A2", "B", "C", "intro"):
                horn = chord_tones(name, n("F3"), n("F4"))
                if section == "intro":
                    if i in (1, 3):
                        note(HORNS, t, horn[-1], 3, 70)
                        note(HORNS, t, horn[-2], 3, 64)
                elif section == "C":
                    for h in horn[-2:]:
                        note(HORNS, t, h, 1.9, 92)
                        note(HORNS, t + 2 * TPB, h, 0.95, 86)
                else:
                    for h in horn[-2:]:
                        note(HORNS, t, h, 3, 66 + 10 * dyn)
            # Timpani on the first beat (and a pickup before the strong bars).
            if section in ("A2", "B", "C"):
                note(TIMPANI, t, root if root < n("A2") + 1 else root - 12, 0.9, 74 + 30 * dyn)
                if section == "C" or i % 4 == 3:
                    for k in range(4):
                        note(TIMPANI, t + 2 * TPB + k * TPB // 4, root + 7 - 12, 0.25, 58 + 10 * k)
            # Choir in the climax.
            if section == "C":
                for p in chord_tones(name, n("A3"), n("E5"))[:4]:
                    note(CHOIR, t, p, 3, 72, 1.02)
            # Glockenspiel sparkles in the quiet parts.
            if section in ("intro", "A", "outro") and i % 2 == 0:
                top = chord_tones(name, n("A5"), n("A6"))
                note(GLOCK, t, top[0], 1, 60)
                note(GLOCK, t + TPB + TPB // 2, top[min(2, len(top) - 1)], 1, 50)

        bars = MELODY[section]
        if section == "A":
            melody_line(FLUTE, start, bars, 92)
        elif section == "A2":
            melody_line(VIOLINS, start, bars, 96, 0, 1.0)
            melody_line(OBOE, start, bars, 70, -1)
        elif section == "B":
            melody_line(CELLOS, start, bars, 98, 0, 1.0)
            melody_line(VIOLINS, start + 4 * BAR, bars[4:], 70, 1, 1.0)
            swell(STRINGS, start + 4 * BAR, 4, 80, 120)
            swell(HORNS, start + 4 * BAR, 4, 80, 120)
        elif section == "C":
            melody_line(TRUMPETS, start, bars, 104)
            melody_line(VIOLINS, start, bars, 108, 0, 1.0)
            melody_line(FLUTE, start, bars, 80, 1)
            cc(STRINGS, start, 11, 120)
            cc(HORNS, start, 11, 115)
            events.append((start, 1, mido.Message("note_on", channel=DRUMS, note=49, velocity=100)))
            events.append((start + BAR * 4, 1, mido.Message("note_on", channel=DRUMS, note=57, velocity=80)))
        elif section == "outro":
            melody_line(FLUTE, start, bars, 80)
            swell(STRINGS, start, 4, 110, 72)
            swell(HORNS, start, 4, 100, 70)
        elif section == "intro":
            swell(STRINGS, start, 4, 60, 100)
        tick += len(chords) * BAR
    # Suspended cymbal swell into the climax.
    c_start = sum(len(c) for s, c in SECTIONS[:4]) * BAR
    for k in range(12):
        events.append((c_start - BAR + k * BAR // 12, 1,
                       mido.Message("note_on", channel=DRUMS, note=51, velocity=20 + k * 6)))
    return tick


def write_midi(path, length):
    mid = mido.MidiFile(ticks_per_beat=TPB)
    track = mido.MidiTrack()
    mid.tracks.append(track)
    track.append(mido.MetaMessage("set_tempo", tempo=mido.bpm2tempo(BPM), time=0))
    track.append(mido.MetaMessage("track_name", name="OOT D.E. Tema do Menu", time=0))
    last = 0
    for t, _, msg in sorted(events, key=lambda e: (e[0], e[1])):
        track.append(msg.copy(time=t - last))
        last = t
    # Room for the reverb after the last bar.
    track.append(mido.MetaMessage("end_of_track", time=max(0, length + 8 * TPB - last)))
    mid.save(path)


def read_wav(path):
    with wave.open(path) as w:
        data = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).reshape(-1, w.getnchannels())
    return data.astype(np.float32)


def main():
    if not os.path.exists(SOUNDFONT):
        sys.exit(f"missing {SOUNDFONT} (apt install fluid-soundfont-gm)")
    length = compose()
    loop = int(round(length / TPB * 60.0 / BPM * RATE))
    with tempfile.TemporaryDirectory() as tmp:
        mid = os.path.join(tmp, "theme.mid")
        wav = os.path.join(tmp, "theme.wav")
        write_midi(mid, length)
        subprocess.run(["fluidsynth", "-ni", "-q", "-g", "0.6", "-r", str(RATE), "-R", "1", "-C", "1",
                        "-F", wav, SOUNDFONT, mid], check=True)
        audio = read_wav(wav)
        # Seamless loop: what rings after the last bar is laid over the start.
        out = audio[:loop].copy()
        tail = audio[loop:loop + loop // 2]
        out[:len(tail)] += tail
        # Normalize to a comfortable level under the game's sound effects.
        peak = np.abs(out).max()
        rms = np.sqrt((out ** 2).mean())
        gain = min(0.89 * 32767 / peak, 0.12 * 32767 / rms)
        out = np.clip(out * gain, -32768, 32767).astype(np.int16)
        loop_wav = os.path.join(tmp, "loop.wav")
        with wave.open(loop_wav, "wb") as w:
            w.setnchannels(2)
            w.setsampwidth(2)
            w.setframerate(RATE)
            w.writeframes(out.tobytes())
        os.makedirs(os.path.dirname(OUT), exist_ok=True)
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", loop_wav, "-c:a", "libvorbis", "-q:a", "5",
                        "-metadata", "title=OOT D.E. - Tema do Menu", "-metadata", "artist=OOT D.E.", OUT],
                       check=True)
    print("wrote", os.path.relpath(OUT, ROOT), f"{loop / RATE:.1f} s", os.path.getsize(OUT) // 1024, "KB")


if __name__ == "__main__":
    main()
