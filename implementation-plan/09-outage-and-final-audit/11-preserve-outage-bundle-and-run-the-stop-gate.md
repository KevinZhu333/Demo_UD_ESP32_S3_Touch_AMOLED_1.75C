# Preserve the outage bundle and run the stop gate

Status: Planned
Requirements: A1, A2, A3, A4, A5, B1, B2, B3, B4, B5, B6, B7, B8, C1, C2, C3, C4, C5, D1, D2, D3, D4, D5, D6, D7, D8, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, F1, F2, F3, F4, F5, F6, F7, F8, G1, G2, G3, G4, G5, G6, G7, G8, G9, H1, H2, H3, H4, H5, H6, H7, H8, H9, I1, I2, I3, I4, I5, I6, I7, I8, J1, J2, J3, J4, J5, K1, K2, K3, K4, K5, K6, K7, K8, K9
Depends on: 09/10

## Outcome
The outage and primary bundles are preserved, post-reconciliation software/documentation checks pass, device state is empty, and the demo is marked Complete only if every applicable A1–J5 and physical K1–K9 condition passes.

## Current gap
Even a reconciled design cannot authorize completion until its final edit passes structure/traceability checks, the frozen software gates remain reproducible afterward, all raw evidence is immutable, and the repository/device have a safe terminal state.

## In scope
- Inventory and hash the complete outage serial log, WAVs, summaries, event calculations, observations/listener record, firmware/configuration IDs, and manifests; preserve it externally alongside but separate from the immutable primary bundle.
- Confirm no active session remains; if one exists, explicitly Stop it, require full type/run/index/SHA ACKs for every finalized segment, wait for ACK-driven cleanup/Complete, and prove authoritative empty device inventory.
- After `09/10`, rerun the complete documentation/link/heading/table/fence/diagram/traceability checks and `git diff --check`; rerun or formally reconfirm the final receiver/build commands and identical frozen hashes without changing firmware/configuration.
- Evaluate the explicit stop gate across all 84 requirement rows, preserve a sanitized hash-verified final handoff in ignored/external storage, and provision a new empty receiver directory as the clean post-demo handoff without deleting evidence.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Expanding the proof after completion, ignoring an open requirement, combining attempts, changing firmware/configuration, or deleting receiver/raw evidence to produce a clean state.

## Likely files
- docs/idea.md
- docs/spec.md
- docs/design.md
- implementation-plan/README.md

## Acceptance
- Both primary and outage bundles are sanitized, hash-verified, externally preserved, and tied to the identical frozen firmware/configuration; device inventory is empty and no active session remains.
- Receiver/build results remain passing after design reconciliation, and final documentation structure, links, exact 84-ID traceability, plan integrity, and `git diff --check` pass.
- The demo is marked Complete only when all 75 A1–J5 rows have applicable evidence and all physical K1–K9 rows pass; otherwise the handoff remains incomplete and follows replay/replanning rules.
- On completion, feature work stops; on a phase-09 procedure failure, `09/01`–`09/11` replay under a new evidence attempt, while any firmware/configuration correction stops for a revised plan and fresh phase-07/08/09 execution.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
