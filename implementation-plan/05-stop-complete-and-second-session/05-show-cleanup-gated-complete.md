# Show cleanup-gated Complete

Status: Planned
Requirements: F1, G7
Depends on: 05/04

## Outcome
The screen reaches `Complete · N segments uploaded` only after all full matching ACKs and successful cleanup leave no session artifact or reservation.

## Current gap
design.md records that the current UI returns to Ready and lacks the required cleanup-gated Complete transition.

## In scope
- Derive Complete from the stopped session, zero pending count, and authoritative empty artifact/reservation snapshot.
- Display the final current-run uploaded total without allowing a premature transition during cleanup retry.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Starting a second session.
- Declaring Complete from receiver files alone or from pending count alone.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Firmware builds.
- A state table covers pending ACK, pending cleanup, cleanup failure, and empty-inventory cases; only the last produces Complete.
- Complete reports the authoritative uploaded count and no WAV, canonical or temporary sidecar, or upload/cleanup reservation remains.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
