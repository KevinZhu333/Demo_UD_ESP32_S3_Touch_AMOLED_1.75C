# Instrument capture continuity

Status: Planned
Requirements: B5, B7, D5, K3
Depends on: 01/03

## Outcome
A compact monotonic summary exposes capture-read spacing, rotation stall duration, next-file readiness, allocation/read/write failures, and RX overflow without adding per-buffer logging to the capture path.

## Current gap
`design.md` identifies synchronous rotation in the capture task as the deciding continuity risk, while the prior 964 ms deficit cannot be localized from existing gap counters or lifecycle events.

## In scope
- Measure first-WAV-open, first admitted buffer, rotation begin/end, next-WAV-open, next I2S-read, maximum inter-read interval, and capture failure/overflow totals using one monotonic clock.
- Register or expose the available I2S RX-overflow signal and keep instrumentation updates bounded, allocation-free, and nonblocking in the capture path.
- Emit one structured summary after Stop and stable per-rotation records rather than a log for every PCM buffer.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Changing DMA capacity, segment duration, recorder ownership, task topology, or upload behavior.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- components/bsp_extra/src/bsp_board_extra.c
- docs/design.md

## Acceptance
- `idf.py build` passes with the instrumentation enabled.
- Static inspection confirms capture-path measurement performs no file I/O, dynamic allocation, network operation, or per-buffer log emission.
- The Stop summary uses the same monotonic timebase as lifecycle events and names every required timing, failure, and overflow field.
- Instrumentation status remains diagnostic and does not claim K3 or physical continuity passage.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
