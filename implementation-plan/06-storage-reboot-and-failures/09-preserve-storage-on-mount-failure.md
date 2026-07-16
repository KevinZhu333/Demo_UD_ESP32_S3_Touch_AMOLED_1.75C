# Preserve storage on mount failure

Status: Planned
Requirements: F1, F3, F5
Depends on: 06/08

## Outcome
A hash-controlled invalid filesystem image causes a safe nonformatting mount failure, and the exact recorded clean backup is restored only after raw readback proves the failed boot made no write.

## Current gap
design.md says mount failure must preserve the partition, but physical evidence and a narrowly safe restore procedure are absent.

## In scope
- On the disposable board with authoritative clean inventory and no active session, record a raw known-good backup and a distinct invalid image, their exact geometry and SHA-256 values, then inject and read back the invalid image before one boot.
- Forbid all sessions and intervening writes, prove the post-failure raw hash is unchanged, restore only the recorded known-good backup, verify restoration readback, boot, and prove empty inventory.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Attempting this scenario with pending, ambiguous, active, or unclassified audio.
- Formatting on mount failure, generating a new restore image, or retrying any write after a hash mismatch.

## Likely files
- main/audio_segment_recorder.c
- partitions.csv
- docs/design.md

## Acceptance
- Before injection, authoritative inventory is clean and exact known-good and invalid-image hashes, board identity, offset, size, and commands are recorded externally.
- Invalid-image readback matches before boot; the failed boot performs no format or session, and a second raw readback remains exactly the invalid-image hash.
- Only the recorded known-good backup is restored, its readback matches the original known-good hash, and the next boot reports authoritative empty inventory.
- Any mismatch stops without another write; the external bundle preserves all raw hashes and labels the board disposable.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
