# Phase 01 — Baseline and Safety

Execute the linked tasks in order. The phase is blocked whenever the current task has not passed under the corrective and replay rules in the [root plan](../README.md).

## Entry conditions

- Governing documents are readable.
- `01/01` performs the initial working-tree and authority inspection.
- No new board/Wi-Fi evidence is retained before `01/02` closes I3.

## Task order

1. [01/01 Establish the software baseline](01-establish-software-baseline.md)
2. [01/02 Sanitize runtime credential logs](02-sanitize-runtime-credential-logs.md)
3. [01/03 Report authoritative storage inventory](03-report-authoritative-storage-inventory.md)
4. [01/04 Instrument capture continuity](04-instrument-capture-continuity.md)
5. [01/05 Run a no-rotation control](05-run-a-no-rotation-control.md)
6. [01/06 Classify the boundary blackout](06-classify-the-boundary-blackout.md)
7. [01/07 Apply or record the measured DMA decision](07-apply-or-record-the-measured-dma-decision.md)
8. [01/08 Run a sixty-five-second continuity smoke](08-run-a-sixty-five-second-continuity-smoke.md)
9. [01/09 Check smoke duration loss per boundary](09-check-smoke-duration-loss-per-boundary.md)
10. [01/10 Listen, preserve, and close the smoke](10-listen-preserve-and-close-the-smoke.md)

## DMA decision

Task `01/07` has two passing branches:

- **Confirmed stall:** use the smallest measured DMA descriptor count covering the worst stall plus 128 ms; allocation and smoke checks must pass.
- **Refuted stall:** make no DMA change, record the refutation in `design.md`, and continue to smoke verification on the unchanged DMA configuration.

An inconclusive diagnosis requires one diagnostic corrective and rerun. Persistent gaps, failed DMA allocation, or failed smoke/listening results stop for `idea.md` → `spec.md`; they cannot introduce D7, another task/queue, or another architecture.

## Exit conditions

- I3-safe logs.
- Authoritative storage inventory is available for every later precondition.
- Either the measured DMA correction passes, or the stall is refuted and unchanged DMA passes.
- Smoke duration loss does not accumulate and every boundary listens cleanly.
- K-shaped smoke results remain diagnostic.
- Task `01/10` Stops, full-ACK-cleans, verifies empty inventory, and preserves the smoke bundle in a unique external receiver directory.
