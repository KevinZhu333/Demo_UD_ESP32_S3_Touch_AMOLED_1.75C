# Show Connecting with Start disabled

Status: Planned
Requirements: E2, E3, G3
Depends on: 02/05

## Outcome
From boot until the authoritative Wi-Fi manager reports a usable connection, the screen shows `Connecting to Wi-Fi` and Start cannot enqueue or begin recording.

## Current gap
`design.md` says the UI currently enters Ready before deferred Wi-Fi startup and enables Start without consuming connection state.

## In scope
- Publish the authoritative Wi-Fi connection snapshot to the UI and initialize the presentation as Connecting rather than Ready.
- Disable the Start control and reject its command path while the connection is not established.
- Verify the state with connection deliberately delayed, without logging configured credential values.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- The Connected transition, idle Offline transition, reconnect policy changes, provisioning, or behavior after recording starts.

## Likely files
- main/main.c
- main/wifi_manager.c
- main/wifi_manager.h
- docs/design.md

## Acceptance
- `idf.py build` passes.
- On the board with connection delayed, the awake screen shows Connecting and Start is visibly unavailable.
- A touch or queued command before connection creates no run ID, active WAV, segment reservation, or recording state.
- The retained log is I3-safe and the result is labeled developmental physical evidence.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
