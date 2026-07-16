# Verify board audio format and channel usefulness

Status: Planned
Requirements: B1, B2, B3, B4, C1, J4
Depends on: 08/03

## Outcome
Actual primary-run board WAVs are verified as clear, uncompressed 16 kHz, 16-bit, two-channel PCM, and each channel's usefulness is documented without silently authorizing a mono or DSP change.

## Current gap
File headers from the smoke showed the intended format, but permanent board-format acceptance still requires actual-board listening and an explicit channel-usefulness assessment on the frozen candidate.

## In scope
- Inspect every primary WAV header and decoded frame geometry for uncompressed PCM, 16 kHz sample rate, 16-bit samples, two channels, consistent frame alignment, and absence of clipping/corruption indicators.
- Listen to representative speech from each channel separately and together, recording the listener, run ID, method, channel usefulness, clarity, and any raw-audio limitation.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Changing to mono, adding DSP/compression, editing audio, or using this task to decide segment-boundary continuity.

## Likely files
- main/audio_segment_recorder.c
- main/runtime_config.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- All primary WAVs match the B2/C1 format and are decoded consistently, and an actual-board B1/B3 listening record is attached to the same run.
- Both channels have an evidence-backed usefulness result; any proposed mono change remains gated through the required decision process rather than being implemented here.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
