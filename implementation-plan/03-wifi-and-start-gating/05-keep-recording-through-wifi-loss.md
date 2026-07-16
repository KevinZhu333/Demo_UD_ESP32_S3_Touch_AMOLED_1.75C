# Keep recording through Wi-Fi loss

Status: Planned
Requirements: E4, G3, G4
Depends on: 03/04

## Outcome
Loss of Wi-Fi after Start changes the awake presentation to `Wi-Fi lost · recording locally` while microphone capture and the current session continue uninterrupted.

## Current gap
`design.md` implements reconnect and local retention but has no UI transition proving that an active disconnect preserves Recording.

## In scope
- Layer the Offline/recording-local presentation on the authoritative active-session and Wi-Fi states without issuing Stop or reopening the recorder.
- Run a board session through a controlled disconnect and reconnect while monitoring run ID, admitted PCM, segment indexes, capture failures, and local retention.
- Confirm the screen remains visibly Recording whenever awake and returns to the connected recording presentation after reconnection.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- The 60-second K8 outage, final catch-up acceptance, upload-count UI, Upload error presentation, or Stop lifecycle states.

## Likely files
- main/main.c
- main/wifi_manager.c
- main/wifi_manager.h
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- `idf.py build` passes.
- Disconnect produces `Wi-Fi lost · recording locally` without a stopped event, run-ID change, index reset, capture-resource failure, local gap, or storage overflow.
- At least one segment finalizes locally while disconnected and remains eligible for ordered upload only after reconnection.
- Reconnection restores the connected Recording presentation without an extra Start action; evidence remains developmental and does not claim K8.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
