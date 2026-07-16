# Prove failed-upload retention and ACK cleanup

Status: Planned
Requirements: E9, F1, F2, F5, H4
Depends on: 06/02

## Outcome
A finalized segment survives receiver failure byte-for-byte and is deleted only after the receiver returns its full matching acknowledgment and cleanup succeeds.

## Current gap
design.md implements retry retention and WAV-first cleanup, but physical pre-ACK preservation and post-ACK cleanup evidence remain incomplete.

## In scope
- With clean inventory and no partition write, finalize one segment while the receiver is unavailable and record its WAV/metadata hashes across multiple failed attempts.
- Restore the receiver, validate WAV/checksum metadata, require the type/run/index/SHA ACK predicate, and observe WAV-first acknowledgment-driven cleanup without repost during cleanup.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Manually removing the finalized segment or accepting a partial/mismatched ACK.
- Testing reboot, ordinary flash, or filesystem-image failure.

## Likely files
- docs/design.md

## Acceptance
- Failed attempts leave WAV and canonical metadata hashes unchanged, pending nonzero, and Upload error visible.
- Receiver validation passes, exactly the full type/run/index/SHA match starts cleanup, and the request order contains no cleanup-induced repost.
- WAV deletion precedes sidecar deletion, the reservation is released last, and authoritative inventory ends empty.
- A sanitized hash-verified developmental bundle is preserved externally.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
