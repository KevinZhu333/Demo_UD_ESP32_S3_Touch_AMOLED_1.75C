# Phase 09 — Outage and Final Audit

This phase captures the separate K8 outage attempt, closes it safely, and performs the final evidence, software, documentation, and stop-work audits. Run the linked tasks in order without changing the frozen firmware or configuration.

## Entry conditions

- Phase 08 passes.
- The primary bundle remains preserved and immutable.
- Device inventory is empty.
- A different empty receiver directory is active.
- Boot-reported usable free space is at least 5,210,464 bytes.
- Firmware/configuration hashes match the frozen phase-07 build.

## Task order

1. [09/01 — Establish outage-run preconditions](01-establish-outage-run-preconditions.md)
2. [09/02 — Capture the sixty-second outage run](02-capture-the-sixty-second-outage-run.md)
3. [09/03 — Verify capture remained active through outage](03-verify-capture-remained-active-through-outage.md)
4. [09/04 — Verify catch-up and close the outage session](04-verify-catch-up-and-close-the-outage-session.md)
5. [09/05 — Inspect supplemental outage continuity](05-inspect-supplemental-outage-continuity.md)
6. [09/06 — Audit final evidence secrecy](06-audit-final-evidence-secrecy.md)
7. [09/07 — Audit current-build A1–J5 evidence](07-audit-current-build-a1-j5-evidence.md)
8. [09/08 — Audit every K1–K9 condition](08-audit-every-k1-k9-condition.md)
9. [09/09 — Run final software gates](09-run-final-software-gates.md)
10. [09/10 — Reconcile design truth with final evidence](10-reconcile-design-truth-with-final-evidence.md)
11. [09/11 — Preserve the outage bundle and run the stop gate](11-preserve-outage-bundle-and-run-the-stop-gate.md)

## Exit conditions

- K8 is recorded while capture remains active and current-run pending count is zero.
- The outage session is then explicitly Stopped, its final partial full-ACK-cleaned, Complete reached, and inventory verified empty.
- Outage teardown is not K7 evidence.
- `09/07` rejects superseded firmware/configuration bundles.
- Both primary and outage bundles remain preserved externally.
- Final software and documentation gates pass after design reconciliation.
- Only then may the demo be marked Complete and feature work stop.

