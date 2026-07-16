# Retain upload error after Stop

Status: Planned
Requirements: E9, G5, G6
Depends on: 05/01

## Outcome
An Upload error raised before or after Stop remains layered on `Stopped · uploading N` until the affected segment receives its full matching acknowledgment.

## Current gap
design.md defines Upload error as orthogonal to lifecycle, but the stopped UI path does not yet preserve the affected failure state or its pending count.

## In scope
- Carry the affected current-run error snapshot from Recording into stopped-uploading status.
- Keep the segment pending and the indicator visible through retries, clearing both only according to their distinct full-ACK and cleanup events.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Treating the retryable error as Complete or as a reason to delete the segment.
- Implementing the post-ACK finalizing display.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- docs/design.md

## Acceptance
- Firmware builds.
- A deterministic stopped-state check keeps `Upload error` visible and pending nonzero after a retryable failure.
- Mismatched ACKs do not clear the error; a type/run/index/SHA-matching ACK clears the affected error and updates counts.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
