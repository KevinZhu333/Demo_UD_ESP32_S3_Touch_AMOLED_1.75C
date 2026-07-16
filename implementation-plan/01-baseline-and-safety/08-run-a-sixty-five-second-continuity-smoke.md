# Run a sixty-five-second continuity smoke

Status: Planned
Requirements: B5, B6, B7, D1, D2, D3, D4, D5, D6, E5, E7, E8, H3, H4, H6, J4, K2, K3, K4, K5, K6, K7
Depends on: 01/07

## Outcome
A fresh approximately 65-second physical smoke captures multiple rotations, concurrent upload, and a nonempty final partial on the selected DMA branch in one coherent developmental attempt.

## Current gap
The only supplied physical run lasted 17.348 seconds and exhibited a near-limit PCM deficit, so `design.md` lacks a multi-boundary check of the measured decision.

## In scope
- Begin with authoritative empty inventory, normal Wi-Fi, a new receiver root, and a continuous audible/time-coded source; record at least 65 seconds across at least six nominal boundaries.
- Keep display and sequential upload activity concurrent, then Stop after at least one buffer enters the final partial and before its full boundary.
- Preserve structured finalize, ACK, cleanup, Stop, capture instrumentation, task-priority, UI, receiver, WAV, and checksum evidence from this one run.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Treating this setup smoke as the primary acceptance run, combining it with another run, or changing the selected DMA/segment architecture during capture.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Same-run finalized indexes and receiver artifacts are exactly ordered with no missing, duplicate, checksum, WAV, gap-counter, or storage-overflow failure.
- Every full segment satisfies its diagnostic E8-shaped finalize-to-full-matching-ACK calculation; current-run pending transitions satisfy finalized minus matching ACKs and return to zero after Stop.
- At least one full matching ACK occurs before Stop while capture, display, and upload remain active, and the final partial receives a full matching ACK within the diagnostic K7-shaped interval.
- All observations are labeled Developmental / Diagnostic; K2–K7 remain open for the phase-08 primary run.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
