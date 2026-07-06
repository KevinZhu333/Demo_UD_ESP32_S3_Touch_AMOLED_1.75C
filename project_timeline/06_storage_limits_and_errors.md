# Task 06: Storage Limits And Errors

## Goal
Prevent recordings from exhausting SPIFFS and handle file I/O failures without crashing or leaving corrupt open handles.

## Files Expected To Change Later
- `main/main.c`
- `main/CMakeLists.txt`, only if direct `esp_spiffs_info()` usage requires an explicit `spiffs` dependency

## Implementation Notes
- Query SPIFFS capacity before recording starts.
- Query or estimate remaining writable space during recording.
- Use `esp_spiffs_info()` if direct free-space checks are needed.
- Stop recording before file growth can exhaust free space.
- Reserve enough space to seek, finalize, flush, and close the WAV header safely.
- On storage limit:
  - stop accepting more PCM data
  - finalize the bytes already recorded
  - show `Storage full`
- Handle failures from:
  - `fopen`
  - `fwrite`
  - `fseek`
  - `fflush`
  - `fclose`
- Clear stale recording state when a failed write means the file should not be considered playable.

## Acceptance Criteria
- Full storage does not crash the app.
- Full storage does not leave an open recording handle.
- A storage-limited recording is finalized if enough data was safely written.
- File I/O errors produce `Error`, `Storage full`, or `No recording` as appropriate.

## Dependencies On Prior Tasks
- Depends on Task 01 for mounted SPIFFS.
- Depends on Task 03 for WAV writer lifecycle.
- Depends on Task 04 for state transitions and synchronization.
