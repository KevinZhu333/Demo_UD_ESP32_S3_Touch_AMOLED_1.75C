# Show Connected and Ready

Status: Planned
Requirements: E2, E3, G3
Depends on: 03/01

## Outcome
After the authoritative station/IP connection event, the screen shows Wi-Fi Connected and Ready and enables Start exactly once the current connection is usable.

## Current gap
The Connecting state introduced by `03/01` has no verified transition to the required Connected/Ready presentation and enabled control.

## In scope
- Drive the UI's Connected and Ready presentation from the authoritative Wi-Fi connection event rather than elapsed time or startup completion.
- Enable Start only when connectivity is true and the existing prior-session storage gate also permits it.
- Exercise automatic STA connection from boot through the first usable Connected state.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Idle disconnect presentation, stale queued commands, active-session disconnect handling, or second-session cleanup gating.

## Likely files
- main/main.c
- main/wifi_manager.c
- main/wifi_manager.h
- docs/design.md

## Acceptance
- `idf.py build` passes.
- A board boot progresses from Connecting to Connected/Ready only after the authoritative connection event.
- Start remains unavailable before that event and becomes available after it when authoritative inventory is empty.
- No credential value is present in retained logs or UI text.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
