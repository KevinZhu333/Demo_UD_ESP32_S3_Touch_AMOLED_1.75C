# ESP32 Device-to-Cloud Audio Demo Design

> **Status:** v1 · Partially implemented
> **Date:** 2026-07-14
> **Role:** This document describes how the agreed demo contract in
> [spec.md](spec.md) is or will be implemented. It does not redefine that
> contract. Proposed changes are agreed first in [idea.md](idea.md), promoted
> into the spec, and only then reflected here.

This design is intentionally small. It describes one board, one audio format,
one upload protocol, one uploader worker, and one filesystem-backed receiver.
It is not a production-cloud design.

## Status vocabulary

| Label | Meaning |
| --- | --- |
| **Required design** | Behavior required by the agreed spec, whether or not the current code provides it. |
| **Implemented** | Present in the current source and covered by compilation, static inspection, or receiver tests. |
| **Partial** | Supporting code exists, but the complete required behavior is missing. |
| **Unverified** | Correctness depends on running the physical board and has no recorded acceptance evidence yet. |

## 1. Demo guardrails and principles

1. Audio capture between Start and Stop is the highest-priority behavior.
2. Only a closed WAV segment may cross the network boundary.
3. Upload failures are isolated from capture while local storage remains.
4. An acknowledged segment may be deleted; an unacknowledged closed segment
   may not be silently deleted or overwritten.
5. The simplest path that proves the contract wins. There is no WebSocket
   transport, codec, playback path, database, Redis, account system,
   dashboard, distributed queue, or production platform.
6. Build and unit-test success are software evidence, not physical-demo
   acceptance.

The only supported device is the ESP32-S3-Touch-AMOLED-1.75C. The fixed first
proof format is 16 kHz, 16-bit, two-channel PCM in WAV containers, divided into
nominal 10-second segments. Any future format, codec, DSP, or duration change
must first satisfy the gates in the spec.

## 2. End-to-end architecture

```mermaid
flowchart LR
    U["User: Start / Stop"] --> UI["Device UI state"]
    MIC["Board microphone + I2S"] --> CAP["High-priority capture task"]
    UI --> CAP
    CAP --> ACTIVE["Active WAV on SPIFFS"]
    ACTIVE -->|"close header + metadata"| CLOSED["Durable closed segment"]
    CLOSED --> Q["Sequential uploader"]
    Q -->|"HTTP POST over Wi-Fi"| API["Minimal FastAPI receiver"]
    API --> VAL["Token, checksum, WAV, metadata validation"]
    VAL --> STORE["Run directory on local filesystem"]
    STORE -->|"matching SEGMENT_ACK"| Q
    Q -->|"delete WAV + metadata"| CLOSED
```

While the uploader handles segment *N*, the capture task writes segment
*N + 1*. The active file is never put on the upload queue. The receiver returns
a normal JSON acknowledgment only after it has accepted the complete WAV and
written its artifacts.

### Required topology

| Boundary | Choice |
| --- | --- |
| Device | One ESP32-S3-Touch-AMOLED-1.75C |
| Network | Device joins one preconfigured Wi-Fi network in STA/client mode |
| Transport | Sequential HTTP POST of closed WAV files |
| Demo endpoint | `http://192.168.15.195:8000/v1/device/audio-segments` on the same LAN |
| Receiver | One FastAPI process for one demo device |
| Persistence | SPIFFS on device; ordinary files on receiver |

## 3. Device UI state machine

### Required design

```mermaid
stateDiagram-v2
    [*] --> Connecting
    Connecting --> Ready: Wi-Fi connected
    Connecting --> Connecting: retry
    Ready --> Recording: Start
    Recording --> Recording: close and upload segment
    Recording --> RecordingLocal: Wi-Fi lost or upload delayed
    RecordingLocal --> Recording: Wi-Fi restored
    Recording --> StoppedUploading: Stop and pending > 0
    Recording --> Complete: Stop and pending = 0
    RecordingLocal --> StoppedUploading: Stop
    StoppedUploading --> Complete: all ACKed
    Complete --> Recording: Start next session
    Recording --> StorageError: storage exhausted
    RecordingLocal --> StorageError: storage exhausted
```

The user-facing text is limited to:

- `Connecting to Wi-Fi`, `Ready`, and Wi-Fi `Connected` or `Offline` status;
- `Recording` or `Recording · uploading segment N`;
- `Wi-Fi lost · recording locally` and `Upload error` without ending capture;
- `Stopped · uploading N`; and
- `Complete · N segments uploaded` only when the pending count is zero.

Start is enabled only when Wi-Fi is connected and no previous session has
pending files. Stop is enabled only during capture. Display sleep is a
presentation-only transition: it must not change capture, upload, session, or
button state. During recording, a double-tap wakes the display without issuing
Start or Stop.

### Current implementation

**Partial.** The LVGL interface has only Start and Stop, shows a Recording
duration, and retains the 10-second display sleep plus double-tap wake behavior.
It does not expose Wi-Fi state or uploaded/pending counts. It enters `Ready`
before deferred Wi-Fi startup, enables Start without checking connectivity, and
returns its internal state to `Ready` as soon as Stop finalizes the active
file. It therefore lacks the required `Stopped · uploading N` and `Complete · N
segments uploaded` progression. Uploader failures are logged and counted but
never reported to LVGL, so the required `Upload error` presentation is also
missing.

## 4. Audio capture and segment lifecycle

### Required lifecycle

1. Start creates a unique run ID, resets the run's segment index to 1, and
   opens a WAV with a placeholder 44-byte header.
2. The I2S capture path appends PCM samples without waiting for network work.
3. At the fixed segment boundary, the device rewrites the WAV header, flushes
   and closes the file, persists its upload metadata, and makes it eligible for
   upload.
4. The next segment opens immediately and receives the next sample exactly
   once. Word boundaries do not affect segmentation.
5. Stop ends capture, closes the current partial segment, and leaves all closed
   files eligible for background upload.

### Current implementation

The capture task reads 1,024 stereo PCM frames per I2S call at priority 5. The
segment recorder writes those buffers to SPIFFS and rotates after accumulated
PCM reaches the configured threshold of
`CONFIG_AUDIO_SEGMENT_SECONDS * 64,000` bytes. Since rotation occurs after a
whole I2S buffer is written, a closed segment can be slightly longer than the
nominal boundary; no buffer is intentionally split, repeated, or discarded.
The next segment index is opened after the previous header and metadata are
finalized.

The current local artifacts are:

```text
<SPIFFS mount>/segment_000001.wav
<SPIFFS mount>/segment_000001.meta.json
```

The metadata records run ID, index, nominal start time, byte count, duration,
gap/overflow counters, and retry count. Segment filenames themselves are
index-only and the index resets for each Start.

The lifecycle is **implemented in source**, including final-partial-segment
closure, but boundary continuity, audio quality, exact duration, and concurrent
Wi-Fi behavior remain **unverified on hardware**. Because header rewrite and
rotation currently execute in the capture task, the physical no-gap test is
the deciding evidence.

## 5. Wi-Fi and sequential uploader

### Required design

- ESP-IDF initializes one STA/client interface and automatically joins the
  configured network.
- Recording has priority over networking. A transient disconnect leaves closed
  segments on SPIFFS and does not end the session.
- One worker uploads one segment at a time in index order. Normal Wi-Fi must
  drain a segment faster than the next nominal 640 KB segment is produced.
- A failed request keeps the original WAV and metadata, waits briefly, and
  retries. The active segment is never considered by the worker.
- After reboot, the worker resumes closed segments described by durable
  metadata.

### Current implementation

`wifi_manager` uses `WIFI_MODE_STA` and ESP-IDF Wi-Fi/IP events. Disconnects
schedule reconnect attempts with exponential delays from 1 to 30 seconds. The
application starts networking in a deferred priority-3 task after the UI is
already Ready, so required Start gating and connection presentation are still
missing.

The uploader is one priority-3 task with an eight-entry in-memory queue. It
waits up to 15 seconds for Wi-Fi, uses a 30-second HTTP timeout, and waits 5
seconds after a failed attempt. Queue overflow does not delete the durable
file; when the queue is idle, the worker scans metadata and selects the lowest
segment index. The same scan begins pending uploads after reboot when complete
upload configuration is available.

This retry/persistence path is **implemented**. Throughput, ordering during a
real disconnect, and absence of capture gaps are **unverified**.

## 6. HTTP request and acknowledgment contract

The device sends one request for each fully closed segment:

```http
POST /v1/device/audio-segments HTTP/1.1
Content-Type: audio/wav
X-Device-Id: <configured device id>
X-Run-Id: <run id>
X-Segment-Index: <positive decimal index>
X-Segment-Start-Ms: <device uint64 start offset in decimal milliseconds>
X-Sample-Rate: 16000
X-Channels: 2
X-Bits-Per-Sample: 16
X-Content-Sha256: <lowercase SHA-256 of complete WAV body>
X-Device-Local-Gap-Count: <non-negative decimal count>
X-Device-Storage-Overflow-Count: <non-negative decimal count>
X-Upload-Retry-Count: <non-negative decimal count>
X-Collection-Token: <locally configured demo token>

<complete WAV bytes>
```

All listed headers are part of the device demo contract. The receiver parser
allows the three device counters to be absent for diagnostic compatibility,
but a run with missing proof counters does not pass its summary. The collection
token is operationally required.

The receiver accepts the request only after it verifies:

- the collection token;
- a positive segment index and non-negative supplied counters;
- SHA-256 equality;
- a readable, non-empty PCM WAV; and
- equality between WAV and header sample rate, channel count, and bit depth.

Device requests always send a non-negative start offset because the firmware
stores it as `uint64_t`. The receiver currently parses that header as a Python
integer but does not separately reject a negative value from another client;
that validation edge remains a small receiver gap.

A successful or same-checksum duplicate request returns HTTP 2xx with a JSON
object containing at least:

```json
{
  "type": "SEGMENT_ACK",
  "run_id": "<same run id>",
  "segment_index": 1,
  "sha256": "<same WAV SHA-256>"
}
```

The device treats the response as an acknowledgment only when all four fields
match the upload. It then removes both the local WAV and its metadata. A
non-2xx response, missing body, malformed JSON, or mismatch is a retryable
failure and leaves the originals intact.

The receiver returns 401 for a missing token, 403 for a configured-token
mismatch, 400 for invalid checksum/WAV/metadata consistency, 409 when the same
run/index already exists with different audio, and validation errors for
invalid headers. A retry with the same run, index, and checksum is idempotent.

## 7. Minimal FastAPI receiver

The FastAPI application contains one relevant route and writes ordinary files.
With the default output directory, a run is laid out as:

```text
device-audio-segments/
└── <sanitized-run-id>/
    ├── segment_000001.wav
    ├── segment_000001.summary.json
    ├── segment_000002.wav
    ├── segment_000002.summary.json
    ├── summary.json
    └── .summary_state.json  (created only after a checksum conflict)
```

`CLOUD_AUDIO_SEGMENT_DIR` changes the root. Unsupported characters in a run ID
are replaced with `_` for the directory name; the original run ID remains in
JSON metadata. Per-segment summaries preserve identity, format, index, nominal
start, duration, checksum, sizes, and device counters. `summary.json` derives
received indexes, missing/duplicate/checksum counts, total received duration,
counter maxima, and proof-failure reasons. When a conflicting duplicate is
seen, `.summary_state.json` is created as the small internal
checksum-conflict counter; it is absent from an ordinary successful run.

The receiver does not concatenate, play, transcribe, analyze, retain by policy,
or expose a dashboard. Manual inspection and deletion are sufficient for this
demo.

## 8. Failure, retry, reboot, and storage behavior

| Event | Required behavior | Current status |
| --- | --- | --- |
| Brief Wi-Fi loss | Keep recording; retain and later drain closed segments. | Retry path implemented; physical K8 test unverified. |
| HTTP or ACK failure | Preserve WAV/metadata and retry sequentially. | Implemented with a 5-second retry delay. |
| Upload queue full | Preserve closed data for filesystem scan. | Implemented; queue-overflow counter is logged. |
| Reboot with closed segments | Resume from persisted metadata after Wi-Fi returns. | **Partial.** Metadata scanning exists, but physical reboot is unverified and the current mount may format SPIFFS after a mount failure. |
| Power loss with active segment | Recovery is not required for this demo. | Active WAV has no finalized metadata and is not uploaded automatically. |
| Storage reserve exhausted | Stop capture and show a visible error; never reduce quality or delete audio. | **Partial.** The 64 KB reserve and visible `Storage error` exist, but the current abort path removes the unfinished active WAV; physical behavior is unverified. |
| Stop with pending files | End microphone capture immediately; continue uploads; show pending until Complete. | Capture stop/upload continuation implemented; UI progression missing. |

### Session-safety gap

Local filenames use only the segment index and reset to 1 for every run. The
recorder refuses to open an existing WAV, preventing a direct silent overwrite,
but the UI currently permits a new Start before older uploads reach zero.
Concurrent deletion and index reuse therefore leave cross-run collision/path
reuse risk and can stop the new run. The required demo-sized fix is to keep
Start disabled until the previous run is Complete; adding a database or a new
persistence service is unnecessary.

This remains the possible pending-file overwrite/path-reuse gap identified for
the demo: the direct WAV guard helps, but WAV and sidecar paths are not scoped
by run ID, so session safety depends on timing and defensive checks rather than
on isolated names. It is not accepted as evidence for F5 or G9.

The current run ID combines the configured device ID with boot-relative timer
time. It distinguishes normal Starts within one boot, but it does not guarantee
uniqueness across reboots. H2 therefore remains partial until the demo uses a
reboot-safe unique value.

An unfinished active WAV after sudden power loss is intentionally outside
recovery scope and may require manual demo reset. Closed, metadata-backed files
are retained by the normal acknowledgment path, but session-level namespace
safety is still partial as described above. In addition, startup currently
mounts SPIFFS with `format_if_mount_failed = true`; a mount failure could erase
pending files, so that behavior must be removed or otherwise resolved before
F3 and F5 can be claimed complete.

## 9. Task priorities and concurrency

| Work | Current task | Priority | Role |
| --- | --- | ---: | --- |
| I2S read and segment write/rotation | `audio_capture` | 5 | Highest application priority; feeds active WAV. |
| Start command and recorder initialization | `app_cmd` | 4 | Keeps storage setup out of the LVGL click callback. |
| Sequential upload | `seg_upload` | 3 | Hashes and sends closed WAVs only. |
| Deferred network startup | `defer_net` | 3 | Starts Wi-Fi and pending-file recovery. |
| Wi-Fi reconnect delay | `wifi_retry` | 3 | Schedules bounded reconnect attempts. |

The tasks are not core-pinned. A recorder mutex protects active-file state and
statistics; the upload worker reads only closed files. Network I/O never runs
inside the I2S capture loop. Capture priority is higher than uploader priority,
but filesystem rotation still occurs in the capture task, so scheduling alone
does not prove gap-free audio.

## 10. Configuration and secret boundaries

Device values come from ESP-IDF **Project Runtime Configuration** and are kept
in local, Git-ignored `sdkconfig`:

| Kconfig symbol | Purpose |
| --- | --- |
| `CONFIG_WIFI_MANAGER_SSID` | One station SSID |
| `CONFIG_WIFI_MANAGER_PASSWORD` | Station password |
| `CONFIG_RUNTIME_CONFIG_CLOUD_AUDIO_SEGMENT_URL` | Segment POST URL |
| `CONFIG_RUNTIME_CONFIG_DEVICE_ID` | Non-secret device identity |
| `CONFIG_RUNTIME_CONFIG_COLLECTION_TOKEN` | Shared demo collection token |
| `CONFIG_AUDIO_SEGMENT_SECONDS` | Fixed session segment duration; agreed default 10 |

Upload is enabled only when SSID, password, endpoint, and token are all present.
The current same-LAN URL is configuration, not a second protocol. Public
Internet use would require HTTPS and is outside this same-LAN acceptance setup.

The receiver reads `CLOUD_COLLECTION_TOKEN` and optional
`CLOUD_AUDIO_SEGMENT_DIR` environment variables. No real password or token
value belongs in source, tracked defaults, documentation, test output, or
logs. Logs may contain non-secret device/run IDs, segment indexes, counters,
and artifact paths.

**Current gap:** `wifi_manager` logs the configured SSID after station startup.
Because the spec treats Wi-Fi credentials as non-loggable, that value must be
removed or redacted before I3 is complete.

## 11. Verification strategy

### Software gates

| Scope | Gate | What it establishes |
| --- | --- | --- |
| Documentation | Relative links/Markdown inspection and `git diff --check` | Document integrity only. |
| Receiver | `python -m pytest -q cloud/tests` | Valid upload, ACK, artifact creation, duplicate handling, gaps, counters, checksum/WAV rejection, and token enforcement. |
| Firmware | `idf.py build` in an activated ESP-IDF environment | Source/configuration compile; no hardware behavior. |

### Physical proof

The quick smoke run records about 65 seconds and should produce six full
segments plus one partial. Final acceptance records at least two minutes and
executes every K-series condition, including listening across every boundary,
duration comparison, live queue observation, final-ACK timing, a 15-second
Wi-Fi interruption, and sleep/double-tap wake. Evidence should include the run
ID, receiver `summary.json`, ordered WAV files, observed device counts, timing,
and listening result.

No software-only result can mark the demo Complete.

## 12. Current implementation status and known gaps

| Area | Status | Evidence or gap |
| --- | --- | --- |
| Board, display, microphone/I2S, SPIFFS initialization | **Implemented / Unverified** | Board-specific source exists and firmware compiles; capture quality requires the board. |
| PCM WAV creation and 10-second rotation | **Implemented / Unverified** | Header, persistence, rotation, and final partial exist; boundary quality is untested physically. |
| STA Wi-Fi and reconnect | **Implemented / Partial** | Event-driven connection exists; UI starts Ready before connection and Start is not gated. |
| Sequential upload, retry, durable closed files | **Implemented / Unverified** | One worker, metadata scan, ACK matching, and delete-after-ACK exist; live Wi-Fi behavior is untested. |
| Minimal receiver | **Implemented** | Route, validation, idempotence, artifacts, summaries, and receiver tests exist. |
| Start/Stop-only interface | **Implemented** | No Play or local playback path remains. |
| Wi-Fi status and upload counts | **Partial** | Recorder counters exist, but the UI does not show Wi-Fi, uploaded, or pending values. |
| Upload error status | **Partial** | The uploader retries and counts failures, but the UI receives no failure status. |
| Post-Stop progression | **Partial** | Capture stops and uploads continue, but the UI returns to Ready without `Stopped · uploading N` or Complete. |
| Second-session safety and run identity | **Partial** | Start is re-enabled before pending files clear; index-only paths can collide, and the boot-relative run ID is not guaranteed unique across reboot. |
| Configuration and secret handling | **Partial** | Local `sdkconfig` is ignored and token/password values are not logged, but the configured SSID is currently logged. |
| Display sleep and double-tap wake | **Implemented / Unverified** | Code preserves session state, but K9 has no physical evidence. |
| K1–K9 acceptance set | **Unverified** | No complete physical-board evidence set is recorded; the demo must not be declared complete. |

## 13. Key tradeoffs

| Choice | Demo benefit | Accepted cost |
| --- | --- | --- |
| HTTP files instead of WebSockets | Playable artifacts, simple retries and debugging | Roughly one request every 10 seconds, not sample-level live latency |
| Uncompressed WAV/PCM | No codec risk and universal cloud readability | About 640 KB per nominal segment |
| 10-second fixed segments | Frequent feedback and manageable outage queue | May cut through words; request overhead is higher than longer segments |
| One sequential uploader | Deterministic ordering and simple ownership | No parallel upload acceleration |
| SPIFFS plus metadata sidecars | Reboot-visible queue without another service | Short buffering only; index namespace needs session gating |
| Filesystem FastAPI receiver | Easy inspection and playback | One-device demo only; no production retention or scaling |
| Shared demo token | Minimal access boundary | Not user authentication or production security |

## Appendix A. Spec-to-design traceability

The spec owns the requirement text. This appendix maps every agreed ID to its
primary design section and current evidence; it does not create new
requirements.

| IDs | Primary design location | Current status |
| --- | --- | --- |
| A1, A2 | Sections 2, 3, 11 | Required flow implemented in part; end-to-end proof unverified |
| A3, A5 | Sections 1, 13 | Implemented as scope guardrails |
| A4 | Sections 1, 2, 11 | Board support implemented; physical proof unverified |
| B1, B3, B4 | Sections 4, 11 | Unverified listening/format decisions |
| B2 | Sections 1, 4, 6 | Implemented 16 kHz, PCM16, two-channel path |
| B5 | Section 9 | Implemented priority ordering |
| B6, B7 | Sections 4, 9, 11 | Unverified on hardware |
| B8 | Sections 1, 13 | Implemented by absence of DSP |
| C1, C5 | Sections 1, 4, 13 | Implemented uncompressed first proof |
| C2, C3, C4 | Sections 1, 13 | Future gates only; no codec is present |
| D1, D8 | Sections 4, 10 | Implemented fixed configured duration |
| D2, D3, D5, D7 | Sections 4, 11 | Lifecycle exists; boundary and duration decision unverified |
| D4, D6 | Sections 4, 6 | Implemented closed-only upload and final partial |
| E1, E2 | Sections 5, 10 | Implemented STA auto-connect configuration |
| E3 | Sections 3, 12 | Partial; Start gating is missing |
| E4, E5, E7, E9 | Sections 5, 8, 9 | Implemented path; concurrent hardware proof unverified |
| E6 | Sections 2, 6 | Implemented HTTP POST only |
| E8 | Sections 5, 11 | Unverified throughput requirement |
| E10 | Sections 2, 10 | Same-LAN HTTP implemented; public use requires HTTPS |
| F1, F2 | Sections 6, 8 | Implemented matching-ACK retention/deletion |
| F3 | Sections 5, 8 | Partial; metadata scan exists, but reboot is unverified and mount failure may format storage |
| F4 | Section 8 | Intentional demo limitation |
| F5 | Sections 8, 12 | Partial; overwrite refusal exists, but cross-run namespace and mount-failure safety do not |
| F6 | Sections 8, 12 | Partial; visible error exists, but abort removes the unfinished active WAV |
| F7 | Sections 1, 13 | Required short-buffer limit; no long-offline claim |
| F8 | Sections 3, 8 | Capture stop/upload continuation implemented; UI partial |
| G1, G2 | Sections 3, 12 | Implemented Start/Stop only; playback absent |
| G3, G5, G6, G7 | Sections 3, 12 | Partial; required status/count states missing |
| G4, G8 | Sections 3, 12 | Implemented in code; display behavior unverified physically |
| G9 | Sections 3, 8, 12 | Partial; zero-pending second-session gate missing |
| H1, H7, H8, H9 | Sections 1, 7 | Implemented minimal one-device receiver scope |
| H2, H3 | Sections 4, 7, 8 | Run/index metadata implemented; reboot-safe identity and second-run isolation partial |
| H4, H5, H6 | Sections 6, 7, 11 | Implemented and covered by receiver tests |
| I1, I2 | Section 10 | Implemented local ESP-IDF configuration; no provisioning UI |
| I3, I4 | Section 10 | Partial; demo-token boundary exists, but the configured SSID is logged |
| I5, I6 | Section 3 | Start/Stop and awake Recording indicator implemented; physical behavior unverified |
| I7, I8 | Sections 7, 10 | Implemented raw-audio/manual-retention demo boundary |
| J1, J2, J3, J5 | Sections 1, 13 | Fixed scope constraints; no battery/Bluetooth optimization |
| J4 | Section 11 | Unverified; physical board is mandatory |
| K1, K2, K3, K4 | Section 11 | Unverified primary-run evidence |
| K5, K6, K7, K8 | Sections 5, 8, 11 | Unverified concurrency, queue, timing, and outage evidence |
| K9 | Sections 3, 11, 12 | Wake path implemented; physical evidence unverified |
