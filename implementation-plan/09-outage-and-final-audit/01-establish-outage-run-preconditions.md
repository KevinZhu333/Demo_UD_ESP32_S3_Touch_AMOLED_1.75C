# Establish outage-run preconditions

Status: Planned
Requirements: E4, F7, K8
Depends on: 08/11

## Outcome
The separate outage attempt begins from verified-empty device and receiver state, sufficient boot-reported usable space, and the unchanged frozen phase-07 firmware/configuration.

## Current gap
K8 is invalid if a backlog or reservation exists before Start, if raw partition size substitutes for usable space, if the primary receiver directory is reused, or if the binary/configuration differs from primary acceptance.

## In scope
- Match firmware image SHA-256 and secret-free runtime-configuration fingerprint to the frozen candidate and verify the immutable primary bundle remains present and unchanged.
- Record boot `esp_spiffs_info` usable free space of at least 5,210,464 bytes and use authoritative inventory to prove no device WAV, canonical/temporary sidecar, pending artifact, ambiguity, or upload/cleanup reservation exists.
- Prove the different `CLOUD_AUDIO_SEGMENT_DIR` is empty, arm sanitized serial/network/evidence capture, and define the monotonic disconnect/reconnect observation method.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Pressing Start, disconnecting Wi-Fi, modifying storage, deleting the primary bundle, or accepting raw partition size as F7 evidence.

## Likely files
- main/audio_segment_recorder.c
- main/wifi_manager.c
- cloud/runtime/audio_segments.py
- docs/design.md

## Acceptance
- Frozen hashes match, boot-reported usable free space is at least 5,210,464 bytes, and authoritative device plus receiver inventories are empty.
- The primary bundle remains hash-identical, and a unique sanitized outage attempt is armed before Start.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
