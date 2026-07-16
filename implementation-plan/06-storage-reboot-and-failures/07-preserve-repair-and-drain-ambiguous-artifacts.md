# Preserve, repair, and drain ambiguous artifacts

Status: Planned
Requirements: D4, E7, F1, F3, F4, F5, G9
Depends on: 06/06

## Outcome
A synthetic index-1 sidecar ambiguity is preserved and repaired in place while a valid index-2 pair remains pending byte-for-byte, after which both canonical pairs drain in order through full ACK cleanup.

## Current gap
design.md preserves ambiguous artifacts and blocks reuse, but lacks a safe physical procedure proving bounded repair without overtaking, data loss, or a repair-image partition write.

## In scope
- From clean inventory, inject once a precomputed manifest containing a valid canonical index-1 pair plus one contradictory synthetic temporary sidecar and a valid pending index-2 pair; record every file hash and receiver request baseline.
- Keep index 2 pending, use the guarded in-filesystem operation to touch only the manifest/hash-matched contradictory index-1 temporary sidecar, prove index-2 WAV/metadata bytes unchanged, then upload index 1 followed by index 2 and full-ACK-clean both.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Writing a repaired partition image, deleting either finalized canonical pair, or permitting index 2 to overtake index 1.
- Continuing after any unexpected path, hash, request, reservation, or inventory classification.

## Likely files
- docs/design.md

## Acceptance
- Clean pre-write inventory and fixture-image readback match the external manifest; the initial boot preserves all files, reports index-1 ambiguity, posts neither pair, and blocks Start.
- Bounded repair touches only the declared contradictory index-1 temporary sidecar while index 2 remains pending; before/after hashes prove index-2 WAV and metadata are identical.
- The resulting canonical pairs validate and POST strictly as indexes 1 then 2; each requires the full type/run/index/SHA predicate before WAV-first cleanup.
- Authoritative inventory ends empty, repair mode is disabled, and the supplemental verification bundle is preserved externally.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
