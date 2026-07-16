# Prove active-WAV power-cut exclusion

Status: Planned
Requirements: D4, F1, F3, F4, F5, G9
Depends on: 06/04

## Outcome
A controlled power cut during active capture leaves one identified unfinished WAV that is excluded from upload and blocks namespace reuse after reboot.

## Current gap
design.md classifies sudden-loss active WAVs outside recovery, but no board evidence proves they remain nonuploadable and cannot be silently overwritten.

## In scope
- Start from empty inventory, begin one synthetic-audio session, record its active path and run ID, cut power before a full segment finalizes, and hash the resulting WAV after reboot.
- Prove the orphan has no canonical or temporary sidecar, is never posted, is reported as F4 ambiguity, and keeps Start unavailable pending explicit cleanup.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Recovering, finalizing, uploading, or manually deleting the unfinished WAV in this task.
- Performing a partition write or continuing if any finalized pair or unexpected artifact exists.

## Likely files
- docs/design.md

## Acceptance
- Pre-cut inventory is empty except for the declared active path, and the evidence records firmware/configuration identity and cut timing.
- After reboot, authoritative inventory contains exactly the manifest/hash-identified WAV-only F4 orphan with no session reservation or metadata.
- Receiver logs show no POST for the orphan, and a Start attempt is rejected without overwriting or reusing its path.
- The orphan is deliberately handed to `06/06`; any additional file, ambiguity, or reservation stops the chain.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
