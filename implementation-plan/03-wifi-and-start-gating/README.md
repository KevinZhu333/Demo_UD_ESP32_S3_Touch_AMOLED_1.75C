# Phase 03 — Wi-Fi and Start Gating

Execute the linked tasks in order. The phase is blocked whenever the current task has not passed under the corrective and replay rules in the [root plan](../README.md).

## Entry conditions

- Phase 02 passes.
- Device inventory is empty.
- A new empty receiver directory is active.
- Known-good local Wi-Fi configuration is available without exposing values.

## Task order

1. [03/01 Show Connecting with Start disabled](01-show-connecting-with-start-disabled.md)
2. [03/02 Show Connected and Ready](02-show-connected-and-ready.md)
3. [03/03 Show Offline while idle](03-show-offline-while-idle.md)
4. [03/04 Reject a stale Start after disconnect](04-reject-a-stale-start-after-disconnect.md)
5. [03/05 Keep recording through Wi-Fi loss](05-keep-recording-through-wifi-loss.md)
6. [03/06 Verify, preserve, and close Wi-Fi gating](06-verify-preserve-and-close-wifi-gating.md)

## Exit conditions

- All three Wi-Fi states and both Start guards pass.
- Active disconnect does not end recording.
- Task `03/06` Stops, full-ACK-cleans, verifies empty inventory, and preserves the bundle.
- No visible G7 Complete claim is required.
