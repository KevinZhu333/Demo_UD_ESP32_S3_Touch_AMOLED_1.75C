# Run a no-rotation control

Status: Planned
Requirements: B5, D6, F8, J4, K3
Depends on: 01/04

## Outcome
A short physical run that stops before the first full boundary establishes start/stop timing and final-partial behavior without any segment rotation.

## Current gap
The prior smoke combined start/stop skew with one rotation, so `design.md` cannot yet separate a general timing deficit from a boundary-specific blackout.

## In scope
- Start from authoritative empty inventory and record a time-coded source for less than ten seconds on the supported board.
- Stop after admitted PCM exists, verify one nonempty final partial, and compare the exact first-WAV-open-to-Stop-lock interval with receiver PCM duration.
- Capture the full physical-bundle identity and hash fields, require a full type/run/index/SHA acknowledgment, and allow ACK-driven cleanup to finish.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Exercising rotation, changing DMA, treating the diagnostic duration comparison as K3 acceptance, or reusing data from the earlier smoke.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- The run contains exactly one nonempty final-partial segment and no full-segment rotation event.
- No I2S read, recorder write, or capture-resource allocation failure occurs; terminal local-gap and storage-overflow counters are zero.
- Device and receiver durations are calculated from the exact K3-shaped clocks and the result is labeled Developmental / Diagnostic, with no K condition marked passed.
- The full ACK predicate passes, cleanup completes, and authoritative inventory returns empty before the next task.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
