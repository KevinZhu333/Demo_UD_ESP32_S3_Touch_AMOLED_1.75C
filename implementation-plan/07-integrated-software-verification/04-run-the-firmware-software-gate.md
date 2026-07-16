# Run the firmware software gate

Status: Planned
Requirements: A4, B5, B6, B7, D4, D5, D6, E1, E2, E3, E4, E5, E7, E9, F1, F2, F3, F4, F5, F6, F7, F8, G3, G4, G5, G6, G7, G8, G9, H2, H3, H4, H6, I1, I3, I4, I5, J5
Depends on: 07/03

## Outcome
The flag-off ESP32 firmware candidate builds successfully in the intended ESP-IDF environment, and its image and secret-free runtime-configuration fingerprints are recorded for all later evidence.

## Current gap
Incremental capture, identity, Wi-Fi, UI, lifecycle, and storage changes have not yet been compiled together as the immutable candidate used by phases 08 and 09.

## In scope
- Disable every cleanup-fault, fixture-repair, and other verification option, then run `idf.py build` in the activated ESP-IDF environment.
- Hash the resulting firmware image and derive a runtime-configuration fingerprint that excludes secret values while detecting behavior-affecting configuration changes.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Flashing a board, inferring physical continuity from compilation, or changing code/configuration to make a failing build pass within this verification task.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/wifi_manager.c
- docs/design.md

## Acceptance
- `idf.py build` passes with verification options disabled, and the output image SHA-256 plus secret-free configuration fingerprint are retained.
- The build handoff distinguishes compiled behavior from all still-unrun board checks.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
