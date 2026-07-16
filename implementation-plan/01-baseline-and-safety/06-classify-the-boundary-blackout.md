# Classify the boundary blackout

Status: Planned
Requirements: B5, B6, B7, D2, D3, D5, J4, K3, K4, K5
Depends on: 01/05

## Outcome
One controlled single-boundary run classifies the suspected rotation blackout as Confirmed or Refuted by correlating monotonic capture instrumentation with PCM and audible time-coded evidence.

## Current gap
`design.md` records inline rotation as a physical risk, but the 964 ms deficit is only suggestive and zero current counters do not establish where samples were lost.

## In scope
- Record a clean 20–30 second run containing one nominal boundary while upload and display work are active and the receiver remains reachable.
- Correlate rotation begin/end, next read/open, maximum inter-read gap, RX overflow, finalize/ACK events, PCM duration, and the time-coded source around that boundary.
- Record listener identity and the exact boundary pair, and classify the hypothesis using predeclared Confirmed, Refuted, or Inconclusive criteria.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Applying a DMA change, increasing segment duration, adding a PCM queue/worker, or declaring K3–K5 passed from this developmental run.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Confirmed means a measured rotation-aligned capture stall/overflow explains the missing interval; Refuted means instrumentation and source evidence exclude that mechanism while the run remains otherwise valid.
- Inconclusive does not pass: it inserts the single permitted diagnostic corrective before this task and requires this original task to pass on rerun.
- Every calculation uses one attempt and one run ID, with no evidence combined from the prior smoke or no-rotation control.
- The run is full-ACK-cleaned and authoritative inventory is empty; all K-shaped observations remain explicitly diagnostic.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
