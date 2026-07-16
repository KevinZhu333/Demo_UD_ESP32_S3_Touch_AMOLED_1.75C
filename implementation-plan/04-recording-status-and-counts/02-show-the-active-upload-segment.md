# Show the active upload segment

Status: Planned
Requirements: E5, G5
Depends on: 04/01

## Outcome
While capture continues and a POST is active, the screen identifies the current run's segment index as `Recording · uploading segment N`.

## Current gap
design.md says the uploader can run concurrently with capture, but the UI neither receives the active upload identity nor renders the required recording-uploading state.

## In scope
- Expose the reserved current-run segment index only for the lifetime of its active upload or acknowledgment attempt.
- Layer the active segment identity onto Recording without changing capture ownership or task priority.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Parallel uploads or changes to the one sequential uploader.
- Retry-error presentation and stopped-session states.

## Likely files
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- main/main.c
- docs/design.md

## Acceptance
- Firmware builds.
- A deterministic check shows the displayed segment index follows the uploader reservation and clears when the attempt ends.
- Recording remains the lifecycle state throughout the upload indication.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
