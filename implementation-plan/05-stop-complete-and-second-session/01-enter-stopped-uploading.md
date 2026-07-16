# Enter stopped uploading

Status: Planned
Requirements: D6, F8, G6, I5
Depends on: 04/07

## Outcome
Stop locks the final admission boundary, finalizes any nonempty partial segment, and displays `Stopped · uploading N` while at least one segment awaits a full matching acknowledgment.

## Current gap
design.md says Stop and final-partial upload exist, but the UI currently returns to Ready instead of exposing the required stopped-uploading lifecycle.

## In scope
- Drive stopped-uploading status from the recorder's Stop boundary and authoritative positive pending count.
- Keep capture admission closed while the uploader continues processing finalized segments in order.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Showing finalizing or Complete after the pending count reaches zero.
- Changing the F8 Stop admission boundary.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- docs/design.md

## Acceptance
- Firmware builds.
- A deterministic lifecycle check shows Stop with pending work enters `Stopped · uploading N` and never Ready or Complete.
- The displayed N equals current-run finalized minus type/run/index/SHA-matching acknowledgments.
- A nonempty final partial is finalized exactly once and remains eligible for the sequential uploader.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
