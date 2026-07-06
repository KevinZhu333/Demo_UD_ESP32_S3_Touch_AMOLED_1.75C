# Task 03: WAV File Writer

## Goal
Add WAV writer helpers in `main/main.c` to create, update, and finalize the single overwriteable recording file.

## Files Expected To Change Later
- `main/main.c`

## Implementation Notes
- Use the fixed path `/spiffs/recording.wav`.
- Open the file for overwrite when recording starts.
- Write a placeholder 44-byte PCM WAV header before writing audio data.
- Use this WAV format:
  - `16000 Hz`
  - `16-bit`
  - `2 channels`
  - PCM format `1`
- Match the current codec defaults and existing raw stereo capture buffer used by the analyzer.
- Track audio data bytes with a `uint32_t`-compatible size.
- On stop, seek back and finalize:
  - `RIFF` chunk size
  - `WAVE` identifier
  - `fmt ` chunk
  - `data` chunk size
- Flush and close the file after finalization.
- Treat partial or failed header writes as recording start failures.

## Acceptance Criteria
- Saved file begins with valid `RIFF`, `WAVE`, `fmt `, and `data` chunks.
- Header sizes match the amount of PCM data written.
- Empty or failed recordings are handled intentionally and do not leave an open file handle.

## Dependencies On Prior Tasks
- Depends on Task 01 for SPIFFS mounting.
- Can be implemented before the LVGL controls if tested through internal calls, but UI integration depends on Task 02.
