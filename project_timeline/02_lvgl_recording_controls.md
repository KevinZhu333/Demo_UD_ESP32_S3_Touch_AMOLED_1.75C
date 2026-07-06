# Task 02: LVGL Recording Controls

## Goal
Keep the live spectrum canvas and add a compact recording/playback control strip with `Record`, `Stop`, `Play`, and a status label.

## Files Expected To Change Later
- `main/main.c`

## Implementation Notes
- Preserve the existing LVGL canvas and timer-driven spectrum drawing.
- Add `Record`, `Stop`, and `Play` buttons using LVGL button objects.
- Add a compact status label near the controls.
- Use LVGL event callbacks for button actions.
- `Record` overwrites `/spiffs/recording.wav`.
- `Stop` finalizes the WAV header and enables playback when finalization succeeds.
- `Play` pauses capture/visualization and starts the existing playback path.
- Required status strings include:
  - `Ready`
  - `Recording 00:12`
  - `Saved 00:12`
  - `Playing`
  - `Storage full`
  - `No recording`
  - `Error`
- Button enablement rules:
  - `Record` is enabled when idle, saved, or error.
  - `Stop` is enabled only while recording.
  - `Play` is enabled only when a finalized recording exists and the app is not recording.
- Keep the layout usable on the existing display size and avoid covering the spectrum canvas.

## Acceptance Criteria
- UI shows the spectrum canvas plus the three controls and status label.
- Button states reflect the current app state.
- Canvas continues updating outside playback.
- Status text is short enough for the existing display.

## Dependencies On Prior Tasks
- Depends on Task 01 for startup-level state and audio initialization hooks.
