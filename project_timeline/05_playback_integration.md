# Task 05: Playback Integration

## Goal
Implement `Play` behavior by sending the finalized WAV file through the existing audio player path and returning to live visualization when playback ends.

## Files Expected To Change Later
- `main/main.c`

## Implementation Notes
- Use `bsp_extra_player_play_file("/spiffs/recording.wav")` for playback.
- Before playing, check that a finalized recording exists.
- If no finalized recording exists, show `No recording` and leave capture idle.
- Set state to `playing` before starting playback so capture pauses immediately.
- Use the audio player callback registered during startup:
  - `AUDIO_PLAYER_CALLBACK_EVENT_IDLE` returns the app to idle/saved visualization state.
  - `AUDIO_PLAYER_CALLBACK_EVENT_PLAYING` may confirm the `Playing` status.
  - `AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE` moves to `No recording` or `Error`, depending on file existence.
  - Unknown playback errors move to `Error`.
- Ensure live analyzer resumes after playback becomes idle.

## Acceptance Criteria
- Pressing `Play` pauses live capture and visualization.
- Saved audio plays through the speaker using the existing player path.
- Playback completion returns to live visualization.
- Missing or invalid recordings do not crash the app.

## Dependencies On Prior Tasks
- Depends on Task 01 for audio player initialization and callback registration.
- Depends on Task 02 for the `Play` button and status label.
- Depends on Task 03 for finalized WAV output.
- Depends on Task 04 for the `playing` state and capture pause behavior.
