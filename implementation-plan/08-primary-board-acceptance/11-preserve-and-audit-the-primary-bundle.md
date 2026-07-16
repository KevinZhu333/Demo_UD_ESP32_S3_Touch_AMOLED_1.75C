# Preserve and audit the primary bundle

Status: Planned
Requirements: A2, J4, K1, K2, K3, K4, K5, K6, K7, K9
Depends on: 08/10

## Outcome
One complete, sanitized, hash-verified primary bundle is preserved immutably as the sole source for K1–K7/K9, while device and receiver state are safely prepared for the separate K8 run.

## Current gap
Individual analyses can pass while their raw inputs are incomplete, mixed across attempts, mutable, secret-bearing, or lost during receiver cleanup; the phase needs one audited evidence boundary before K8 begins.

## In scope
- Inventory and hash the primary raw serial log, WAVs, per-segment and run summaries, UI/listening observations, run/configuration/firmware identifiers, calculations, and the distinct supplemental-session record.
- Cross-check that every K1–K7/K9 conclusion references only the single primary run/attempt and frozen hashes; preserve the bundle in an ignored or external immutable location without deleting receiver evidence.
- Confirm no active session remains; if one exists, Stop it, require full type/run/index/SHA ACK cleanup for every finalized segment, wait for cleanup, and require authoritative empty device inventory.
- Activate a different new empty `CLOUD_AUDIO_SEGMENT_DIR` for phase 09 and record that path's empty inventory without removing the phase-08 directory.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Supplying K8, combining the supplemental run with primary calculations, changing firmware/configuration, or deleting raw primary evidence to make the next receiver directory clean.

## Likely files
- cloud/runtime/audio_segments.py
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- The preserved manifest contains firmware image SHA-256, secret-free runtime-configuration fingerprint, run ID, raw-log hash, every WAV/summary hash, calculations, observations, and listener identity.
- K1–K7/K9 all reference the same immutable primary attempt, the supplemental session is excluded, and no secret or authorization value is present.
- Device inventory is empty, receiver evidence remains preserved, and a different verified-empty receiver directory is active for phase 09.
- A procedure failure triggers replay of `08/01`–`08/11`; any required firmware/configuration change stops for replanning and fresh phase-07/08/09 execution.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
