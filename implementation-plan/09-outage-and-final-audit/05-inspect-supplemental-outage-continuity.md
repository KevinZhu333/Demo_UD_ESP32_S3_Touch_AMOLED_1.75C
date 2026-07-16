# Inspect supplemental outage continuity

Status: Planned
Requirements: B1, B6, B7, D2, D3, E4, J4, K8
Depends on: 09/04

## Outcome
Ordered outage-run audio and its time-coded source show no obvious Wi-Fi-induced gap, corruption, repetition, overlap, or reordering across boundaries formed during disconnection and catch-up.

## Current gap
Counters and complete artifact indexes cannot detect samples skipped before persistence, so the K8 run needs a supplemental continuity inspection even though primary-run K4 remains the sole final boundary-listening criterion.

## In scope
- Identify every outage-run boundary whose adjacent audio spans the offline or catch-up interval and play immutable WAVs in index order without crossfade, overlap, inserted silence, or repair.
- Record listener identity, run/attempt, boundary pairs, WAV hashes, time-code observations, and explicit results for gap, corruption, duplicated/repeated audio, and ordering.
- Keep these observations labeled Supplemental / K8 continuity and correlate them only with the same outage bundle.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Replacing primary-run K4, mixing primary WAVs into playback, masking anomalies, or changing audio/firmware after a failed observation.

## Likely files
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Every identified offline/catch-up boundary has a listener, run, pair, hash, and result, with no obvious gap, corruption, repetition, overlap, or reordering.
- The record is explicitly supplemental and does not change the source run for K4.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
