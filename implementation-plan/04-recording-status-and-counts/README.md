# Phase 04 — Recording Status and Counts

## Entry conditions

- Phase 03 passes.
- Device inventory is empty.
- A new empty receiver directory is active.
- Receiver unavailability can be induced without changing Wi-Fi connectivity.

## Task order

1. [04/01 — Show current-run upload counts](01-show-current-run-upload-counts.md)
2. [04/02 — Show the active upload segment](02-show-the-active-upload-segment.md)
3. [04/03 — Preserve counts through Wi-Fi loss](03-preserve-counts-through-wifi-loss.md)
4. [04/04 — Show a persistent upload error](04-show-a-persistent-upload-error.md)
5. [04/05 — Restore recording status after wake](05-restore-recording-status-after-wake.md)
6. [04/06 — Verify count status on board](06-verify-count-status-on-board.md)
7. [04/07 — Verify, preserve, and close upload error](07-verify-preserve-and-close-upload-error.md)

Run tasks in order. A task may start only after its declared predecessor passes. Any pre-phase-07 corrective follows the root [corrective-brick rules](../README.md#correctives-before-phase-07).

## Exit conditions

- Counts, active upload identity, wake restoration, and Upload error use authoritative current-run state.
- Error clearing uses the full type/run/index/SHA ACK predicate.
- Task `04/07` Stops, full-ACK-cleans, verifies empty inventory, and preserves the bundle.
- No visible G7 Complete claim is required.
- K9 remains open.

