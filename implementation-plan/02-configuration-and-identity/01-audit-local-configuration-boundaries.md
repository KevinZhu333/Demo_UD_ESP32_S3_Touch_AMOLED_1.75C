# Audit local configuration boundaries

Status: Planned
Requirements: I1, I3, I4
Depends on: 01/10

## Outcome
An explicit audit proves that device configuration remains in ignored ESP-IDF output, receiver secrets remain in environment variables, and tracked files and logs contain no live credential or token.

## Current gap
`design.md` describes the intended local configuration boundary and I3 log fix, but no post-fix evidence bundle yet proves all tracked, generated, runtime, and receiver boundaries together.

## In scope
- Inspect Kconfig definitions, ignored `sdkconfig` behavior, receiver environment use, tracked defaults, logs, documentation, fixtures, and the full working-tree diff.
- Record only symbol names, redacted presence/absence results, ignore-rule results, and sanitized hashes; do not copy configured values into commands or evidence.
- Confirm the collection-token requirement still fails closed without introducing accounts or production authentication.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Rotating credentials, provisioning Wi-Fi, changing the endpoint, or replacing the agreed demo token model.

## Likely files
- main/Kconfig.projbuild
- main/runtime_config.c
- .gitignore
- cloud/runtime/auth.py
- docs/design.md

## Acceptance
- Real Wi-Fi values and the collection token occur only in ignored local `sdkconfig` output or the receiver process environment.
- Tracked files, retained logs, fixtures, screenshots, and the task diff contain no live SSID, password, token, or authorization value.
- Receiver behavior still requires a nonblank matching environment token and device values remain sourced from ESP-IDF project configuration.
- The sanitized audit record is preserved without exposing the compared values.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
