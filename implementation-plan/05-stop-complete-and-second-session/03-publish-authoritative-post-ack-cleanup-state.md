# Publish authoritative post-ACK cleanup state

Status: Planned
Requirements: F1, F2, G5, G6, G7, G9
Depends on: 05/02

## Outcome
The UI receives a distinct post-ACK cleanup snapshot in which pending count is zero only after all finalized segments have full matching ACKs, while remaining artifacts and reservations are still reported.

## Current gap
design.md emits separate ACK and cleanup records, but the UI does not consume cleanup ownership and cannot distinguish zero pending acknowledgments from a fully cleaned session.

## In scope
- Publish pending count, cleanup-in-progress, remaining-artifact, and reservation truth as one synchronized lifecycle snapshot.
- Preserve the existing WAV-first cleanup order and release reservations only after all associated artifacts are gone.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Adding a cleanup-failure injection mechanism.
- Rendering finalizing or Complete before the snapshot interface is established.

## Likely files
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- main/main.c
- docs/design.md

## Acceptance
- Firmware builds.
- A deterministic check proves `pending_count == 0` follows full type/run/index/SHA ACKs and can coexist with cleanup artifacts or reservations.
- A separate cleanup-complete event appears only after the WAV, sidecars, temporary sidecars, and reservation are absent.
- Start eligibility remains false while the cleanup snapshot reports any artifact or reservation.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
