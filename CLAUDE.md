# Assignment3 — MR rhythm-combat game (MDDN222 A3, Quest 3)

Mixed-reality rhythm game for the "Agency & Embodiment in XR" assignment (due 21 Oct 2026).
The player is the Warden of their own room: slice, shoot, dodge and purify/destroy creatures
coming through rifts on the beat. Choices (purify vs destroy) and performance decide one of
three endings: Harmony, Silence, Overrun.

## Engine / platform
- Unreal Engine 5.8 (launcher build), started from the VR template (`Content/XRFramework`).
- Target: standalone Quest 3 via OpenXR, mobile forward renderer, passthrough MR.
  No Lumen, Nanite, ray tracing or post-process on device. Must hold 90fps (75 minimum).
- MCP plugin and toolsets are editor-only (`TargetAllowList: Editor`) — keep them out of the APK.

## Code layout
- `Source/Assignment3/Rhythm/` — gameplay framework (C++, exposed to Blueprint):
  - `RhythmTypes.h` — enums + `FRhythmNoteRow` (beatmap DataTable row).
  - `RhythmConductor` — song clock, beatmap scheduling, note pooling, `OnSongFinished` / `OnNoteResolved`.
  - `RhythmNote` — base note actor; position derived from song time; hit windows, grading, dodge checks.
  - `RhythmBladeComponent` / `RhythmPalmComponent` — slice and purify interactions on motion controllers.
  - `RhythmScoreSubsystem` — score, combo, health, choice counts, `EvaluateEnding()`.
- Blueprints own presentation (meshes, VFX, audio, UI, onboarding/offboarding flow); C++ owns rules and timing.
- `Tools/generate_beatmap.py` — builds beatmap CSVs (BPM grid, or librosa beat detection).
- `Beatmaps/*.csv` — source CSVs; import into Content as DataTables with row type `RhythmNoteRow`.

## Build
Editor must be closed for a full build if the module is loaded (otherwise use Live Coding, Ctrl+Alt+F11):
```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" Assignment3Editor Win64 Development "-Project=C:\Users\yoshi\Documents\Unreal Projects\Assignment3\Assignment3.uproject" -WaitMutex
```

## Conventions
- Blueprint-facing C++: `Rhythm|...` categories, tooltips on every property, no raw spawning of notes (use the pool).
- Never spawn/destroy actors per note mid-song; pool and toggle visibility.
- Includes are module-relative: `#include "Rhythm/RhythmNote.h"`.

## Submission requirements
- APK: `MDDN222_P3_LASTNAME_FIRSTNAME.apk`
- Android package: `nz.ac.wgtn.mddn222.project3.LASTNAME_FIRSTNAME`
- Display name: `MDDN222 FIRSTNAME LASTNAME PROJECTNAME`
- Also: 30s video, artist's statement PDF, AI-usage document (Claude Code used for C++ and tooling).
