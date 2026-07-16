# Verify, preserve, and close post-reboot identity

Status: Planned
Requirements: H2, H3, H6
Depends on: 02/04

## Outcome
A clean power-cycle followed by a new session proves that run identity remains distinct across reboot, then closes the phase with empty device storage and a preserved identity bundle.

## Current gap
`design.md` specifically leaves H2 partial because boot-relative identity can repeat; same-boot evidence cannot close that reboot risk.

## In scope
- Record the last pre-reboot run ID from the preserved same-boot bundle, perform a normal reboot with empty device inventory, and accept one new Start afterward.
- Verify the post-reboot ID differs from every phase-02 pre-reboot ID and is preserved exactly across its local, transport, ACK, and receiver records.
- Explicitly Stop the final session, require full type/run/index/SHA ACKs, wait for cleanup, require authoritative empty inventory, preserve the sanitized hash-verified phase bundle, and select a different empty receiver root for phase 03.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Rebooting with pending audio, testing filesystem recovery, or claiming visible G7 Complete before phase 05.

## Likely files
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- The post-reboot run ID is nonempty, receiver-safe, and distinct from every captured pre-reboot ID; its segment indexes start at 1 and metadata preserves it exactly.
- The final session is explicitly Stopped, every finalized segment receives its full matching ACK, ACK-driven cleanup completes, and authoritative inventory is empty without relying on a visible Complete state.
- The external identity bundle records firmware SHA-256, secret-free configuration fingerprint, run IDs, raw-log hashes, WAV/summary hashes, and observations.
- A separate phase-03 `CLOUD_AUDIO_SEGMENT_DIR` is active and empty; phase-02 evidence remains preserved rather than deleted.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
