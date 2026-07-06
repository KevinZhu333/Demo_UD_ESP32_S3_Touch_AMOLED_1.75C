# Add On-Device WAV Recording and Playback

## Goal
Segment the v1 on-device WAV recording and playback feature into small implementation tasks. Each task should be safe for a future Codex run to implement independently while preserving the existing ESP32-S3 live spectrum analyzer.

## Files Expected To Change Later
- `main/main.c`
- `main/CMakeLists.txt`, only if direct SPIFFS APIs require an explicit dependency
- No BSP or managed component changes are expected unless a future build exposes a missing dependency

## Implementation Notes
- Add a local-only recording flow: tap `Record` to overwrite `/spiffs/recording.wav`, tap `Stop` to finalize the WAV file, and tap `Play` to route that file through the existing speaker playback path.
- Keep storage predictable by using one overwriteable WAV file on the SPIFFS partition.
- Preserve the live spectrum canvas outside playback.
- Use the existing single microphone capture path; do not add another I2S or codec reader.

## Current Repo Facts
- Main app is in `main/main.c`.
- Current analyzer uses `bsp_extra_i2s_read()`.
- Codec helpers are in `components/bsp_extra`.
- SPIFFS partition is `7M` from `partitions.csv`.
- Existing playback path is `bsp_extra_player_play_file()`.
- Build command is `idf.py -B build-codex2 build`.

## Ordered Task List
1. `01_startup_spiffs_audio_init.md` - mount SPIFFS and initialize codec/player startup ownership.
2. `02_lvgl_recording_controls.md` - add LVGL controls and status text around the existing canvas.
3. `03_wav_file_writer.md` - write and finalize the single WAV file.
4. `04_capture_record_state_machine.md` - integrate recording into the existing mic capture task.
5. `05_playback_integration.md` - play the finalized WAV through the existing player.
6. `06_storage_limits_and_errors.md` - add capacity checks and robust file error handling.
7. `07_build_and_manual_qa.md` - build and manually verify the complete flow.
8. `08_screen_off_recording.md` - turn display/backlight off during recording while capture continues.
9. `09_global_display_auto_off.md` - make display/backlight auto-off global after any interaction.

## Acceptance Criteria
- The feature is divided into small, ordered markdown tasks.
- Each task names its goal, later source files, implementation notes, acceptance criteria, and dependencies.
- No firmware source files are changed by this planning step.

## Assumptions
- Use a single overwriteable `/spiffs/recording.wav` file for v1.
- Playback pauses the live analyzer.
- Recording is local-only; no transcription or network upload is added.
- The existing target remains `esp32s3` on ESP-IDF 6.0.2.

## Dependencies On Prior Tasks
- None. This file is the timeline index.
