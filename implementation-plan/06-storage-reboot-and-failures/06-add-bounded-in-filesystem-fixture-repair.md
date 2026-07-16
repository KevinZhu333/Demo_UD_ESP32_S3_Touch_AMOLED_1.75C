# Add bounded in-filesystem fixture repair

Status: Planned
Requirements: D4, E7, F1, F3, F4, F5, G9
Depends on: 06/05

## Outcome
A guarded verification-only repair operation can remove or reconcile only explicitly authorized manifest/hash-matched synthetic files without a partition rewrite or wildcard cleanup.

## Current gap
The storage tests need deterministic teardown for the known F4 orphan and a later contradictory synthetic sidecar, but design.md has no bounded repair interface that refuses unrelated or durable files.

## In scope
- Add a nondefault maintenance action that accepts an exact external scenario ID, path allowlist, file type, length, and SHA-256 manifest and refuses any mismatch or unexpected inventory entry.
- Permit only explicit F4-orphan removal or index-1 synthetic contradiction repair, log every in-filesystem operation, never touch a finalized canonical pair, and default the action off.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Formatting, partition-image repair, wildcard deletion, or automatic cleanup of unknown user audio.
- Allowing the repair action in the accepted flag-off build.

## Likely files
- main/Kconfig.projbuild
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Firmware builds with repair disabled and in an isolated verification configuration.
- The operation refuses missing manifests, hash/length/path mismatches, canonical finalized-pair deletion, unlisted files, and any active session or reservation.
- With explicit F4 authorization, it removes only the exact orphan handed off by `06/05`, records before/after hashes, and leaves authoritative inventory empty.
- The result is labeled `Supplemental / Verification build`, and the normal path is unchanged with the option off.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
