# Restore recording status after wake

Status: Planned
Requirements: G4, G5, G8, I6, K9
Depends on: 04/04

## Outcome
After display sleep and wake, the screen immediately reconstructs the authoritative Recording state, counts, active upload identity, Wi-Fi condition, and any Upload error without affecting capture.

## Current gap
design.md reports that sleep/wake did not stop one smoke recording, but correct awake status and upload counts are unimplemented and K9 remains unproven.

## In scope
- Refresh the full visible recording snapshot from authoritative subsystems on wake rather than from stale widget state.
- Preserve capture and uploader operation while the display is asleep and record this check as preliminary K9-shaped evidence only.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Claiming K9 acceptance outside the phase-08 primary run.
- Changing display timeout or adding power-management features.

## Likely files
- main/main.c
- main/display_power.c
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Firmware builds.
- Wake restores `Recording` or its truthful uploading/offline/error variant with the same current-run counts.
- Structured capture and upload evidence spans the sleep interval without an application-induced stop.
- The handoff labels any board result Developmental and leaves K9 open.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
