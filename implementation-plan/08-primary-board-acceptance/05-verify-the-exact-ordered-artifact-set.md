# Verify the exact ordered artifact set

Status: Planned
Requirements: H3, H4, H6, K2
Depends on: 08/04

## Outcome
The same-run device finalization records, receiver WAVs, per-segment metadata, and `summary.json` independently describe exactly one ordered terminal set `1..T` with valid contents and no missing or duplicate index.

## Current gap
The short smoke contained indexes 1 and 2, but K2 requires the terminal set of the full primary run to agree across every independent event and receiver representation.

## In scope
- Read `T` from the same-run terminal `session_event=stopped` finalized value, require `T >= 1`, and compare finalized events, WAV filenames, per-segment summaries, and receiver `summary.json` against exactly `1..T`.
- Validate each WAV/checksum and required metadata identity, start/duration/size/counter fields; confirm receiver received/first/last/missing/duplicate results agree with the exact set.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Combining artifacts from another run or attempt, accepting filename order without content validation, or claiming audible K4 continuity.

## Likely files
- cloud/api/audio_receiver.py
- cloud/runtime/audio_segments.py
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Same-run finalized events, receiver WAVs, per-segment summaries, and `summary.json` each contain exactly indexes `1..T` once, with `T >= 1` and zero missing/duplicate indexes.
- Every accepted artifact has matching run/index identity, SHA-256, valid uncompressed WAV structure, and complete required metadata.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
