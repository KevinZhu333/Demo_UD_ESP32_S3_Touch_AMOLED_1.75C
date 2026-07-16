# Preserve counts through Wi-Fi loss

Status: Planned
Requirements: E4, G3, G5
Depends on: 04/02

## Outcome
When Wi-Fi drops during recording, the UI changes to local-recording status while retaining truthful current-run uploaded and pending counts.

## Current gap
design.md does not yet connect Wi-Fi loss and uploader counters to one coherent UI snapshot, so count continuity across an outage is unverified.

## In scope
- Compose authoritative Wi-Fi, recording, and count snapshots without resetting counts on disconnect.
- Verify that local finalizations increase pending while no matching ACK can arrive and that reconnect preserves the same run counters.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Performing the 60-second K8 outage acceptance run.
- Adding an offline transport or another upload queue.

## Likely files
- main/main.c
- main/wifi_manager.c
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Firmware builds.
- A deterministic disconnect/reconnect check shows no count reset and no transition out of active recording.
- Displayed pending count remains equal to current-run finalized minus fully matching acknowledged segments.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
