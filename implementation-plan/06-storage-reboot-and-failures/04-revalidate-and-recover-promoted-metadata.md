# Revalidate and recover promoted metadata

Status: Planned
Requirements: D4, F1, F3, F5
Depends on: 06/03

## Outcome
On boot, a synthetic complete WAV with valid temporary metadata is revalidated, promoted safely to canonical metadata, uploaded once, and full-ACK-cleaned.

## Current gap
design.md describes temporary-sidecar recovery and strict validation, but the power-loss promotion path lacks a deterministic physical fixture and preservation proof.

## In scope
- From a current clean inventory, inject one precomputed manifest/hash-matched synthetic fixture containing a complete WAV and valid temporary metadata but no canonical sidecar or reservation.
- Boot, prove byte/checksum revalidation before promotion, ordered eligibility only after canonical metadata exists, then require full matching ACK cleanup and empty inventory.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Promoting malformed, contradictory, or hash-mismatched metadata.
- Editing the fixture image after it is written or deleting the finalized pair manually.

## Likely files
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Pre-write authoritative inventory is clean and the injected image, readback, WAV, and temporary-metadata hashes match the external manifest.
- Recovery validates WAV size/header/checksum and all metadata before atomic promotion; the uploader never opens the WAV while only invalid or incomplete metadata exists.
- Exactly one ordered POST receives a full type/run/index/SHA ACK, drives cleanup, and leaves authoritative empty inventory.
- Any unexpected path or hash stops the procedure without erase or a repair-image write.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
