# Verify catch-up and close the outage session

Status: Planned
Requirements: E4, E5, E7, F1, F5, F7, H3, J4, K8
Depends on: 09/03

## Outcome
The current-run backlog drains sequentially to zero within 60 seconds of reconnection while capture remains active, after which the session is explicitly Stopped and its final partial is full-ACK-cleaned to empty inventory.

## Current gap
K8 has not passed until every same-run pending transition is validated, the timed zero occurs while recording, and the still-active run is subsequently closed without discarding or overtaking durable audio.

## In scope
- Recompute every outage-run pending transition as `finalized_count - full type/run/index/SHA matching_acked_count`, verify structured counts, and establish that pre-disconnect zero immediately preceded the 60-second disconnection.
- From the recorded reconnect timestamp, require the formula to return to zero within 60,000 ms while capture remains active; verify sequential index delivery with no overtaking, overwrite, discard, or mismatched-ACK cleanup.
- After recording the qualifying active-capture zero timestamp, explicitly Stop, finalize the nonempty final partial if present, require full matching ACKs, wait for ACK-driven cleanup and Complete, and prove authoritative empty device inventory.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Treating the teardown Stop/final-partial timing as K7, counting ACKs from another run, or manually deleting a retained segment.

## Likely files
- main/audio_segment_recorder.c
- main/wifi_manager.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Every pending transition matches finalized-minus-full-matching-ACK arithmetic, and backlog reaches zero within 60,000 ms after reconnect while the same session is still Recording.
- Receiver artifacts arrive sequentially under one run ID with no missing/overtaken/dropped durable segment.
- The later teardown Stop, final full-match ACK cleanup, Complete state, and authoritative empty inventory pass and are explicitly excluded from primary K7 evidence.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
