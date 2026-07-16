# Establish primary-run preconditions

Status: Planned
Requirements: E8, F7, I3
Depends on: 07/07

## Outcome
The supported board, receiver, normal Wi-Fi, time-coded audio source, storage, and evidence capture are ready for one uncontaminated primary attempt using the exact frozen phase-07 image and configuration.

## Current gap
Final acceptance cannot reuse a short smoke or begin from an unknown backlog, stale binary, impaired network, secret-bearing log, or receiver directory containing artifacts from another run.

## In scope
- Match the flashed image SHA-256 and secret-free runtime-configuration fingerprint to phase 07, verify the fixed same-LAN receiver is continuously reachable on normal Wi-Fi, and record boot-reported usable free space.
- Prove authoritative device inventory and the new receiver directory contain no WAV, canonical/temporary sidecar, pending artifact, upload/cleanup reservation, or prior-run output; verify usable free space meets the applicable normal-run and later F7 budget.
- Arm raw serial capture, screen observation, listener/time-coded source records, and receiver hashing without exposing SSID, password, token, or authorization values.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Starting the primary session, intentionally impairing Wi-Fi, changing firmware/configuration, or crediting any K condition.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Frozen image/configuration fingerprints match, normal Wi-Fi and receiver checks pass, usable free space is recorded, and both authoritative inventories are empty.
- Evidence capture is armed with sanitized paths and a unique attempt number before Start is pressed.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
