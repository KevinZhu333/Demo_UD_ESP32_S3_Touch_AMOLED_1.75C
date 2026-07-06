# Task 08: Screen-Off Recording Mode

## Goal
Allow recording to continue while the display/backlight is off, then wake the UI on touch so the user can stop and save the recording.

## Files Expected To Change Later
- `main/main.c`
- No BSP or managed component changes are expected.

## Implementation Notes
- Add local display-sleep state in `main/main.c`:
  - `recording_display_sleep_enabled`, default `true`
  - `recording_display_off`, default `false`
  - `recording_display_off_delay_ms`, default `10000`
  - `last_touch_or_ui_activity_tick`
- On `Record`:
  - start recording exactly as today
  - keep the UI visible initially with `Recording 00:00`
  - arm a timer or state check that turns the display/backlight off after 10 seconds if still recording
- While recording:
  - keep `audio_fft_task()` reading from the mic and writing PCM to the WAV writer
  - skip or minimize canvas redraw while the display is off
  - do not enter deep sleep and do not pause FreeRTOS tasks
- Display-off behavior:
  - use `bsp_display_backlight_off()` as the first implementation
  - keep LVGL and touch input alive
  - set `recording_display_off = true`
- Wake behavior:
  - use LVGL touch/input activity to wake the display
  - on touch while display is off, call `bsp_display_backlight_on()`
  - set `recording_display_off = false`
  - refresh controls and status without stopping recording
  - keep `Stop` enabled after wake
- On `Stop`, recording error, playback, or startup failure:
  - ensure the display/backlight is on before showing final status or errors
  - clear screen-off recording state

## Acceptance Criteria
- Pressing `Record` starts normal recording and shows `Recording 00:00`.
- After 10 seconds of continuous recording, the display/backlight turns off.
- Audio capture and `/spiffs/recording.wav` writes continue while the display is off.
- Touch wakes the display/backlight without stopping recording.
- After wake, `Stop` is visible and enabled.
- Pressing `Stop` after wake finalizes the WAV and shows `Saved mm:ss`.
- Playback behavior remains unchanged.
- Startup failure still shows the startup error label and does not create controls.

## Test Plan
- Build with:
  ```bash
  idf.py -B build-codex2 build
  ```
- Manual QA:
  - record for more than 10 seconds and confirm the screen turns off
  - wake with touch and confirm status still shows recording
  - stop after wake and confirm `Saved mm:ss`
  - play the saved recording and confirm it includes audio captured while the display was off
  - confirm a second recording overwrites the first
  - confirm storage-full and WAV error paths turn the display back on before showing `Storage full` or `Error`

## Assumptions
- Default screen-off delay is 10 seconds.
- Wake source is touch/LVGL input only; physical button support is deferred unless a real board button API is added.
- "Display off" means backlight/brightness off, not ESP32 deep sleep.
- Recording must continue using the existing single capture task and existing WAV writer.

## Dependencies On Prior Tasks
- Depends on Task 02 for LVGL controls and status updates.
- Depends on Task 04 for recording state and capture-task integration.
- Depends on Task 06 for storage error handling that must wake the display before showing errors.
