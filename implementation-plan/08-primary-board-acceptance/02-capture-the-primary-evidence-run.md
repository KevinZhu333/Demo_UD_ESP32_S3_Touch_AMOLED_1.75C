# Capture the primary evidence run

Status: Planned
Requirements: A1, A4, J4, K1, K5, K7, K9
Depends on: 08/01

## Outcome
One uninterrupted Start-to-Stop session on the supported board produces the complete raw evidence set from which K1–K7 and K9 will be evaluated, with no restart or attempt mixing.

## Current gap
The existing 17.348-second smoke is developmental and cannot satisfy the specification's single primary normal-Wi-Fi run, two-minute duration, live-upload, final-partial, or screen-wake acceptance model.

## In scope
- Press Start once, run the time-coded source and normal-Wi-Fi capture continuously for at least 120 seconds, and retain raw serial, UI observations, receiver requests, WAVs, summaries, and timestamps under one run ID and attempt.
- During that same session, exercise display sleep/double-tap wake and concurrent upload; schedule Stop after at least one buffer is admitted to a new segment but before that segment reaches its full boundary.
- Capture the Stop-boundary, finalization, full-match ACK, cleanup, and stopped events without an intervening Stop, Start, restart, induced outage, or receiver impairment.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Analyzing or declaring any K pass, running K8, recapturing a failed fragment, or modifying the frozen candidate.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/wifi_manager.c
- docs/design.md

## Acceptance
- A single uniquely identified primary attempt contains a continuous Start-to-Stop event stream, at least 120 seconds of intended capture, pre-Stop upload activity, the planned nonempty-final-partial Stop position, and the sleep/wake observations.
- All raw artifacts remain immutable for downstream analyses, and their initial hashes plus firmware/configuration fingerprints are recorded.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
