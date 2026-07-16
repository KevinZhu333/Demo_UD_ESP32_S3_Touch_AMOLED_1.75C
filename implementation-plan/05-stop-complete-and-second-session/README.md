# Phase 05 — Stop, Complete, and Second Session

## Entry conditions

- Phase 04 passes.
- Device inventory is empty.
- A new empty receiver directory and request log are active.
- Count and error snapshots are authoritative.

## Task order

1. [05/01 — Enter stopped uploading](01-enter-stopped-uploading.md)
2. [05/02 — Retain upload error after Stop](02-retain-upload-error-after-stop.md)
3. [05/03 — Publish authoritative post-ACK cleanup state](03-publish-authoritative-post-ack-cleanup-state.md)
4. [05/04 — Show stopped finalizing until cleanup](04-show-stopped-finalizing-until-cleanup.md)
5. [05/05 — Show cleanup-gated Complete](05-show-cleanup-gated-complete.md)
6. [05/06 — Gate Start through prior-session cleanup](06-gate-start-through-prior-session-cleanup.md)
7. [05/07 — Start a clean second session](07-start-a-clean-second-session.md)
8. [05/08 — Add a one-shot post-ACK cleanup fault](08-add-a-one-shot-post-ack-cleanup-fault.md)
9. [05/09 — Verify, preserve, and close two sessions](09-verify-preserve-and-close-two-sessions.md)

Run tasks in order. A task may start only after its declared predecessor passes. Any pre-phase-07 corrective follows the root [corrective-brick rules](../README.md#correctives-before-phase-07).

## Exit conditions

- Uploading → finalizing → Complete follows pending and cleanup truth.
- Cleanup-fault verification shows WAV-first deletion, no repost, retained sidecar/reservation, retry, and later Complete.
- Both first and second sessions are explicitly Stopped and full-ACK-cleaned.
- Cleanup-fault option is disabled in the handed-off build.
- Device inventory is empty and the bundle is preserved.

