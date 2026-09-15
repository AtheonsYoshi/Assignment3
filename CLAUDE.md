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
- Unreal MCP server: http://localhost:8000/mcp (only while the editor is open). Exposes Blueprint, DataTable,
  Material, Actor/Scene, UMG, Config Settings, Logs and Live Coding toolsets.
- While the editor is open, change project settings through the ConfigSettingsToolset rather than editing
  `Config/*.ini` by hand, or the editor may overwrite the file.

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

## Content layout
- `Content/Resonance/Levels/L_Resonance` — game level (editor startup + game default map), duplicated from the template level.
- `Content/Resonance/Core/BP_ResonanceDirector` — game flow. Currently a test flow: 3s delay → StartSong, no-fail mode on,
  prints ending + score when the song finishes. Onboarding/offboarding will grow from here.
- `Content/Resonance/Notes/BP_Note_{Slice,Shoot,Dodge,Choice}` — note Blueprints (Slice colours by hand + rotates its arrow in OnNoteActivated).
- `Content/Resonance/Materials/M_Neon` + `MI_Neon_*` — unlit opaque emissive with fresnel edge (Color, Intensity params).
- `Content/Resonance/Beatmaps/DT_*` — DataTables imported from `Beatmaps/*.csv`.
- `BP_XRPawn` (template pawn): `BladeLeft/Right` (+ `BladeVisual*`) and `PalmLeft/Right` on the grip controllers.
  Grip `IA_Grab_*_Pressed.Started` opens the palm, `IA_Grab_*_Released.Completed` closes it (grab logic untouched);
  `IA_Shoot_Left/Right` call the `RhythmShoot(bRightHand)` function (sphere trace from the aim pose → TryShoot).

## Unreal MCP gotchas
- Visual-only mesh components need `bodyInstance.collisionProfileName = "NoCollision"`. Setting only `collisionEnabled`
  gets overwritten by the profile on load (this blocked the pawn from spawning).
- `read_graph_dsl` output is lossy (it omits existing wiring). Never rewrite an existing graph from it — add nodes with
  `create_node` / `connect_pins` instead. `write_graph_dsl` appends to a graph.
- DSL events use the node type id without `AddEvent|` (e.g. `Rhythm|Note|EventOnNoteActivated`); `fn` bodies must be
  written into a graph created with `add_function_graph`.
- PIE runs at ~3 fps while the editor window isn't focused. For desktop PIE, pass a `startTransform` at head height (z≈170).

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
Student: Matthew Williamson. Project name: Resonance.
- APK file: `MDDN222_P3_Williamson_Matthew.apk` (rename the packaged APK to this)
- Android package (set): `nz.ac.wgtn.mddn222.project3.Williamson_Matthew`
- Display name (set): `MDDN222 Matthew Williamson Resonance`
- Also: 30s video, artist's statement PDF, AI-usage document (Claude Code used for C++ and tooling).
