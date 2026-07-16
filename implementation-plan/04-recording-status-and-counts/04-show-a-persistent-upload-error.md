# Show a persistent upload error

Status: Planned
Requirements: E9, G5
Depends on: 04/03

## Outcome
A retryable POST or acknowledgment failure raises a nonterminal `Upload error` indicator that persists until the affected segment receives a full matching acknowledgment.

## Current gap
design.md records upload retry and failure counts, but no failure state reaches the UI and the existing implementation cannot demonstrate persistent error presentation.

## In scope
- Publish the affected run/index and retryable failure state without removing its WAV, metadata, pending count, or reservation.
- Clear the indicator only when type, run ID, segment index, and SHA-256 all match the acknowledgment contract for that segment.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Treating Upload error as a terminal lifecycle state.
- Changing retry order, retry delay policy, or the receiver protocol.

## Likely files
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- main/main.c
- docs/design.md

## Acceptance
- Firmware builds.
- A deterministic failed-attempt check leaves the segment pending and the indicator visible through retries.
- A mismatched type, run, index, or SHA does not clear the indicator, release the reservation, or start cleanup.
- A full matching ACK clears the affected error without ending an active recording.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
