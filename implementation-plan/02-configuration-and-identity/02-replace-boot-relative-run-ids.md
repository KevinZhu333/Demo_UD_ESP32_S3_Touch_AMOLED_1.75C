# Replace boot-relative run IDs

Status: Planned
Requirements: H2, H3, H6
Depends on: 02/01

## Outcome
Every accepted Start creates a receiver-safe run ID with 128 bits of hardware-random identity instead of boot-relative time, while one session retains that exact ID for all segments and metadata.

## Current gap
`design.md` states that the current device-ID plus boot-relative timer value can collide across reboot, leaving H2 partial and risking receiver namespace reuse.

## In scope
- Generate a lowercase canonical run ID from fresh ESP hardware randomness only after Start is accepted and before segment 1 opens.
- Keep the encoded ID within the receiver's 95-byte bound and preserve it unchanged in local metadata, HTTP headers, ACK matching, lifecycle records, and receiver summaries.
- Handle randomness or formatting failure by rejecting Start without opening or reserving a segment path.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Adding a database, persistent counter, clock-synchronization service, second identity namespace, or receiver mapping change.

## Likely files
- main/audio_segment_recorder.c
- main/audio_segment_recorder.h
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- `idf.py build` and `python -m pytest -q cloud/tests` pass.
- Static inspection proves one ID is generated per accepted Start, remains stable for the session, and cannot be truncated or regenerated during rotation, retry, or Stop.
- A forced generation/format failure leaves no active WAV, finalized artifact, or reservation.
- Run IDs remain valid for the existing injective receiver directory mapping and do not expose secret material.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
