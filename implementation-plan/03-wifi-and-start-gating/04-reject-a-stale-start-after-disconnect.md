# Reject a stale Start after disconnect

Status: Planned
Requirements: E3
Depends on: 03/03

## Outcome
A Start action accepted by the UI just before disconnect is revalidated at command execution and cannot open a recording after connectivity has become false.

## Current gap
Disabling the button on state refresh does not close the race between touch dispatch, the application command queue, and a Wi-Fi disconnect.

## In scope
- Recheck authoritative Wi-Fi connectivity in the command path immediately before accepting Start and creating the run ID or WAV.
- Return a non-recording unavailable result to the UI without disturbing storage or the reconnect state.
- Exercise the race deterministically by holding the Start command until after the disconnect event, then releasing it.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Adding a general command framework, changing touch behavior, testing active recording loss, or altering Wi-Fi reconnection.

## Likely files
- main/main.c
- main/wifi_manager.c
- main/wifi_manager.h
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- `idf.py build` passes.
- The deterministically stale Start is rejected after disconnect and creates no run ID, active WAV, metadata, reservation, or Recording transition.
- A fresh Start succeeds only after authoritative Connected state returns and inventory is empty.
- The test mechanism is disabled in the handed-off build and its evidence is labeled Supplemental / Verification build if runtime fault control was required.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
