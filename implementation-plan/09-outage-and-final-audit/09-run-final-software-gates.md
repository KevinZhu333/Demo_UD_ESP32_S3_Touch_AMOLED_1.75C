# Run final software gates

Status: Planned
Requirements: A4, B5, B6, B7, D4, D5, D6, E1, E2, E3, E4, E5, E7, E9, F1, F2, F3, F4, F5, F6, F7, F8, G3, G4, G5, G6, G7, G8, G9, H1, H2, H3, H4, H5, H6, I1, I3, I4, I5, J5
Depends on: 09/08

## Outcome
The unchanged frozen source/configuration passes the receiver and ESP-IDF software gates again, and its generated image/configuration fingerprints remain identical to the accepted physical-evidence candidate.

## Current gap
Final physical results do not replace software gates, and earlier phase-07 results need a final no-drift check before design reconciliation and the stop-work decision.

## In scope
- Run `python -m pytest -q cloud/tests` with receiver dependencies and `idf.py build` in the activated ESP-IDF environment, retaining complete sanitized results.
- Hash the rebuilt firmware image, compare it and the secret-free runtime-configuration fingerprint with the frozen phase-07 values, and inspect the diff for generated configuration or secret material.
- Record all unrun physical checks separately even though phases 08/09 already supply their own preserved evidence.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Correcting a failing test/build, changing firmware/configuration, or using software-gate success to replace board evidence.

## Likely files
- cloud/tests/test_audio_segments.py
- main/main.c
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Receiver tests and firmware build pass, and rebuilt image/configuration fingerprints exactly match the frozen accepted candidate.
- No generated local configuration, credential, or unrelated change appears in the final diff.
- Any failure or fingerprint drift stops execution for replanning rather than modifying the candidate.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
