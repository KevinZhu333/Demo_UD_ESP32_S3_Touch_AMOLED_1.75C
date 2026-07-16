# Apply or record the measured DMA decision

Status: Planned
Requirements: B5, B6, B7, D5, J5, K3
Depends on: 01/06

## Outcome
The repository either applies the smallest measurement-justified DMA capacity correction or records that the stall was refuted and leaves DMA unchanged, with exactly one branch selected.

## Current gap
`design.md` has no measured DMA decision, and J5 forbids speculative resource optimization while the capture continuity risk remains unresolved.

## In scope
- For a Confirmed stall, retain the 240-frame descriptor size and select the smallest descriptor count whose measured buffering covers the worst observed stall plus 128 ms.
- For a Refuted stall, make no DMA/runtime change and record the evidence-backed refutation and unchanged configuration.
- Build the selected branch and, when capacity changed, verify boot-time allocation and the effective live descriptor configuration before smoke testing.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- D7 duration changes, a PCM queue or writer task, task-topology changes, compression, or any alternate capture architecture.

## Likely files
- components/bsp_extra/src/bsp_board_extra.c
- main/main.c
- main/Kconfig.projbuild
- docs/design.md

## Acceptance
- Exactly one branch is recorded as Confirmed-stall correction or Refuted-stall unchanged DMA, with the source measurements and arithmetic attached.
- `idf.py build` passes; the Confirmed branch also allocates successfully on the supported board and reports the intended descriptor geometry.
- Failed allocation, insufficient measured capacity, or any persistent unexplained gap stops execution for `idea.md` → `spec.md` reconciliation rather than passing through a corrective architecture change.
- No K requirement is marked passed by this decision task.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
