# Show Offline while idle

Status: Planned
Requirements: E3, G3
Depends on: 03/02

## Outcome
When Wi-Fi is lost while no session is recording, the awake screen changes from Connected to Offline and Start becomes unavailable until reconnection.

## Current gap
`design.md` has event-driven reconnect internally but no UI consumer for the required Offline state or corresponding idle Start guard.

## In scope
- Propagate an authoritative disconnect event to the idle UI and render Wi-Fi Offline without ending or inventing a session.
- Disable Start immediately on the same state update and restore the Connecting/Connected path during reconnect attempts.
- Verify the transition by removing network reachability only after an idle Connected state is observed.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Changing reconnect backoff, testing a disconnect during recording, or validating a command already queued before disconnect.

## Likely files
- main/main.c
- main/wifi_manager.c
- main/wifi_manager.h
- docs/design.md

## Acceptance
- `idf.py build` passes.
- A board disconnect while idle produces Offline and unavailable Start without creating a session or storage artifact.
- Reconnection returns through the authoritative connection states and re-enables Start only when connected and inventory remains empty.
- Sanitized evidence records state/event order without storing the SSID.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
