# Audit architecture interface and duration guardrails

Status: Planned
Requirements: A3, A5, B8, C1, C2, C3, C4, C5, D4, D7, D8, E6, E7, E10, F1, F5, G1, G2, H1, H7, H8, H9, I2, I4, I7, I8, J1, J2, J3, J5
Depends on: 07/05

## Outcome
The accepted candidate remains inside the frozen proof: one supported board, raw PCM/WAV, fixed 10-second in-session segmentation, one HTTP POST path, one sequential uploader, SPIFFS, one minimal receiver, and a Start/Stop-only interface with no Play control.

## Current gap
Measurement-driven DMA work and multiple UI/storage bricks could unintentionally introduce an unauthorized queue, duration control, protocol, codec, DSP path, product feature, or persistence architecture unless the integrated result is audited explicitly.

## In scope
- Inspect the implementation and build configuration for each cited scope/architecture guardrail, including upload ordering, active-file exclusion, fixed in-session segment duration under D8, and absence of unauthorized DSP/compression/resource changes.
- Exercise or statically substantiate that duration cannot change during a session and the user interface exposes only Start and Stop for recording, with no Play action under G1 and G2.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Expanding scope, changing a requirement, or accepting a D7/J5 conditional change without its required measured evidence and idea-to-spec decision.

## Likely files
- main/main.c
- main/audio_segment_recorder.c
- main/Kconfig.projbuild
- cloud/api/audio_receiver.py
- docs/design.md

## Acceptance
- Every cited guardrail has concrete code/configuration evidence, D8 remains fixed for a running session, and G1/G2 expose Start/Stop without Play.
- No alternate codec, protocol, queue/worker, storage layer, receiver, or excluded product feature is present.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
