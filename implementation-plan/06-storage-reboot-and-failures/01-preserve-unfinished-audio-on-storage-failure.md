# Preserve unfinished audio on storage failure

Status: Planned
Requirements: F4, F5, F6
Depends on: 05/09

## Outcome
The implemented write- or capacity-failure path stops recording with a visible error while preserving the unfinished active WAV as a classified, nonuploadable F4 orphan instead of silently discarding or promoting it. This brick leaves physical failure behavior Implemented / Unverified for the later guarded board proof.

## Current gap
design.md marks F6 partial because the visible failure exists but the abort path removes the unfinished active WAV, obscuring the evidence and cleanup obligation.

## In scope
- Change the failure transition to close and retain the active path as an explicitly unfinished, nonuploadable orphan while blocking namespace reuse.
- Report the failure and exact orphan identity through authoritative status and inventory without creating metadata or upload eligibility.
- Build the firmware and statically trace every affected failure edge to prove it does not unlink, promote, reserve for upload, or permit reuse of the unfinished path; confirm the live board partition and authoritative inventory remain unchanged and empty.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Promising recovery or upload of the unfinished WAV.
- Inducing a live board storage failure, writing or erasing the board partition, running the near-full fixture, or automatically deleting an orphan.

## Likely files
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- main/main.c
- docs/design.md

## Acceptance
- Firmware builds.
- Static control-flow inspection covers every changed write/capacity-failure edge and shows that admission stops, the visible storage error is published, and the active path is closed without unlink, metadata creation, upload eligibility, or namespace reuse.
- The handoff labels physical F4/F6 behavior Implemented / Unverified and assigns its guarded live proof to `06/10`; it does not present static or build evidence as board evidence.
- No board partition operation or live failure run occurs, and a fresh authoritative board inventory remains identical to the phase-entry empty result for `06/02`.
- No finalized unacknowledged pair is overwritten or deleted by the implemented path.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
