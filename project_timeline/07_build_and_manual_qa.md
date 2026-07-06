# Task 07: Build And Manual QA

## Goal
Verify the completed recording and playback feature through an ESP-IDF build and manual hardware QA.

## Files Expected To Change Later
- `project_timeline/07_build_and_manual_qa.md`, to record checklist results after testing
- Firmware source files only if QA exposes issues that need fixes

## Implementation Notes
- Build with:
  ```bash
  idf.py -B build-codex2 build
  ```
- Flash and monitor manually after a successful build.
- Record manual QA results in this markdown file after testing.
- Capture any failures with enough detail for a follow-up implementation task.

## Manual QA Checklist
- [ ] Boot shows spectrum and controls. Not run: hardware flash/monitor not available in this session.
- [ ] Pressing `Record` changes status and creates `/spiffs/recording.wav`. Not run: hardware flash/monitor not available in this session.
- [ ] Pressing `Stop` finalizes the WAV file. Not run: hardware flash/monitor not available in this session.
- [ ] Pressing `Play` outputs through the speaker. Not run: hardware flash/monitor not available in this session.
- [ ] Playback returns to live visualization. Not run: hardware flash/monitor not available in this session.
- [ ] Starting a second recording overwrites the first. Not run: hardware flash/monitor not available in this session.
- [ ] Recording until storage limit shows `Storage full` and is graceful. Not run: hardware flash/monitor not available in this session.
- [ ] Storage-full testing does not crash or leave a corrupt open file. Not run: hardware flash/monitor not available in this session.

## Build Results
- ESP-IDF Python environment repaired with `/Users/starspulse/.espressif/v6.0.2/esp-idf/install.sh`.
- `idf.py -B build-codex2 build`: passed after sourcing `/Users/starspulse/.espressif/v6.0.2/esp-idf/export.sh`.
- Fallback verification passed: `env IDF_PATH=/Users/starspulse/.espressif/v6.0.2/esp-idf cmake --build build`.
- Build output generated `/Users/starspulse/myProject3/build-codex2/myProject3.bin`; app size was `0xc6010` bytes with `0x739ff0` bytes free in the smallest app partition.
- Remaining non-fatal build notes include unknown legacy `ESP_BROOKESIA_*` defaults and LVGL demo source/component validation warnings.

## Acceptance Criteria
- `idf.py -B build-codex2 build` passes.
- Manual QA checklist results are recorded in this file after testing.
- Any failing checklist item has a clear note with observed behavior.

## Dependencies On Prior Tasks
- Depends on Tasks 01 through 06 being implemented.
