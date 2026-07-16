# Verify display sleep, wake, and counts

Status: Planned
Requirements: G4, G5, G8, I6, J4, K9
Depends on: 08/08

## Outcome
During the same primary recording, the display sleeps and wakes on one double-tap without changing the session, then shows Recording with uploaded and pending counts equal to the latest same-run structured state.

## Current gap
The smoke showed sleep/wake did not stop capture, but it did not prove correct post-wake counts or the complete K9 behavior on the accepted build under concurrent upload load.

## In scope
- Locate the primary-run sleep and single double-tap wake interval, verify capture and upload continued, and prove the gesture caused no Start, Stop, or other consent-boundary event.
- At least 200 ms after wake, compare the visible Recording label and uploaded/pending values with the latest preceding same-run structured counts, retaining timestamped screen and serial observations.
- Confirm the screen contains no microphone audio content and that the behavior occurred under the concurrent load analyzed for B5/K5.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Using a different run, accepting stale pre-sleep values, exercising a second Start, or treating display wake as permission to change capture state.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Exactly one intended double-tap wakes the sleeping display without a Start/Stop transition while recording/upload remain active.
- After the 200 ms UI refresh, the awake screen shows Recording and values equal to the latest same-run uploaded and pending structured counts.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
