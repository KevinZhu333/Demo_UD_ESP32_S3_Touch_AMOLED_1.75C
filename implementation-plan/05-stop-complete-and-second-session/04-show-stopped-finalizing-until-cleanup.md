# Show stopped finalizing until cleanup

Status: Planned
Requirements: F1, G6, G7, G9
Depends on: 05/03

## Outcome
After every segment has a full matching ACK, the screen shows `Stopped · finalizing` for as long as any cleanup artifact or reservation remains.

## Current gap
design.md defines the finalizing condition but records that the current UI cannot represent the post-ACK, pre-cleanup interval.

## In scope
- Enter finalizing only when the stopped session's authoritative pending count is zero and cleanup truth is nonempty.
- Hold finalizing and keep Start disabled across cleanup retries without reposting the acknowledged segment.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Treating ACK count zero as the entry condition.
- Adding the deterministic cleanup fault used for the later physical check.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Firmware builds.
- State-level tests cannot enter finalizing before all full matching ACKs arrive.
- With pending zero and any WAV, sidecar, temporary sidecar, or reservation present, finalizing remains visible and Start remains unavailable.
- Cleanup retry does not issue a second POST for the acknowledged segment.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
