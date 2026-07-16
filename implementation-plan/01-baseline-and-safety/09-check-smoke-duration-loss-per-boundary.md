# Check smoke duration loss per boundary

Status: Planned
Requirements: B7, D3, D5, K3
Depends on: 01/08

## Outcome
The 65-second smoke is analyzed at every rotation to determine whether PCM duration loss grows with boundary count rather than merely checking the terminal tolerance.

## Current gap
The prior smoke's 964 ms terminal difference passed the K3 limit narrowly but provided too few boundaries to show whether each synchronous rotation loses more samples.

## In scope
- Reconstruct cumulative device monotonic intervals and cumulative receiver PCM durations at each finalize boundary from only the `01/08` attempt.
- Tabulate incremental and cumulative deficit, capture stall/overflow data, segment sizes, and boundary number, including the Stop-lock terminal comparison.
- Fail on any unexplained boundary-correlated missing, duplicated, or reordered interval and record whether a monotonic accumulation trend exists.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Averaging away a failed boundary, changing firmware, relaxing K3, or promoting this diagnostic result to final K3 evidence.

## Likely files
- docs/design.md
- cloud/runtime/audio_segments.py

## Acceptance
- The analysis uses exact PCM frame counts and the first-WAV-open-to-Stop-lock monotonic clock, with units and arithmetic reproducible from the preserved bundle.
- No rotation introduces an unexplained positive deficit beyond one 64 ms capture-buffer granularity, and the sequence shows no increasing boundary-correlated deficit trend.
- The terminal absolute difference is reported against, but does not claim passage of, K3's 1,000 ms limit.
- Any accumulating or unexplained loss stops phase execution for `idea.md` → `spec.md` reconciliation.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
