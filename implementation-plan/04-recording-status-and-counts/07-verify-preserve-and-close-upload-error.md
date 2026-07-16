# Verify, preserve, and close upload error

Status: Planned
Requirements: E9, G5, J4
Depends on: 04/06

## Outcome
A controlled receiver failure proves that Upload error persists during retries and clears only after the affected segment's full matching ACK, then the phase closes with no device artifacts.

## Current gap
design.md lacks board evidence for the visible nonterminal error path and a preserved, sanitized phase handoff bundle.

## In scope
- Make the receiver unavailable without disconnecting Wi-Fi, observe retained counts and error status, then restore it and require a type/run/index/SHA-matching ACK.
- Explicitly Stop any active session, wait for full-matching-ACK-driven cleanup, prove authoritative empty device inventory, preserve a sanitized hash-verified bundle externally, and select a new empty receiver directory for phase 05.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Claiming visible G7 Complete before phase 05 implements it.
- Deleting receiver evidence to make the next receiver directory clean.

## Likely files
- docs/design.md

## Acceptance
- Wi-Fi stays connected while the failed request leaves the WAV and metadata pending and the error visible.
- Neither a failed request nor any mismatched ACK clears the error or starts cleanup; the full matching ACK does.
- Stop is explicit, every finalized segment is full-ACK-cleaned, and authoritative device inventory is empty without claiming G7 Complete.
- The sanitized bundle contains firmware/configuration identity, run ID, raw-log hash, WAV/summary hashes, and observations; phase 05 has a distinct empty `CLOUD_AUDIO_SEGMENT_DIR`.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
