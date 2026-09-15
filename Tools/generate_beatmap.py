"""
Generates a beatmap CSV you can import into Unreal as a DataTable (row type: RhythmNoteRow).

Two ways to get the beat grid:
  1. From a BPM (no extra installs):
       python Tools/generate_beatmap.py --bpm 120 --length 90 --out Beatmaps/Test_120bpm.csv
  2. Detected from your song (needs: pip install librosa):
       python Tools/generate_beatmap.py --audio path/to/song.wav --out Beatmaps/MySong.csv

The song is split into three acts that introduce the verbs one by one:
  Act 1  slices only
  Act 2  + shooting and dodge walls
  Act 3  + notes coming from the sides (turn around in your room)
Choice notes (corrupted creatures: purify or destroy) are placed at --choices, with a gap
around each one so the player has time to decide.

This is a starting point, not a finished chart — import it, play it, then hand-tune rows in the
DataTable editor (or edit the CSV and reimport).
"""

import argparse
import csv
import random
import sys
from pathlib import Path

HEADER = ["Name", "Time", "Type", "Hand", "Direction", "LaneX", "LaneY", "ApproachYaw"]

# Lane positions in cm, relative to the conductor's hit point.
HAND_LANE_X = {"Left": -25.0, "Right": 25.0}
SLICE_HEIGHTS = [-15.0, 0.0, 15.0]
SHOOT_LANES_X = [-60.0, -30.0, 30.0, 60.0]
# Wall centres (hit point is 30cm below the head, walls are 80cm tall by default).
DUCK_WALL = (0.0, 60.0)       # covers the head from just below it upwards -> duck
LEAN_WALLS = [(-30.0, 30.0), (30.0, 30.0)]  # covers one side of the head -> lean away

DIFFICULTY_BEAT_STEP = {"easy": 2, "normal": 1, "hard": 1}


def beat_grid_from_bpm(bpm, length, offset):
    step = 60.0 / bpm
    beats = []
    t = offset
    while t < length:
        beats.append(round(t, 4))
        t += step
    return beats, bpm


def beat_grid_from_audio(path):
    try:
        import librosa
    except ImportError:
        sys.exit("librosa isn't installed. Run: pip install librosa  (or use --bpm instead)")

    y, sr = librosa.load(path, mono=True)
    tempo, beats = librosa.beat.beat_track(y=y, sr=sr, units="time")
    bpm = float(tempo[0]) if hasattr(tempo, "__len__") else float(tempo)
    length = librosa.get_duration(y=y, sr=sr)
    return [round(float(b), 4) for b in beats], bpm, length


def build_notes(beats, length, choices, difficulty, seed):
    rng = random.Random(seed)
    step = DIFFICULTY_BEAT_STEP[difficulty]
    act1_end = length / 3.0
    act2_end = length * 2.0 / 3.0
    outro_start = length - 4.0

    notes = []
    hand_toggle = 0
    for index, time in enumerate(beats):
        if index < 4 or time >= outro_start:
            continue  # breathing room at the start and end
        if any(abs(time - c) < 1.5 for c in choices):
            continue  # leave space around choice moments
        if index % step:
            continue

        act = 1 if time < act1_end else 2 if time < act2_end else 3
        bar_beat = index % 16

        if act >= 2 and bar_beat == 0 and index % 32 == 0:
            x, y = DUCK_WALL if rng.random() < 0.5 else rng.choice(LEAN_WALLS)
            notes.append(dict(Time=time, Type="Dodge", Hand="Any", Direction="Any", LaneX=x, LaneY=y, ApproachYaw=0.0))
            continue

        if act >= 2 and index % 4 == 2 and rng.random() < (0.5 if act == 2 else 0.7):
            yaw = 0.0
            if act == 3 and rng.random() < 0.35:
                yaw = rng.choice([-90.0, 90.0] + ([180.0] if difficulty == "hard" else []))
            notes.append(dict(Time=time, Type="Shoot", Hand="Any", Direction="Any",
                              LaneX=rng.choice(SHOOT_LANES_X), LaneY=30.0, ApproachYaw=yaw))
            continue

        hand = "Left" if hand_toggle % 2 == 0 else "Right"
        direction = "Down" if (hand_toggle // 2) % 2 == 0 else "Up"
        hand_toggle += 1
        yaw = 0.0
        if act == 3 and rng.random() < 0.2:
            yaw = rng.choice([-60.0, 60.0])
        notes.append(dict(Time=time, Type="Slice", Hand=hand, Direction=direction,
                          LaneX=HAND_LANE_X[hand], LaneY=rng.choice(SLICE_HEIGHTS), ApproachYaw=yaw))

    for c in choices:
        notes.append(dict(Time=round(c, 4), Type="Choice", Hand="Any", Direction="Any", LaneX=0.0, LaneY=15.0, ApproachYaw=0.0))

    notes.sort(key=lambda n: n["Time"])
    return notes


def snap(time, beats):
    return min(beats, key=lambda b: abs(b - time)) if beats else time


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--bpm", type=float, help="Song tempo in beats per minute")
    source.add_argument("--audio", type=str, help="Audio file to detect beats from (needs librosa)")
    parser.add_argument("--length", type=float, default=90.0, help="Song length in seconds (BPM mode)")
    parser.add_argument("--offset", type=float, default=0.0, help="Time of the first beat in seconds (BPM mode)")
    parser.add_argument("--choices", type=str, default="", help="Comma separated times for choice notes (default: 25%%, 50%%, 75%% of the song)")
    parser.add_argument("--difficulty", choices=DIFFICULTY_BEAT_STEP.keys(), default="normal")
    parser.add_argument("--seed", type=int, default=7)
    parser.add_argument("--out", type=str, required=True)
    args = parser.parse_args()

    if args.audio:
        beats, bpm, length = beat_grid_from_audio(args.audio)
    else:
        beats, bpm = beat_grid_from_bpm(args.bpm, args.length, args.offset)
        length = args.length

    if args.choices:
        choices = [float(c) for c in args.choices.split(",") if c.strip()]
    else:
        choices = [length * 0.25, length * 0.5, length * 0.75]
    choices = [snap(c, beats) for c in choices]

    notes = build_notes(beats, length, choices, args.difficulty, args.seed)

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(HEADER)
        for i, n in enumerate(notes):
            writer.writerow([f"N{i:04d}", n["Time"], n["Type"], n["Hand"], n["Direction"], n["LaneX"], n["LaneY"], n["ApproachYaw"]])

    counts = {}
    for n in notes:
        counts[n["Type"]] = counts.get(n["Type"], 0) + 1
    print(f"Wrote {len(notes)} notes to {out}")
    print("  " + ", ".join(f"{k}: {v}" for k, v in sorted(counts.items())))
    print(f"  Set BPM on the RhythmConductor to {bpm:.2f}")


if __name__ == "__main__":
    main()
