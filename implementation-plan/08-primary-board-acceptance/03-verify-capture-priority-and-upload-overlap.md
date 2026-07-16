# Verify capture priority and upload overlap

Status: Planned
Requirements: B5, E5, J4, K5
Depends on: 08/02

## Outcome
The primary run proves capture stayed the highest-priority application workload and remained Recording while a same-run segment uploaded and received its matching ACK under concurrent display, rotation, and network load.

## Current gap
Source-level task priorities and a short smoke do not establish the complete B5 live priority/startup/failure/counter set or K5 upload overlap on the frozen acceptance candidate.

## In scope
- Inspect the live task list and startup log to prove capture priority is strictly above every application-controlled display, command, network-orchestration, and upload task, and that I²S/recorder/buffer/task resources initialize before uploader work starts.
- Under concurrent display, segment rotation, and upload load, verify no I²S read, recorder write, or capture-resource allocation failure and terminal same-run `local_gap_count == 0` and `storage_overflow_count == 0`.
- Before Stop, pair at least one same-run full type/run/index/SHA ACK with evidence that both recorder and awake/asleep UI lifecycle remained Recording, with no Stop or error transition; reserve boundary listening conclusions for `08/07`.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Claiming K4 passage, using an ACK after Stop for K5, or treating ESP-IDF internal Wi-Fi/system task priority as application-controlled priority.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/wifi_manager.c
- docs/design.md

## Acceptance
- Every B5 live priority, initialization, failure, and terminal-counter condition passes on the primary run.
- At least one full matching ACK occurs before Stop while capture and UI state are Recording under concurrent load, satisfying the K5 measurement without relying on post-Stop traffic.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
