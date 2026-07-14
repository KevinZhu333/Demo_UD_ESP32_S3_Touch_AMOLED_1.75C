# Repository Operating Contract

> **Scope:** ESP32 device-to-cloud audio proof of concept
>
> **Purpose:** Keep implementation work small, traceable, and honest
>
> **Applies to:** Every agent and every repository change

This repository proves one loop: Start, continuously capture audio, close and
upload short WAV segments over Wi-Fi while capture continues, then Stop and
upload the final partial segment. It is a demo, not a production platform.

## 1. Required reading and authority

Before changing implementation or behavior, read:

1. [`docs/idea.md`](docs/idea.md) — collaborative decision staging area.
2. [`docs/spec.md`](docs/spec.md) — agreed, stable demo contract.
3. [`docs/design.md`](docs/design.md) — current technical design and truthful
   implementation status.

Use this authority order:

1. Discuss and agree on a changed decision in `idea.md`.
2. Promote that decision into `spec.md`.
3. Update `design.md` and implementation to satisfy the current spec.
4. Use this file to govern how work is performed; it cannot redefine product
   requirements.

If `idea.md` and `spec.md` disagree, pause implementation until they are
reconciled. Never change the contract merely to make existing code appear
compliant.

## 2. Demo scope guardrails

- Support only the ESP32-S3-Touch-AMOLED-1.75C and the agreed Start-to-Stop
  recording proof.
- Keep one segmented WAV/PCM path, one HTTP POST transport, one sequential
  uploader worker, and one minimal filesystem-backed receiver.
- Prefer simplifying or deleting unused code over adding an abstraction.
- Do not add another protocol, receiver, persistence layer, queueing system,
  database, dashboard, account system, framework, or production
  infrastructure.
- Do not add transcription, voiceprints, task extraction, integrations,
  playback, provisioning, OTA, production authentication, or multi-device
  scaling.
- Add DSP only under B8, compression only under C2-C5, and memory or power
  optimization only under J5. Do not bypass their required evidence.
- Stop adding features when acceptance conditions K1-K9 pass.

## 3. Implementation discipline

1. Work on one concrete implementation step at a time.
2. Before editing, identify the `A1`-`K9` requirement IDs the step serves.
3. Make the smallest direct change that satisfies those IDs; do not create a
   competing architecture.
4. Inspect the working tree and preserve all unrelated user changes. Never
   discard, overwrite, reformat, or include them without explicit direction.
5. Keep recording higher priority than display, Wi-Fi, and upload work.
6. Treat every closed, unacknowledged segment as durable: never silently
   overwrite, discard, or delete it, and never upload an active WAV file.
7. Update `design.md` when behavior, interfaces, implementation status, or
   known gaps change. Update `spec.md` only after the decision process above.
8. Report the IDs addressed, files changed, checks run, and remaining gaps.
   Distinguish compiled or statically tested behavior from physical-board
   evidence.

## 4. Secrets and credentials

- Keep real Wi-Fi credentials and the device collection token only in ignored
  local `sdkconfig` output or environment variables.
- Never place real or live credentials or token values in source, tracked
  defaults, fixtures, documentation, commits, test output, screenshots, or
  logs. Obviously synthetic test placeholders are allowed.
- Do not print request authorization values while debugging.
- Treat device IDs and the agreed LAN receiver URL as non-secret configuration;
  this does not make tokens or Wi-Fi credentials safe to expose.
- Before handing off a change, inspect its diff for accidental secrets and
  generated local configuration.

## 5. Conditional verification gates

Run the checks that match the changed area:

| Changed area | Required gate |
| --- | --- |
| Documentation | Verify relative links, heading structure, tables, code fences, and diagrams; when requirements or traceability change, verify complete `A1`-`K9` coverage; run `git diff --check`. |
| Cloud receiver | In an environment with receiver dependencies installed, run `python -m pytest -q cloud/tests`. |
| Firmware | In an activated ESP-IDF environment, run `idf.py build`. |
| Cross-cutting behavior | Run every applicable software gate above, then perform the relevant physical-board checks. |
| Demo-complete claim | Record physical-board evidence for every acceptance condition `K1`-`K9`. |

A passing build or receiver test is evidence only for that software gate. It is
not evidence that audio is continuous, Wi-Fi concurrency works on the board,
the display behaves correctly, or the demo is complete.

## 6. Completion criteria

A step is complete only when:

- its behavior matches the cited requirement IDs;
- all applicable gates pass, or any unrun gate is explicitly reported;
- documentation describes the actual state without presenting partial or
  unverified behavior as implemented;
- no unrelated user work or secret material is included; and
- remaining limitations and physical verification needs are named clearly.

The demo itself is complete only after physical evidence satisfies every
`K1`-`K9` condition. At that point, stop implementation rather than expanding
the proof.

## 7. Review priorities

Review for, in order:

1. Drift from the agreed spec or missing requirement traceability.
2. Audio loss, gaps, corruption, duplication, or reordering.
3. Overwriting or deleting a segment before a matching acknowledgment.
4. Unsafe second-session filenames, retry/order errors, and storage-failure
   behavior.
5. Credential, token, raw-audio, or generated-file exposure.
6. Claims of implementation or completion without matching evidence.
7. Scope creep and unnecessary production complexity.

Avoid style-only churn. A review should protect the proof, its audio, and its
agreed boundary.
