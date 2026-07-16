# Verify distinct same-boot run IDs

Status: Planned
Requirements: H2, H3, H6
Depends on: 02/03

## Outcome
Two sequential accepted Starts in one boot produce distinct run IDs and separate exact receiver namespaces without leaving artifacts between sessions.

## Current gap
Source inspection cannot prove that the new ID generator is invoked for every accepted Start rather than accidentally reusing boot-scoped state.

## In scope
- Record, Stop, full-ACK-clean, and inventory-check a short first session, then perform a second accepted Start in the same boot and record its ID.
- Verify the two IDs differ and that each session's indexes start at 1 under its own receiver directory and metadata identity.
- Stop and full-ACK-clean the second session so no active or durable artifact carries into the reboot task.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Power-cycle uniqueness, idempotent retry testing, or using birthday-probability arguments as physical evidence.

## Likely files
- main/audio_segment_recorder.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Both Starts succeed only from authoritative empty inventory and yield nonempty, byte-distinct run IDs.
- Each receiver namespace contains only its own exact ID, index-1 artifact, and matching metadata; neither overwrites or aliases the other.
- Both sessions are explicitly Stopped, every finalized segment receives a full matching ACK, cleanup succeeds, and inventory is empty.
- Results remain tied to one firmware/configuration fingerprint and one evidence attempt.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
