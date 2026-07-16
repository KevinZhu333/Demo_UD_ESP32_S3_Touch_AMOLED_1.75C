# Verify capture remained active through outage

Status: Planned
Requirements: A1, B5, E4, J4, K8
Depends on: 09/02

## Outcome
The outage event stream proves the same session kept admitting and finalizing audio while Wi-Fi was unavailable, without a lifecycle stop, capture failure, priority inversion, or terminal gap/overflow counter.

## Current gap
A reconnect and eventual upload set do not by themselves prove microphone capture continued throughout the outage; data could have paused, failed, or restarted invisibly.

## In scope
- Correlate the uninterrupted run ID, recording lifecycle, capture/finalize events, and time-coded source across the pre-disconnect, 60-second offline, and post-reconnect intervals.
- Verify capture task priority remains above application display/network/upload work and observe no I²S read, recorder write, or capture-resource allocation failure, with terminal `local_gap_count == 0` and `storage_overflow_count == 0`.
- Confirm no Stop, restart, hidden new session, storage-full transition, or lowered-quality/alternate-format behavior occurs in the K8 measurement window.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Calculating the 60-second catch-up deadline, closing the still-active session, or using this run as primary K4/K7 evidence.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/wifi_manager.c
- docs/design.md

## Acceptance
- One unchanged run ID remains active and continues capture/finalization across the complete measured outage, with no Stop/restart or capture/resource failure.
- Live priority evidence and terminal same-run metadata/summary show the required B5 ordering and zero gap/overflow counters.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
