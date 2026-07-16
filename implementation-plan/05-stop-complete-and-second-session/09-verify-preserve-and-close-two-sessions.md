# Verify, preserve, and close two sessions

Status: Planned
Requirements: F1, F2, G6, G7, G9, H2, H3, H6, J4
Depends on: 05/08

## Outcome
A two-session board procedure proves the deterministic cleanup-gated lifecycle followed by a collision-free second session, then hands off the normal flag-off build with empty device inventory and preserved evidence.

## Current gap
The lifecycle, deterministic cleanup-fault path, and second-session identity have not been physically exercised together, and the verification option must not remain enabled.

## In scope
- In a Supplemental / Verification build, Stop the first session before its first full boundary so its final partial deterministically receives the one-shot post-ACK cleanup fault; verify uploading, finalizing, WAV-first cleanup, no repost, retained sidecar/reservation, retry, and Complete.
- After that Complete, Start the second session with a distinct run ID, explicitly Stop it, require full type/run/index/SHA ACK cleanup and Complete, then restore the flag-off build, prove empty inventory, and preserve a sanitized hash-verified bundle in a unique external receiver directory for phase 06.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Treating verification-build fault evidence as normal-build acceptance evidence.
- Leaving either session active, manually deleting finalized audio, or reusing the phase receiver directory.

## Likely files
- docs/design.md

## Acceptance
- The injected run has exactly one POST, one full matching ACK, WAV-first deletion, no repost during cleanup retry, retained sidecar/reservation during finalizing, and later Complete.
- The cleanup-fault option is disabled after the procedure and the handed-off flag-off firmware/configuration hashes are recorded.
- Both sessions are explicitly Stopped, their nonempty partials are full-ACK-cleaned, the second begins only after first-session Complete, and run IDs are distinct.
- Authoritative device inventory is empty; the sanitized bundle records firmware/configuration fingerprints, run IDs, raw-log hashes, WAV/summary hashes, observations, and supplemental labeling.
- Phase 06 is assigned a different empty `CLOUD_AUDIO_SEGMENT_DIR`; no receiver evidence is deleted.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
