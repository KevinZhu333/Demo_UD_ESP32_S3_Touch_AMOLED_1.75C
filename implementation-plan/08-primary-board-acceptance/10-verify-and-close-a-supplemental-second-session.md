# Verify and close a supplemental second session

Status: Planned
Requirements: F1, F2, G6, G7, G9, H2, H3, H6, J4
Depends on: 08/09

## Outcome
After the primary session reaches cleanup-gated Complete, a distinct second session starts cleanly, produces a small valid artifact set, and is explicitly Stopped, full-ACK-cleaned, and returned to empty inventory.

## Current gap
Primary acceptance alone does not prove the completed session releases Start safely or that the repaired run identity and cleanup gates permit another session without collision; an unclosed supplementary Start would contaminate phase 09.

## In scope
- Prove the primary run has zero pending acknowledgements, no WAV/sidecar/temporary-sidecar/reservation, and visible Complete before Start becomes available.
- Start one supplemental session with a run ID distinct from the primary, capture a short nonempty partial, Stop it, and accept cleanup only after the full type/run/index/SHA ACK predicate succeeds for every finalized segment.
- Wait for WAV-first ACK-driven cleanup and visible Complete, then prove authoritative empty device inventory and keep the supplemental bundle separate from all primary K calculations.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Replacing any primary K evidence, leaving an active/unfinished WAV, manually deleting durable audio, or reusing a run ID or receiver path.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Start remains unavailable until primary Complete, then one second Start succeeds with a distinct stable run ID and index sequence beginning at 1.
- The second session is explicitly Stopped, every finalized segment receives a full matching ACK, cleanup reaches Complete, and authoritative device inventory is empty.
- Supplemental artifacts are labeled and excluded from K1–K7/K9 calculations.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
