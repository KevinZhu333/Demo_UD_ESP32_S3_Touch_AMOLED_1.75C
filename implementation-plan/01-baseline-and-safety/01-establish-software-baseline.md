# Establish the software baseline

Status: Planned
Requirements: A4, H3, H4, H5, H6, I3, I4
Depends on: None

## Outcome
A reproducible, read-only baseline records the clean working tree, aligned governing documents, receiver-test result, firmware-build result, and current secret scan before behavioral work begins.

## Current gap
`design.md` describes a partially implemented system, but this plan has no fresh, attempt-linked software baseline against which later changes and evidence can be compared.

## In scope
- Inspect the working tree and confirm `idea.md`, `spec.md`, and `design.md` are mutually consistent without modifying implementation.
- Run the receiver suite, run an ESP-IDF firmware build, and inspect tracked source and generated diffs for credential, token, and authorization values.
- Record exact commands, toolchain identity, results, commit identity, and firmware image SHA-256 in a sanitized ignored or external baseline bundle.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Firmware behavior changes, flashing a board, starting Wi-Fi, or retaining new physical-board evidence.

## Likely files
- AGENTS.md
- docs/idea.md
- docs/spec.md
- cloud/tests/
- main/
- docs/design.md

## Acceptance
- The initial working-tree state is recorded and unrelated user changes, if any, are preserved and excluded.
- `python -m pytest -q cloud/tests` and `idf.py build` pass in their required environments; a missing required environment blocks the task rather than becoming a passing result.
- The authority audit finds no unresolved `idea.md`/`spec.md` disagreement, and the sanitized diff/secret inspection finds no exposed live credential or token.
- No board or Wi-Fi run is performed or retained by this task.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
