# Gate Start through prior-session cleanup

Status: Planned
Requirements: E3, F1, F5, G7, G9
Depends on: 05/05

## Outcome
Start remains unavailable from Stop through all ACK and cleanup work, and becomes available only after the prior session reaches authoritative Complete.

## Current gap
design.md says recorder-level namespace protection exists, but the UI exposes Start early and the combined rejection and re-enable behavior is unverified.

## In scope
- Bind visible Start availability and the command handler to the same cleanup-gated Complete predicate.
- Reject stale or concurrent Start commands while any prior-session artifact or reservation owns the namespace.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Renaming or overwriting prior-session artifacts to permit an early Start.
- Testing a new run ID for the second session.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- docs/design.md

## Acceptance
- Firmware builds.
- Start is visibly disabled and command-level rejected during stopped-uploading and stopped-finalizing.
- Cleanup failure cannot enable Start, and authoritative Complete enables it without deleting unacknowledged audio.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
