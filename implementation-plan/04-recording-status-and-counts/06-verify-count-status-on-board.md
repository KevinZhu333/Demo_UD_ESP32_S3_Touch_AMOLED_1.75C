# Verify count status on board

Status: Planned
Requirements: E5, G5, G8, J4
Depends on: 04/05

## Outcome
A targeted board run demonstrates that current-run counts and active-upload status track real finalize, POST, full-ACK, and wake events while recording continues.

## Current gap
The new status path has software evidence only; design.md requires physical proof that board capture, Wi-Fi upload, and display activity coexist with truthful counters.

## In scope
- Run one short multi-segment board session with at least one upload overlapping capture and one display sleep/wake interval.
- Compare every visible count transition with same-run structured finalize and full-matching-ACK events and record firmware/configuration hashes.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Reusing this developmental run for final K9 acceptance.
- Evaluating post-Stop Upload error or cleanup-gated Complete.

## Likely files
- docs/design.md

## Acceptance
- Visible counts equal current-run finalized minus matching-ACK events at each recorded observation.
- The active upload index matches the sole in-flight sequential upload and capture remains active.
- Wake restores the same authoritative values, and the evidence bundle records firmware, configuration, run, serial, WAV, summary, and observation hashes.
- Results are labeled Developmental unless their hashes later match the phase-07 accepted build.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
