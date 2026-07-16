# ESP32 Audio Proof-of-Concept Implementation Plan

This directory is the dependency-ordered execution plan for the frozen
Start-to-Stop audio proof of concept. It is a planning artifact, not evidence
that any behavior is implemented or verified. Execute exactly one task at a
time, and do not start a task until its declared predecessor has passed.

The governing authority remains, in order, [`docs/idea.md`](../docs/idea.md),
[`docs/spec.md`](../docs/spec.md), [`docs/design.md`](../docs/design.md), and
[`AGENTS.md`](../AGENTS.md). If `idea.md` and `spec.md` disagree, stop and
reconcile them before implementation. A task may not change the frozen
contract merely to make current behavior pass.

## Initial artifact

The initial artifact contains exactly 86 Markdown files: this README, nine
phase READMEs, and 76 canonical task files. The phase task counts are
`10, 5, 6, 7, 9, 10, 7, 11, 11`.

| Phase | Tasks | Plan |
| --- | ---: | --- |
| 01 — Baseline and safety | 10 | [Open phase](01-baseline-and-safety/README.md) |
| 02 — Configuration and identity | 5 | [Open phase](02-configuration-and-identity/README.md) |
| 03 — Wi-Fi and Start gating | 6 | [Open phase](03-wifi-and-start-gating/README.md) |
| 04 — Recording status and counts | 7 | [Open phase](04-recording-status-and-counts/README.md) |
| 05 — Stop, Complete, and second session | 9 | [Open phase](05-stop-complete-and-second-session/README.md) |
| 06 — Storage, reboot, and failures | 10 | [Open phase](06-storage-reboot-and-failures/README.md) |
| 07 — Integrated software verification | 7 | [Open phase](07-integrated-software-verification/README.md) |
| 08 — Primary board acceptance | 11 | [Open phase](08-primary-board-acceptance/README.md) |
| 09 — Outage and final audit | 11 | [Open phase](09-outage-and-final-audit/README.md) |

## Exact task template

Every canonical or corrective task uses this template without changing the
heading names or order. The following template is reproduced verbatim:

```markdown
# [Short, imperative title]

Status: Planned
Requirements: [Applicable IDs like E3, G3]
Depends on: [Previous task ID, e.g., 03/01]

## Outcome
[1-2 sentences describing the observable result]

## Current gap
[Brief explanation of the gap based on design.md]

## In scope
- [Concrete action]
- [Concrete action]

## Out of scope
- [What we are NOT doing yet]

## Likely files
- [e.g., main/main.c]
- [e.g., docs/design.md]

## Acceptance
- [e.g., Firmware builds.]
- [e.g., Start remains unavailable before connection.]

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
```

In every instantiated task, `## In scope` also contains this exact bullet:

```markdown
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.
```

In every instantiated task, `## Likely files` contains this exact bullet:

```markdown
- docs/design.md
```

In every instantiated task, `## Acceptance` contains this exact bullet:

```markdown
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.
```

## Canonical requirement registry

Only these 84 requirement tokens may appear in task `Requirements:` fields:

- `A1, A2, A3, A4, A5`
- `B1, B2, B3, B4, B5, B6, B7, B8`
- `C1, C2, C3, C4, C5`
- `D1, D2, D3, D4, D5, D6, D7, D8`
- `E1, E2, E3, E4, E5, E6, E7, E8, E9, E10`
- `F1, F2, F3, F4, F5, F6, F7, F8`
- `G1, G2, G3, G4, G5, G6, G7, G8, G9`
- `H1, H2, H3, H4, H5, H6, H7, H8, H9`
- `I1, I2, I3, I4, I5, I6, I7, I8`
- `J1, J2, J3, J4, J5`
- `K1, K2, K3, K4, K5, K6, K7, K8, K9`

The `Requirements:` field is traceability, not a completion claim. Software
checks do not prove physical behavior, developmental smoke results do not
satisfy final K conditions, and conditional requirements remain governed by
their decision gates.

## Canonical chain

- `01/01` depends on `None`.
- Every other canonical task depends on its immediate predecessor.
- `02/01` depends on `01/10`.
- `03/01` depends on `02/05`.
- `04/01` depends on `03/06`.
- `05/01` depends on `04/07`.
- `06/01` depends on `05/09`.
- `07/01` depends on `06/10`.
- `08/01` depends on `07/07`.
- `09/01` depends on `08/11`.

The initial graph therefore has 76 nodes, 75 edges, one root, and no branch or
cycle. A task is blocked until its predecessor passes. Failure blocks all
downstream work except the narrowly allowed corrective or replay procedure
below.

## Corrective bricks before phase 07

A failed task in phases 01–06 may receive one narrowly scoped corrective in
that phase only when the correction remains within the current `idea.md` and
`spec.md` contract.

1. Give it ID `PP/NN-r1` and filename `NN-r1-<slug>.md`.
2. Permit at most one corrective file in a phase.
3. `01/01-r1` depends on `None`. Every other corrective depends on the failed
   task's last passing predecessor.
4. Add the corrective link immediately before the failed task in the phase
   README.
5. Rewrite the failed task to depend on the corrective and set its status to
   `Planned / Rerun required`.
6. Do not run downstream work until the original failed task passes on rerun.
7. If the corrective fails or a second corrective is needed in that phase,
   stop execution and replan.

The corrective is the sole work authorized after the failure. It must itself
use the exact task template, preserve requirement traceability, and leave
`docs/design.md` truthful. Initial task counts remain the canonical counts.
Each valid insertion adds one task and one Markdown file to its phase; with
`c` affected phases, where `0 <= c <= 6`, post-correction totals are `76 + c`
task files and `86 + c` Markdown files. Each affected phase has exactly one
additional linked task. The canonical 76-node chain remains identifiable,
while each corrective becomes the single inserted predecessor of its rerun
task.

For every phase-01 failure, a corrective may gather diagnostics or correct
only a measured DMA implementation. It may not introduce D7, change segment
duration, add a PCM task or queue, or select another architecture. An
inconclusive rotation result permits one diagnostic corrective and rerun.
Persistent gaps, failed DMA allocation, or failed smoke or listening results
stop execution for the `idea.md` to `spec.md` decision process.

## Replay after phase 07 begins

No firmware or runtime-configuration corrective is permitted after `07/01`
begins. Any such correction stops execution, requires a revised plan, and
requires fresh execution beginning at `07/01`, followed by complete new phase
08 and phase 09 attempts.

For an evidence or procedure failure that does not change firmware or runtime
configuration:

- A phase-08 failure resets `08/01` through `08/11` to
  `Planned / Replay required`.
- A phase-09 failure resets `09/01` through `09/11` to
  `Planned / Replay required`.
- An analysis-only error resets the earliest affected analysis task and every
  downstream analysis task in that phase.
- Existing dependency fields do not change. Reset status blocks every
  downstream task until its predecessor passes again.
- Increment the evidence-attempt number and archive the failed bundle.
- Never combine data, observations, calculations, or conclusions from
  different attempts.

Resetting status is the dependency-preserving replay mechanism. The earliest
invalidated task and all of its downstream tasks must be reopened; no later
task can retain `Passed` while an earlier predecessor is awaiting replay.

## Evidence freshness

Every physical evidence bundle records:

- Firmware image SHA-256.
- Runtime-configuration fingerprint with secrets excluded.
- Run ID.
- Raw serial-log hash.
- WAV and summary hashes.
- Observation record and listener identity where applicable.

Physical evidence from phases 01–06 is developmental unless its firmware and
runtime-configuration hashes match the phase-07 accepted flag-off build or the
behavior is explicitly recaptured by `07/07`. Verification-mode evidence made
with cleanup-fault or fixture-repair options is always labeled
`Supplemental / Verification build` and proves only the guarded failure path.

Tasks `07/01`, `07/07`, and `09/07` reject stale evidence. Every later firmware
or runtime-configuration change invalidates affected earlier physical
evidence. No superseded bundle may satisfy final A–J or K status. Evidence
files, receiver outputs, storage backups, logs, and PCM remain outside tracked
source in an ignored or external location and contain no credential, token,
authorization value, or SSID.

## Inter-phase teardown

Every phase's final task must:

1. Explicitly Stop any active session.
2. Require the full type/run/index/SHA acknowledgment predicate for every
   finalized segment.
3. Wait for acknowledgment-driven cleanup.
4. Require authoritative empty device inventory.
5. Preserve a sanitized, hash-verified evidence bundle in an ignored or
   external directory.
6. Use a new empty external receiver directory for the next phase.

Receiver evidence is never deleted to obtain a clean next run. The next phase
uses a different `CLOUD_AUDIO_SEGMENT_DIR`. Device cleanup is matching-ACK
driven except for the explicitly authorized disposable fixture procedures in
phase 06. Before phase 05, teardown does not claim visible G7 Complete; it
requires Stop, the full matching ACK predicate, cleanup, and empty inventory.

## Task execution and truthfulness

- Work on one task only; do not bundle later behavior into an earlier task.
- Inspect and preserve unrelated work before every edit.
- Use only the files and behavior needed for the task's cited requirements.
- Keep finalized unacknowledged audio durable and never upload an active WAV.
- Clear errors, increment acknowledgment counts, release reservations, or
  clean artifacts only after `SEGMENT_ACK`, run ID, segment index, and SHA-256
  all match.
- Reconcile `docs/design.md` during every task that changes behavior,
  interfaces, implementation status, evidence status, or a known gap. A
  verification task may update design truth but may not implement behavior
  incidentally.
- Apply every software and physical gate required by `AGENTS.md`. An unrun
  physical gate remains explicitly unverified.
- Stop rather than expanding scope when a task requires a decision not already
  authorized by `idea.md` and `spec.md`.

## Mechanical validation

Initial artifact validation requires:

- Exactly 86 Markdown files.
- Exactly 76 canonical task files.
- Phase counts `10, 5, 6, 7, 9, 10, 7, 11, 11`.
- One 76-node, 75-edge acyclic canonical chain.
- `Status: Planned` in every initial task.
- Exact template heading order.
- The standard per-task `docs/design.md` reconciliation bullets.
- Only canonical requirement tokens.
- All task and README links resolve.
- Every phase README contains its specified entry and exit conditions.
- No file outside `implementation-plan/**/*.md` changes.
- No secret or generated configuration appears.
- Markdown structure and `git diff --check` pass.

Post-correction validation retains the original 76 files as the canonical task
set, permits at most one corrective in each pre-phase-07 phase, verifies every
inserted dependency edge and README position, recalculates totals as `76 + c`
tasks and `86 + c` Markdown files, and applies the replay rules without mixing
attempts.
