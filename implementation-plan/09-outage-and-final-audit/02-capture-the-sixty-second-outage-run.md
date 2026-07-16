# Capture the sixty-second outage run

Status: Planned
Requirements: E4, F7, J4, K8
Depends on: 09/01

## Outcome
One separate board session records continuously through a measured 60-second Wi-Fi disconnection and remains active until all same-run backlog drains after reconnection.

## Current gap
No physical outage/catch-up run exists, and starting from clean idle state alone does not prove the required active-recording `pending_count == 0` condition immediately before disconnect.

## In scope
- Start one new recording, wait while capture is active until same-run event counts prove `pending_count == finalized_count - matching_acked_count == 0`, and record that zero event immediately before disconnect.
- Disconnect Wi-Fi for at least 60 continuous seconds using monotonic timestamps, retain recording/UI/serial evidence throughout, reconnect, and leave capture active until a later same-run zero-backlog event is recorded.
- Preserve raw serial, receiver requests/artifacts, UI observations, disconnect/reconnect timestamps, firmware/configuration fingerprints, and initial hashes under one outage run and attempt.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Stopping before post-reconnect backlog reaches zero, combining primary-run events, declaring K8 before analysis, or changing firmware/configuration to respond to a failure.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/wifi_manager.c
- docs/design.md

## Acceptance
- The raw bundle contains active-recording proof and a correct same-run pending-zero event immediately before disconnect, at least 60 continuous seconds offline, reconnection, and a later pending-zero event while capture is still active.
- No Stop/restart occurs inside the K8 measurement window, and all raw artifacts remain immutable for downstream analysis.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
