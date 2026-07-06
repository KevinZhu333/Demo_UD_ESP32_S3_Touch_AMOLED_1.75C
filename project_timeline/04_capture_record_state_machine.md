# Task 04: Capture Record State Machine

## Goal
Extend the existing single mic-reading task into a capture and recorder state machine without adding another codec or I2S reader.

## Files Expected To Change Later
- `main/main.c`

## Implementation Notes
- Use these app states:
  - `idle`
  - `recording`
  - `playing`
  - `error`
- Keep one task responsible for calling `bsp_extra_i2s_read()`.
- During `recording`, write the same raw 16-bit stereo PCM frames already read from `bsp_extra_i2s_read()`.
- During `idle`, continue updating the live FFT visualization.
- During `playing`, pause mic reads and visualization updates to avoid codec contention and feedback.
- Protect shared state between LVGL callbacks, the capture task, and audio player callback with FreeRTOS-safe synchronization.
- Keep state transitions explicit:
  - record request opens the WAV writer and enters `recording`
  - stop request finalizes the WAV writer and returns to saved/idle behavior
  - play request enters `playing`
  - unrecoverable errors enter `error`

## Acceptance Criteria
- Recording does not break the live spectrum display.
- Playback does not race with mic capture.
- No second I2S or microphone reader is introduced.
- Shared state is not read or written unsafely across callbacks and tasks.

## Dependencies On Prior Tasks
- Depends on Task 01 for single startup audio ownership.
- Depends on Task 03 for WAV writer start/write/finalize helpers.
- Integrates with Task 02 button callbacks and status updates.
