# Add a one-shot post-ACK cleanup fault

Status: Planned
Requirements: F1, F2, G6, G7, G9
Depends on: 05/07

## Outcome
A guarded verification option can fail cleanup exactly once after a full matching ACK and WAV deletion but before sidecar deletion, making the stopped-finalizing interval deterministic.

## Current gap
design.md requires proof of cleanup retry and finalizing, but the repository has no safe deterministic mechanism to hold the post-ACK cleanup window on demand.

## In scope
- Add an obviously nondefault, one-shot verification option that triggers only for a selected synthetic run/index after WAV-first deletion and before canonical sidecar removal.
- Emit structured injection and retry records, retain sidecar and reservation, prohibit repost, and automatically allow the next cleanup retry to succeed.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Enabling the option in the normal or handed-off build.
- Injecting failure before ACK matching, deleting metadata first, or applying the fault more than once.

## Likely files
- main/Kconfig.projbuild
- main/audio_segment_recorder.c
- docs/design.md

## Acceptance
- Firmware builds with the option off and in the isolated verification configuration.
- The flag-off path is byte-for-byte behaviorally unchanged at the injection point.
- The flagged deterministic test proves one full type/run/index/SHA ACK, one POST, WAV-first deletion, retained sidecar/reservation, one injected failure, and a later cleanup retry.
- Verification output is labeled `Supplemental / Verification build`, stored outside tracked source, and contains no credentials.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
