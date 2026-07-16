# Listen, preserve, and close the smoke

Status: Planned
Requirements: B1, B6, B7, D2, D3, J4, K4
Depends on: 01/09

## Outcome
Every adjacent WAV boundary from the 65-second smoke receives a recorded listening result, and the phase closes with durable evidence and an empty device for phase 02.

## Current gap
`design.md` has no multi-boundary listening record, and analysis alone cannot prove the absence of audible gaps, repeated speech, or corruption.

## In scope
- Play the `01/08` WAVs in index order without crossfade, inserted silence, or overlap and record listener identity, run ID, every adjacent index pair, source cue, and result.
- Explicitly Stop if any session is still active, require the full type/run/index/SHA predicate for every finalized segment, and wait for ACK-driven cleanup.
- Require authoritative empty device inventory, preserve the sanitized hash-verified smoke bundle externally, and activate a different empty `CLOUD_AUDIO_SEGMENT_DIR` for phase 02.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Editing audio, concealing a failed boundary, deleting receiver evidence to clean the next run, or claiming final K4 passage.

## Likely files
- docs/design.md
- cloud/runtime/audio_segments.py

## Acceptance
- Every adjacent boundary has a named listener result with no obvious gap, corruption, or repeated audio; any failure stops for `idea.md` → `spec.md` reconciliation.
- Every finalized segment has a full matching ACK, ACK-driven cleanup completes, and authoritative inventory reports no active path, artifact, ambiguity, or reservation.
- The external bundle records firmware SHA-256, secret-free configuration fingerprint, run ID, raw-log hash, WAV/summary hashes, and observation record; it is labeled Developmental / Diagnostic.
- The phase-02 receiver directory exists separately and is empty without deleting the phase-01 bundle.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
