# Verify document and plan integrity

Status: Planned
Requirements: A1, A2, A3, A4, A5, B1, B2, B3, B4, B5, B6, B7, B8, C1, C2, C3, C4, C5, D1, D2, D3, D4, D5, D6, D7, D8, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, F1, F2, F3, F4, F5, F6, F7, F8, G1, G2, G3, G4, G5, G6, G7, G8, G9, H1, H2, H3, H4, H5, H6, H7, H8, H9, I1, I2, I3, I4, I5, I6, I7, I8, J1, J2, J3, J4, J5, K1, K2, K3, K4, K5, K6, K7, K8, K9
Depends on: 07/01

## Outcome
The governing documents and implementation plan are mechanically coherent: links resolve, Markdown structures are sound, traceability is complete, and the canonical task graph remains one executable chain.

## Current gap
Many incremental documentation updates and any allowed pre-phase-07 corrective insertion can leave stale links, malformed Markdown, missing IDs, or broken dependencies even when individual task handoffs passed.

## In scope
- Validate local relative links, headings, tables, code fences, diagrams, exact A1–K9 coverage, task statuses, phase counts, and the canonical dependency graph under the root plan rules.
- Run whitespace/error checks and confirm every corrective, if present, follows its naming, link, dependency, rerun-status, and canonical-count rules.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Editing firmware or receiver behavior, or treating documentation integrity as physical-board evidence.

## Likely files
- implementation-plan/README.md
- implementation-plan/07-integrated-software-verification/README.md
- docs/design.md

## Acceptance
- All documentation integrity checks pass, all plan links resolve, and the canonical graph contains 76 nodes and 75 immediate-predecessor edges before any permitted corrective additions.
- Requirement coverage contains the same 84 canonical IDs as `idea.md` and `spec.md`, and `git diff --check` passes.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
