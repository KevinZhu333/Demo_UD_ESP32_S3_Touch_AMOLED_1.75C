# Audit secrets and generated configuration

Status: Planned
Requirements: I1, I3, I4
Depends on: 07/04

## Outcome
The integrated diff, tracked tree, build handoff, and evidence references contain no live Wi-Fi credentials, collection token, authorization value, SSID, raw-audio payload, or generated local configuration.

## Current gap
The build and physical-development workflow consumes local secrets and sensitive audio, so clean earlier tasks do not prove that the final candidate or accumulated documentation is free of accidental disclosure.

## In scope
- Inspect tracked changes, ignored/generated configuration boundaries, logs, fixtures, documentation, and referenced evidence manifests for prohibited secret or sensitive values.
- Confirm real Wi-Fi credentials and device token remain only in ignored local configuration or receiver environment, and that sanitized fingerprints cannot reconstruct them.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Printing, copying, rotating, or requesting any live credential; adding a production secret-management system.

## Likely files
- .gitignore
- main/runtime_config.c
- main/wifi_manager.c
- docs/design.md

## Acceptance
- Secret and generated-configuration inspection finds no prohibited tracked or logged value, and the evidence bundle paths are ignored or external.
- The audit output itself is sanitized and does not expose matching context around a live secret.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
