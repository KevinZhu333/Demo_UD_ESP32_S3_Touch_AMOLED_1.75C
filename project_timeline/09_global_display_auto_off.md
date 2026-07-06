# Task 09: Global Display Auto-Off

## Goal
Make display/backlight auto-off global across the app: after any touch or button interaction, the display stays awake briefly, then turns off after 10 seconds or less of inactivity regardless of whether the app is ready, recording, saved, playing, or showing an error.

## Files Expected To Change Later
- `main/main.c`
- No BSP or managed component changes are expected.

## Implementation Notes
- Generalize the existing Task 08 display-sleep state:
  - rename `recording_display_*` helpers/state to `display_auto_off_*`
  - keep the fixed timeout at 10 seconds or less
  - keep `wake_layer` as the full-screen hidden touch catcher while the display is off
- Auto-off behavior:
  - remove the condition that only turns display/backlight off while `APP_UI_STATE_RECORDING`
  - allow auto-off in `READY`, `RECORDING`, `SAVED`, `PLAYING`, and `ERROR`
  - continue skipping canvas redraw while display/backlight is off
  - do not enter deep sleep, deinitialize the panel, stop LVGL, or pause FreeRTOS tasks
- Interaction behavior:
  - any touch or button press marks user activity and restarts the inactivity timer
  - if display/backlight is off, the first touch only wakes the display through `wake_layer`
  - the first wake touch must not activate an underlying button such as `Record`, `Stop`, or `Play`
- Status/error behavior:
  - startup failure must force display/backlight on before showing the startup error label
  - recording, storage, and playback errors may wake the display before showing `Error`, `Storage full`, or `No recording`
  - after showing status/error text, display/backlight may auto-off again after inactivity
- Audio behavior:
  - keep `audio_fft_task()` capture and WAV writing unchanged while display/backlight is off
  - keep existing playback behavior and codec ownership unchanged

## Acceptance Criteria
- Booting and doing nothing turns display/backlight off after about 10 seconds.
- Touching the dark screen wakes the display without pressing any underlying control.
- Touching the screen without pressing a button wakes the display, then it turns off again after about 10 seconds.
- Pressing `Record` starts recording, then display/backlight turns off after about 10 seconds of inactivity.
- Recording continues while display/backlight is off.
- Touching to wake while recording shows the UI with `Stop` enabled.
- Pressing `Stop` after wake finalizes the WAV normally.
- After `Saved`, display/backlight turns off again after about 10 seconds of inactivity.
- During `PLAYING`, display/backlight also follows the same inactivity auto-off behavior.
- Startup and runtime error labels remain visible immediately when first shown, then follow normal auto-off timing.

## Test Plan
- Build with:
  ```bash
  idf.py -B build-codex2 build
  ```
- Manual QA:
  - boot and wait for auto-off
  - touch dark screen and verify it wakes without activating controls
  - wait again and verify it turns off
  - record for longer than 10 seconds and verify recording continues while dark
  - wake, stop, and play the recording
  - verify saved/play/error states all auto-off after inactivity

## Assumptions
- Use the existing 10-second timeout from Task 08.
- "Display off" means `bsp_display_backlight_off()` / brightness `0`, not deep sleep.
- Global auto-off applies during playback too.
- Wake source remains LVGL touch input only.

## Dependencies On Prior Tasks
- Depends on Task 08 for the initial display-off state, wake layer, and recording-safe screen-off behavior.
