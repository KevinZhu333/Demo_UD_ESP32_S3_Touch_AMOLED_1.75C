# Verify run-ID stability within one session

Status: Planned
Requirements: H2, H3, H6
Depends on: 02/02

## Outcome
A multi-segment board run proves that one accepted Start uses exactly one run ID from segment 1 through final partial, acknowledgment, cleanup, and receiver metadata.

## Current gap
The new run-ID source can compile without proving that rotations, retries, and the final partial preserve the same identity end to end.

## In scope
- Start from empty inventory and capture at least one full segment plus a nonempty final partial under the phase-02 build.
- Compare run ID across Start, every finalized/ACK/cleanup record, canonical metadata, HTTP request record, ACK, receiver path, per-segment summary, and run summary.
- Stop, require the full ACK predicate, and wait for ACK-driven cleanup before preserving the developmental bundle.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Reboot uniqueness, same-boot second-Start uniqueness, duplicate-request behavior, or final acceptance evidence.

## Likely files
- main/audio_segment_recorder.c
- cloud/api/audio_receiver.py
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Exactly one nonempty run ID appears for every segment and lifecycle artifact in the session, with indexes starting at 1 and no identity mismatch.
- Receiver directory and summaries preserve the exact ID and every full type/run/index/SHA ACK matches its request.
- ACK-driven cleanup completes and authoritative device inventory is empty.
- The bundle contains firmware/configuration hashes and is labeled Developmental unless it later matches the phase-07 accepted build.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
