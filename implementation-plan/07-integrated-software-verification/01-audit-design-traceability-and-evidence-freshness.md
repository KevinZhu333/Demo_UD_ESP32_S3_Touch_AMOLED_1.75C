# Audit design traceability and evidence freshness

Status: Planned
Requirements: A1, A2, A3, A4, A5, B1, B2, B3, B4, B5, B6, B7, B8, C1, C2, C3, C4, C5, D1, D2, D3, D4, D5, D6, D7, D8, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, F1, F2, F3, F4, F5, F6, F7, F8, G1, G2, G3, G4, G5, G6, G7, G8, G9, H1, H2, H3, H4, H5, H6, H7, H8, H9, I1, I2, I3, I4, I5, I6, I7, I8, J1, J2, J3, J4, J5, K1, K2, K3, K4, K5, K6, K7, K8, K9
Depends on: 06/10

## Outcome
Every canonical requirement has a truthful design status and an evidence reference whose firmware and secret-free configuration fingerprints are known. Earlier physical bundles are explicitly classified as Current, Superseded, or Supplemental / Verification build.

## Current gap
The implementation phases can change the accepted binary and configuration, so developmental evidence cannot be trusted for final traceability until its fingerprints and verification-mode status are compared with the flag-off candidate.

## In scope
- Expand and audit all 84 A1–K9 rows against `idea.md`, `spec.md`, `design.md`, and the preserved evidence manifests, recording missing or stale evidence without promoting it.
- Verify each physical bundle records firmware SHA-256, a secret-free runtime-configuration fingerprint, run ID, raw serial-log hash, WAV and summary hashes, and any required observation or listener identity.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Changing firmware, runtime configuration, the agreed specification, or any physical acceptance result.

## Likely files
- docs/idea.md
- docs/spec.md
- docs/design.md

## Acceptance
- Exactly 84 unique canonical IDs are accounted for, with no nonexistent, duplicate, or placeholder requirement token.
- Every prior bundle is classified, and no Superseded or Supplemental / Verification-build bundle is accepted as final default-build evidence.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
