# Audit final evidence secrecy

Status: Planned
Requirements: I3
Depends on: 09/05

## Outcome
The preserved primary, supplemental, and outage evidence contains no SSID, password, token, authorization value, or other live credential, while sensitive raw PCM remains only in approved ignored or external storage.

## Current gap
Final evidence combines serial, receiver, screen, audio, configuration, and manifest material, so earlier log sanitation alone cannot rule out disclosure introduced during capture or bundling.

## In scope
- Inspect every final candidate bundle and manifest for credential-bearing logs, request headers, configuration values, screenshots, filenames, summaries, and copied command output without echoing a discovered secret.
- Confirm raw audio and any storage images/backups remain in ignored or external locations with sanitized metadata and are absent from the tracked diff.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Printing secret matches or surrounding live values, committing evidence, deleting immutable bundles, or adding a production credential system.

## Likely files
- main/wifi_manager.c
- main/runtime_config.c
- .gitignore
- docs/design.md

## Acceptance
- No final evidence or tracked change contains an SSID, password, token, authorization value, or generated live configuration.
- Raw PCM/storage images remain only in explicitly ignored or external locations and the audit record itself is sanitized.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
