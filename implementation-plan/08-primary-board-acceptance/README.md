# Phase 08 — Primary Board Acceptance

This phase captures and analyzes one immutable normal-Wi-Fi board run for K1–K7 and K9. Run every task in order against the frozen phase-07 firmware and configuration; do not substitute or combine another run.

## Entry conditions

- Phase 07 passes.
- Accepted firmware/configuration hashes are frozen.
- Device inventory is empty.
- A new empty external receiver directory is active.
- Normal-Wi-Fi and time-coded-source preconditions pass.

## Task order

1. [08/01 — Establish primary-run preconditions](01-establish-primary-run-preconditions.md)
2. [08/02 — Capture the primary evidence run](02-capture-the-primary-evidence-run.md)
3. [08/03 — Verify capture priority and upload overlap](03-verify-capture-priority-and-upload-overlap.md)
4. [08/04 — Verify board audio format and channel usefulness](04-verify-board-audio-format-and-channel-usefulness.md)
5. [08/05 — Verify the exact ordered artifact set](05-verify-the-exact-ordered-artifact-set.md)
6. [08/06 — Verify the exact primary duration interval](06-verify-the-exact-primary-duration-interval.md)
7. [08/07 — Listen across every primary boundary](07-listen-across-every-primary-boundary.md)
8. [08/08 — Verify primary delivery and Stop timing](08-verify-primary-delivery-and-stop-timing.md)
9. [08/09 — Verify display sleep, wake, and counts](09-verify-display-sleep-wake-and-counts.md)
10. [08/10 — Verify and close a supplemental second session](10-verify-and-close-a-supplemental-second-session.md)
11. [08/11 — Preserve and audit the primary bundle](11-preserve-and-audit-the-primary-bundle.md)

## Exit conditions

- One primary run supplies K1–K7 and K9.
- The supplemental second session is excluded from K calculations, explicitly Stopped, full-ACK-cleaned, and Complete.
- Device inventory is empty.
- Task `08/11` preserves the complete sanitized primary bundle and hash manifest without deleting it.
- Phase 09 uses a different empty receiver directory.
- A physical-procedure failure reopens `08/01`–`08/11`; a firmware/configuration correction stops for replanning.

