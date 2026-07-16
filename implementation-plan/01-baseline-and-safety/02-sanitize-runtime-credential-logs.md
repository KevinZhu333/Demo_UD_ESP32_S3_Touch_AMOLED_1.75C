# Sanitize runtime credential logs

Status: Planned
Requirements: I3
Depends on: 01/01

## Outcome
Firmware logs report Wi-Fi progress without emitting the configured SSID, password, collection token, or authorization value, and a sanitized boot log proves the boundary before later board evidence is retained.

## Current gap
`design.md` identifies a direct I3 violation: `wifi_manager` currently prints the configured SSID after station startup.

## In scope
- Replace credential-bearing Wi-Fi log text with a value-free connection-state message and audit nearby runtime logging for other protected values.
- Build the firmware and perform the minimum boot/connect check needed to compare the private configured values against captured output without copying those values into evidence.
- Preserve only the sanitized log and its hash in an ignored or external evidence location.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Changing credential storage, Wi-Fi provisioning, authentication, connection behavior, or UI Wi-Fi states.

## Likely files
- main/wifi_manager.c
- main/runtime_config.c
- main/runtime_config.h
- docs/design.md

## Acceptance
- `idf.py build` passes.
- An operator-side exact-value comparison confirms that the SSID, password, collection token, and request authorization value are absent from the retained boot/connect log.
- Logs still distinguish connection progress and failure without revealing credential material.
- No pre-fix board/Wi-Fi output is retained as evidence.
- `git diff` contains no live secret or generated local configuration.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
