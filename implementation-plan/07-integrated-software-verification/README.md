# Phase 07 — Integrated Software Verification

This phase freezes a default, verification-option-off firmware and configuration candidate only after the design, software, documentation, secrecy, architecture, and evidence-freshness gates agree. Run the linked tasks in order; a failure blocks every successor.

## Entry conditions

- Phases 01–06 pass.
- Device inventory is empty.
- Verification options are disabled.
- Every prior evidence bundle contains firmware/configuration hashes.

## Task order

1. [07/01 — Audit design traceability and evidence freshness](01-audit-design-traceability-and-evidence-freshness.md)
2. [07/02 — Verify document and plan integrity](02-verify-document-and-plan-integrity.md)
3. [07/03 — Run the receiver software gate](03-run-the-receiver-software-gate.md)
4. [07/04 — Run the firmware software gate](04-run-the-firmware-software-gate.md)
5. [07/05 — Audit secrets and generated configuration](05-audit-secrets-and-generated-configuration.md)
6. [07/06 — Audit architecture, interface, and duration guardrails](06-audit-architecture-interface-and-duration-guardrails.md)
7. [07/07 — Recapture current-build evidence and publish the handoff](07-recapture-current-build-evidence-and-publish-handoff.md)

## Evidence-freshness rules

- `07/01` classifies each earlier bundle as Current, Superseded, or Supplemental/Verification-build.
- `07/06` substantively checks fixed in-session duration under D8 and the Start/Stop-only, no-Play interface under G1/G2.
- `07/07` recaptures any targeted behavior still needed for A–J evidence when the earlier bundle hash differs from the accepted flag-off build.
- Verification-mode evidence remains supplemental and cannot replace default-build evidence.

## Exit conditions

- Design, receiver, firmware, documentation, secret, and architecture gates pass.
- Required targeted evidence matches the accepted flag-off build or is recaptured.
- Device inventory is empty.
- The accepted firmware/configuration hashes are frozen.
- Any later firmware/configuration change stops for replanning.

