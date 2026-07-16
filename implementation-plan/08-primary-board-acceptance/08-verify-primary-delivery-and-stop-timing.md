# Verify primary delivery and Stop timing

Status: Planned
Requirements: E8, K6, K7
Depends on: 08/07

## Outcome
Every full segment is acknowledged promptly on normal Wi-Fi, every pending transition is arithmetically correct and bounded, and the intentionally nonempty final partial receives its full matching ACK within the required Stop deadline.

## Current gap
Smoke timing covered only one full segment and cannot establish all full-segment E8 intervals, every-event K6 accounting, or the exact K7 Stop position and final-partial deadline in the primary run.

## In scope
- Pair every nominal full segment's same-run/index finalized and full type/run/index/SHA ACK event and require each `acked.event_monotonic_ms - finalized.event_monotonic_ms < 10000 ms`.
- For every same-run finalized/ACK event, independently derive `pending_count = finalized_count - matching_acked_count`, require the structured value to match, never exceed 2, and reach zero no later than 15,000 ms after the same-run Stop-boundary event.
- Prove Stop occurred after at least one buffer entered the new segment and before its full boundary, identify that nonempty final partial, and require its matching ACK event minus Stop-boundary `event_monotonic_ms` to be no more than 15,000 ms.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Using receiver file timestamps or the post-finalization stopped-log prefix as event time, counting mismatched ACKs, or crediting a full segment as the K7 final partial.

## Likely files
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Every full-segment finalize-to-matching-ACK interval is less than 10,000 ms.
- Every same-run pending event satisfies the finalized-minus-full-matching-ACK formula, the maximum is at most 2, and the zero event is within 15,000 ms after Stop.
- The evidenced Stop position is inside a new nonempty partial segment, and that segment's full matching ACK is within 15,000 ms after the Stop boundary.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
