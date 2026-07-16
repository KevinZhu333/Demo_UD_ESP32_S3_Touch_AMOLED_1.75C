# Run the receiver software gate

Status: Planned
Requirements: H1, H3, H4, H5, H6, I4
Depends on: 07/02

## Outcome
The minimal receiver passes its complete automated suite for the frozen request interface, ordered identity handling, WAV/checksum validation, exact duplicate behavior, metadata preservation, and configured authentication boundary.

## Current gap
Receiver changes and firmware metadata changes made during earlier bricks need one clean, dependency-installed gate on the integrated candidate before board acceptance begins.

## In scope
- Run `python -m pytest -q cloud/tests` in an environment with the receiver dependencies installed and retain the command, environment identity, and complete result.
- Confirm the tests exercise the existing full request and acknowledgement contract, including `SEGMENT_ACK`, run ID, segment index, and SHA-256 matching, without logging authorization values.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Adding receiver features, changing the HTTP contract, or claiming network or physical-device verification from a software test.

## Likely files
- cloud/api/audio_receiver.py
- cloud/tests/test_audio_segments.py
- docs/design.md

## Acceptance
- `python -m pytest -q cloud/tests` exits successfully with no skipped required receiver behavior.
- The retained result is sanitized, identifies the tested revision, and is described only as receiver software-gate evidence.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
