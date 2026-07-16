# Show current-run upload counts

Status: Planned
Requirements: G5
Depends on: 03/06

## Outcome
The recording screen displays uploaded and pending counts derived from authoritative events for the active run, without including recovered or prior-run work.

## Current gap
design.md records that uploader counters exist, but the UI does not consume them and therefore cannot show the G5 counts.

## In scope
- Publish a thread-safe current-run count snapshot using finalized and fully matching acknowledged events.
- Render uploaded and pending counts while recording and refresh them when the snapshot changes.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Displaying the identity of a segment currently being uploaded.
- Implementing Upload error or post-Stop lifecycle states.

## Likely files
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- main/main.c
- docs/design.md

## Acceptance
- Firmware builds.
- A deterministic state-level check proves `pending_count = finalized_count - matching_acked_count` for the current run and excludes recovery ACKs.
- The displayed uploaded and pending values update from the authoritative snapshot without changing recording state.
- Physical-board count behavior remains Implemented / Unverified until later board tasks pass.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
