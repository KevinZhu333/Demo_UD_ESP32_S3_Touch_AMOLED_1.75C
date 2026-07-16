# Phase 02 — Configuration and Identity

Execute the linked tasks in order. The phase is blocked whenever the current task has not passed under the corrective and replay rules in the [root plan](../README.md).

## Entry conditions

- Phase 01 passes.
- Device inventory is empty.
- A new empty receiver directory is active.
- I3-safe logging remains enabled.

## Task order

1. [02/01 Audit local configuration boundaries](01-audit-local-configuration-boundaries.md)
2. [02/02 Replace boot-relative run IDs](02-replace-boot-relative-run-ids.md)
3. [02/03 Verify run-ID stability within one session](03-verify-run-id-stability-within-one-session.md)
4. [02/04 Verify distinct same-boot run IDs](04-verify-distinct-same-boot-run-ids.md)
5. [02/05 Verify, preserve, and close post-reboot identity](05-verify-preserve-and-close-post-reboot-identity.md)

## Exit conditions

- Real credentials/tokens remain only in ignored local configuration or receiver environment.
- Run IDs are stable within a session and distinct across Starts and reboot.
- Task `02/05` explicitly Stops its final session, full-ACK-cleans it, verifies empty inventory, and preserves the identity bundle.
- No visible G7 Complete claim is required.
