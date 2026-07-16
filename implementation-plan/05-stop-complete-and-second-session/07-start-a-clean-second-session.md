# Start a clean second session

Status: Planned
Requirements: G9, H2, H3, H6
Depends on: 05/06

## Outcome
After authoritative Complete, a second Start opens a new session namespace with a distinct run ID and segment indexing restarted at one.

## Current gap
design.md records recorder safeguards but lacks an end-to-end proof that a cleanup-gated second session starts without colliding with the first.

## In scope
- Exercise the second-Start transition in a deterministic lifecycle test after empty artifact/reservation truth.
- Verify the new run ID is stable within the second session, differs from the first, and is propagated with index and metadata fields.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Leaving a physical session active or an unfinished WAV as this task's handoff state.
- Running the guarded cleanup-failure scenario.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- docs/design.md

## Acceptance
- Firmware builds.
- A deterministic two-session lifecycle test rejects the second Start before Complete and accepts it afterward.
- The accepted second session uses a distinct stable run ID, begins at segment index one, and never reuses or overwrites first-session paths.
- The test fixture closes without leaving an active device session or durable artifact.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
