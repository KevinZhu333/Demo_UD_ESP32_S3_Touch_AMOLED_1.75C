# Audit current-build A1–J5 evidence

Status: Planned
Requirements: A1, A2, A3, A4, A5, B1, B2, B3, B4, B5, B6, B7, B8, C1, C2, C3, C4, C5, D1, D2, D3, D4, D5, D6, D7, D8, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, F1, F2, F3, F4, F5, F6, F7, F8, G1, G2, G3, G4, G5, G6, G7, G8, G9, H1, H2, H3, H4, H5, H6, H7, H8, H9, I1, I2, I3, I4, I5, I6, I7, I8, J1, J2, J3, J4, J5
Depends on: 09/06

## Outcome
Every A1–J5 requirement has applicable, current-build evidence or a truthful failing/open status; no superseded or verification-build-only bundle is accepted for the default proof.

## Current gap
Passing K conditions is insufficient for demo completion, and evidence gathered before the frozen build or under a guarded fault option may be stale or too narrow for the final 75-row audit.

## In scope
- Audit each exact A1–J5 row against the frozen phase-07 firmware SHA-256, secret-free configuration fingerprint, software results, primary/outage bundles, and targeted phase-07 recapture.
- Reject any Superseded bundle, classify verification-mode artifacts as Supplemental / Verification build, and require default-build evidence wherever a requirement claims normal behavior.
- Record evidence type and applicability so compiled/static results are never presented as physical-board proof and conditional gates remain conditional.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Auditing K1–K9, filling a missing row with evidence from another binary, or changing firmware/configuration after the frozen gate.

## Likely files
- docs/idea.md
- docs/spec.md
- docs/design.md

## Acceptance
- All 75 unique A1–J5 IDs have an evidence citation and honest status, with firmware/configuration freshness checked for every physical bundle.
- No Superseded or Supplemental / Verification-build-only result supports a default-build completion claim, and any missing applicable evidence blocks completion.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
