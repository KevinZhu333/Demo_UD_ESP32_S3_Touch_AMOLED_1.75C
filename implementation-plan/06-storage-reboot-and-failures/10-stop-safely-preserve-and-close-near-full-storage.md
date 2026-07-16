# Stop safely, preserve, and close near-full storage

Status: Planned
Requirements: F1, F3, F4, F5, F6
Depends on: 06/09

## Outcome
A manifest-controlled near-full run stops visibly without lowering quality or deleting finalized audio, then performs the sole authorized destructive teardown only after exact inventory proves nothing durable or ambiguous can be erased.

## Current gap
design.md leaves F6 and near-full board behavior partial, and destructive cleanup is unsafe unless every remaining byte is classified as synthetic filler or the one authorized F4 orphan.

## In scope
- From clean inventory, add only manifest/hash-identified synthetic filler, record at least one full-ACK-cleaned finalized segment if capacity permits, and continue fixed-format capture until the storage error stops admission and leaves at most one known F4 orphan.
- Explicitly Stop any active session, drain every finalized pair through type/run/index/SHA ACK-driven cleanup, preserve the complete external evidence bundle, require the exact near-full inventory gate, record F4 cleanup authorization, then run `storage-flash`, restore the accepted flag-off build, prove empty inventory, and select a new empty receiver directory for phase 07.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Erasing while any active session/path, finalized pair, reservation, sidecar, unexpected WAV, or unclassified ambiguity exists.
- Lowering sample rate, changing channel/format, deleting pending audio, or treating synthetic cleanup as ordinary behavior.

## Likely files
- docs/design.md

## Acceptance
- The failure is visible, recording admission stops, PCM/WAV format is unchanged, and no finalized unacknowledged segment is overwritten or deleted.
- Before teardown, all finalized pairs have full matching ACKs and completed cleanup; authoritative inventory contains no active session/reserved path, finalized pair, upload/cleanup reservation, canonical or temporary sidecar, or unclassified ambiguity.
- The only remaining files are the exact manifest/hash-identified synthetic filler and at most one exact manifest/hash-identified F4 orphan; explicit F4 cleanup authorization is recorded before `storage-flash`.
- Evidence and raw manifests are preserved externally before the destructive command; post-provision boot reports empty inventory and all verification modes are disabled.
- The accepted flag-off firmware/configuration hashes are restored, and phase 07 receives a distinct empty `CLOUD_AUDIO_SEGMENT_DIR` without deleting prior evidence.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
