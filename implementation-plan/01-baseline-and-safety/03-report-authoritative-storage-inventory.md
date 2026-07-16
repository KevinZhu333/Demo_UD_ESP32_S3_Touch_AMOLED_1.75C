# Report authoritative storage inventory

Status: Planned
Requirements: F1, F3, F5, F7
Depends on: 01/02

## Outcome
One read-only inventory operation reports usable SPIFFS capacity, active and upload reservations, and every segment artifact classified by durability and ambiguity so later storage gates can rely on a single source of truth.

## Current gap
`design.md` describes artifact and reservation rules, but it does not identify an operator-visible inventory result that can prove an empty precondition without guessing from selected filenames.

## In scope
- Add a bounded read-only inventory snapshot covering active paths, upload/cleanup reservations, WAVs, canonical sidecars, temporary sidecars, finalized pairs, unfinished files, and ambiguous combinations.
- Report SPIFFS total, used, usable free bytes, and explicit counts with a stable completion record suitable for evidence hashing.
- Ensure inventory inspection never promotes, deletes, renames, truncates, formats, or otherwise mutates an artifact.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Repairing an artifact, mounting with format-on-failure, erasing storage, or changing upload eligibility.

## Likely files
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- main/main.c
- docs/design.md

## Acceptance
- `idf.py build` passes.
- Repeated inventory calls on unchanged storage produce the same classifications and do not change filesystem bytes, reservations, or usable-space accounting.
- The report explicitly distinguishes an empty inventory from active, pending-finalized, temporary, unfinished, and ambiguous states and includes the F7 usable-free-space value.
- A board invocation completes without a partition write and its sanitized record is preserved with firmware/configuration hashes.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
