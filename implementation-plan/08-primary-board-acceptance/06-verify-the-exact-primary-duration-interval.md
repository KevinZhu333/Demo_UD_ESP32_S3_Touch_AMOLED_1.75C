# Verify the exact primary duration interval

Status: Planned
Requirements: A1, K1, K3
Depends on: 08/05

## Outcome
The primary run's exact device monotonic recording interval is at least 120,000 ms, and its difference from the sum of receiver PCM frame durations is no more than 1,000 ms.

## Current gap
The earlier smoke narrowly met a duration-loss threshold but lasted only 17.348 seconds; neither result can replace the normative primary-run clock and summed-PCM calculation.

## In scope
- Read `session_duration_ms` from the same-run `session_event=stop_boundary` whose interval starts immediately after the first active WAV opens successfully and ends immediately after Stop acquires the recorder/session lock, before finalization begins.
- Require that interval to be at least 120,000 ms with no intervening restart, then sum every receiver WAV as `frame_count / sample_rate * 1000` and compute the absolute difference.
- Retain the exact input events, frame counts, sample rates, arithmetic, units, and attempt/run identifiers in the analysis record.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Using wall-clock Start/Stop observations, receiver write timestamps, WAV container length, another run, or a relaxed threshold.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- The unbroken same-run monotonic `session_duration_ms` is at least 120,000 ms.
- `abs(sum(frame_count / sample_rate * 1000) - session_duration_ms) <= 1000 ms`, with all values traceable to immutable primary artifacts.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
