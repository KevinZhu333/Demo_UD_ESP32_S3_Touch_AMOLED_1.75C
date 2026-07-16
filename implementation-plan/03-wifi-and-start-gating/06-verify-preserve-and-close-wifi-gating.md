# Verify, preserve, and close Wi-Fi gating

Status: Planned
Requirements: E2, E3, E4, G3, G4, J4
Depends on: 03/05

## Outcome
One physical procedure verifies Connecting, Connected, Offline, both idle Start guards, and active-session disconnect continuity, then leaves a preserved bundle and clean device for phase 04.

## Current gap
The individual Wi-Fi behaviors need one current-build handoff that proves their ordering together and closes all active/pending state safely.

## In scope
- Exercise automatic boot connection, pre-connection Start rejection, Connected/Ready enablement, idle Offline rejection, reconnect, Start, and an active disconnect/reconnect in the declared order.
- Explicitly Stop the active session, require full type/run/index/SHA ACKs for every finalized segment, and wait for ACK-driven cleanup without claiming visible G7 Complete.
- Require authoritative empty device inventory, preserve a sanitized hash-verified bundle externally, and activate a different empty receiver root for phase 04.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Upload/pending count presentation, Upload error, stopped lifecycle progression, K8, or a demo-complete claim.

## Likely files
- main/main.c
- main/wifi_manager.c
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- The three required Wi-Fi states appear from authoritative events; Start is rejected before connection and after idle disconnect, then accepted only after reconnection with empty inventory.
- Active disconnect does not end or replace the session, and capture resumes connected presentation without a second Start.
- The final Stop is followed by full matching ACKs, ACK-driven cleanup, and an authoritative empty inventory; no visible Complete state is required or claimed.
- The bundle records firmware SHA-256, secret-free configuration fingerprint, run ID, raw-log hash, WAV/summary hashes, and observations; the phase-04 receiver directory is separate and empty.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
