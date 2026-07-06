# Task 01: Startup SPIFFS And Audio Init

## Goal
Mount SPIFFS and initialize shared audio resources once during startup so later recording and playback tasks have a stable foundation.

## Files Expected To Change Later
- `main/main.c`
- `main/CMakeLists.txt`, only if a future build requires an explicit dependency for SPIFFS APIs

## Implementation Notes
- Call `bsp_spiffs_mount()` during `app_main()` startup before any recording or playback file access.
- Move codec initialization ownership out of the capture task and initialize once at startup with `bsp_extra_codec_init()`.
- Initialize the audio player once at startup with `bsp_extra_player_init()`.
- Register the player callback with `bsp_extra_player_register_callback()` so later tasks can react to idle and playback events.
- Log startup failures clearly, including SPIFFS mount, codec init, player init, and callback setup context.
- If startup audio initialization fails, keep the UI error state explicit rather than failing silently.

## Acceptance Criteria
- Boot still starts the existing display and live spectrum path when initialization succeeds.
- Startup logs clearly report SPIFFS and audio initialization failures.
- Codec initialization is not repeated inside the mic capture task.
- Audio player initialization happens once.

## Dependencies On Prior Tasks
- None.
