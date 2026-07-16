# Reconcile design truth with final evidence

Status: Planned
Requirements: A1, A2, A3, A4, A5, B1, B2, B3, B4, B5, B6, B7, B8, C1, C2, C3, C4, C5, D1, D2, D3, D4, D5, D6, D7, D8, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, F1, F2, F3, F4, F5, F6, F7, F8, G1, G2, G3, G4, G5, G6, G7, G8, G9, H1, H2, H3, H4, H5, H6, H7, H8, H9, I1, I2, I3, I4, I5, I6, I7, I8, J1, J2, J3, J4, J5, K1, K2, K3, K4, K5, K6, K7, K8, K9
Depends on: 09/09

## Outcome
`docs/design.md` becomes the exact final technical and evidence record for all 84 requirements, with no stale Partial, Verified, Complete, gap, interface, or implementation claim.

## Current gap
Per-task reconciliation keeps the design close to reality, but only the complete primary/outage matrices and final software-gate results can support the aggregate final statuses and explicit stop-work conclusion.

## In scope
- Reconcile every A1–K9 traceability row, implementation-status summary, known gap, physical procedure result, interface description, and evidence reference against the immutable final manifests.
- Distinguish implementation, software-gate, developmental, supplemental/verification-build, primary physical, and outage physical evidence, and retain any failed/open condition without weakening the specification.
- Verify `idea.md` and `spec.md` still agree; do not alter them merely to fit observed behavior.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Changing implementation/specification, manufacturing a completion status, deleting contrary evidence, or adding a new product requirement.

## Likely files
- docs/idea.md
- docs/spec.md
- docs/design.md

## Acceptance
- Every one of the 84 canonical IDs has a truthful final design status and immutable evidence reference, and aggregate sections agree with the detailed traceability rows.
- No developmental, stale, supplemental, or software-only result is mislabeled as required physical verification.
- `idea.md` and `spec.md` remain aligned; any disagreement blocks the final stop gate.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
