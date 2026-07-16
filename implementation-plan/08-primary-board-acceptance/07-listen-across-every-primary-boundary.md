# Listen across every primary boundary

Status: Planned
Requirements: B1, B6, B7, D2, D3, J4, K4
Depends on: 08/06

## Outcome
One named listener evaluates every adjacent primary-run segment boundary in index order and finds no obvious gap, corruption, duplicated/repeated audio, or reordering.

## Current gap
Automated silence, clipping, checksum, and duplicate-buffer checks cannot detect samples skipped entirely; the primary run still needs the normative every-boundary listening result.

## In scope
- Play the immutable receiver WAVs in index order without crossfade, inserted silence, overlap, repair, normalization, or omitted boundary, using the time-coded source to localize any discontinuity.
- Record listener identity, run ID, attempt, every boundary pair `(n, n+1)`, playback method, and an explicit pass/fail observation for gap, corruption, or repeated audio.
- Correlate any anomaly with the same-run finalize/upload events without altering the captured WAVs.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Substituting the phase-01 smoke or phase-09 supplemental listening, masking a defect with playback overlap, or changing firmware after a failed result.

## Likely files
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- The record contains one result for every adjacent pair from `(1,2)` through `(T-1,T)` and identifies the listener, run, attempt, and immutable WAV hashes.
- Every boundary has no obvious gap, corruption, repeated/duplicated audio, or ordering error; any failure triggers a full phase-08 procedure replay or replanning under the root rules.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
