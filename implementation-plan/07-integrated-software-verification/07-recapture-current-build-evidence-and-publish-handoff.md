# Recapture current-build evidence and publish the handoff

Status: Planned
Requirements: A1, A2, A3, A4, A5, B1, B2, B3, B4, B5, B6, B7, B8, C1, C2, C3, C4, C5, D1, D2, D3, D4, D5, D6, D7, D8, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, F1, F2, F3, F4, F5, F6, F7, F8, G1, G2, G3, G4, G5, G6, G7, G8, G9, H1, H2, H3, H4, H5, H6, H7, H8, H9, I1, I2, I3, I4, I5, I6, I7, I8, J1, J2, J3, J4, J5, K1, K2, K3, K4, K5, K6, K7, K8, K9
Depends on: 07/06

## Outcome
Every targeted A–J behavior needed before acceptance either has evidence from the accepted flag-off image/configuration or is recaptured on that exact candidate, and the candidate hashes are frozen for phases 08 and 09.

## Current gap
The freshness audit can identify developmental bundles whose hashes differ from the integrated build; those observations remain stale until the affected targeted checks are safely recaptured and indexed in a current-build handoff.

## In scope
- Recapture only the targeted A–J board behaviors whose prior bundles are Superseded, recording firmware/configuration fingerprints, run ID, serial-log hash, WAV/summary hashes, observations, and listener identity where applicable.
- Explicitly Stop any active session, accept cleanup only after full `SEGMENT_ACK` type/run/index/SHA matches for every finalized segment, wait for ACK-driven cleanup, and verify authoritative empty device inventory.
- Preserve a sanitized, hash-verified current-build bundle in a unique ignored or external receiver directory, then activate a different new empty `CLOUD_AUDIO_SEGMENT_DIR` for phase 08.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Using this targeted recapture as K1–K7/K9 primary-run evidence, retaining verification-mode output as default-build proof, or changing firmware/configuration after the gate.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/wifi_manager.c
- docs/design.md

## Acceptance
- Every required targeted A–J evidence row is Current for the accepted flag-off candidate or explicitly remains open for its assigned acceptance task; no stale bundle is promoted.
- The final session is Stopped, all finalized segments are full-match ACK-cleaned, authoritative inventory is empty, and the sanitized bundle plus manifest are preserved without deleting receiver evidence.
- Accepted firmware image SHA-256 and secret-free runtime-configuration fingerprint are frozen; any later change is declared a stop-for-replanning event.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
