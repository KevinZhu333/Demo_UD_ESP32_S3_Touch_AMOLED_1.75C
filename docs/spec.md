# ESP32 Device-to-Cloud Audio Demo Specification

> **Document role:** Stable, self-contained contract for what the demo must prove and why.
>
> **Authority:** Agreed decisions are staged in [`idea.md`](idea.md) and become normative when promoted here. [`design.md`](design.md) must describe how this contract is met without redefining it. If the whiteboard and this specification disagree, implementation pauses until they are reconciled.
>
> **Status:** v1 · Agreed demo contract
> **Date:** 2026-07-14
> **Stability:** Change only after an explicit scope or behavior decision is agreed on the whiteboard.

## 1. Background and problem

This proof of concept must show that one ESP32 wearable device can capture speech continuously during a user-controlled session while short, complete audio segments are uploaded over Wi-Fi in the background. Upload work must not interrupt capture, and the receiver must obtain ordered, playable files.

For this demo, **always-on** means continuous recording between an explicit Start and Stop. Automatic recording from boot is future product context, not part of this hardware proof.

## 2. Goals and non-goals

### Goals

- Let a user start and stop one continuous recording session from the device.
- Close 10-second WAV segments without gaps and upload closed segments while capture continues.
- Tolerate a brief Wi-Fi interruption by retaining closed segments locally and retrying them in order.
- Deliver verifiable, playable audio to one minimal receiver.
- Expose only enough device status to make the proof observable.

### Non-goals

- A production audio platform, multi-device service, dashboard, account system, database, Redis, or production operations.
- Live WebSocket audio transport, multiple cloud protocols, or premature codec work.
- Voiceprints, transcription, task extraction, Notion integration, idle detection, or other product intelligence.
- Wi-Fi provisioning UI, OTA updates, production authentication, Bluetooth audio, local playback, or long-duration offline operation.
- Battery optimization, multi-hour endurance claims, or automatic recording from boot.

## 3. Audience and terminology

This specification is for firmware, receiver, test, and review work on the demo.

| Term | Meaning |
| --- | --- |
| Session / run | One recording begun by Start and ended by Stop, with a unique run ID. |
| Active segment | The WAV currently receiving microphone samples; it is not eligible for upload. |
| Closed segment | A finalized WAV with complete metadata that may be queued for upload. |
| Pending segment | A closed segment that has not received a matching receiver acknowledgment. |
| Normal Wi-Fi | The intended same-LAN demo network operating without an intentional outage. |
| Cloud / receiver | The minimal HTTP receiver used to prove delivery; it may run on the same LAN. |
| Complete | Every segment from the stopped session has been acknowledged and no upload remains pending. |

## 4. Scope

### In scope

- The ESP32-S3-Touch-AMOLED-1.75C board, its microphone, display, touch controls, and local storage.
- Wi-Fi station connection to one preconfigured network.
- 16 kHz, 16-bit, two-channel PCM capture as the initial format.
- Uncompressed WAV segmentation, sequential HTTP POST upload, retry, and acknowledgment-driven deletion.
- A minimal FastAPI receiver that validates and stores segments and metadata.
- Physical-board verification of continuous capture, upload concurrency, retry, display sleep, and wake.

### Out of scope

Everything not needed to prove the Start → continuous record → close/upload segment → repeat → Stop loop is excluded. In particular, this contract does not authorize a second transport, a second receiver, a new persistence platform, a cloud dashboard, or production infrastructure.

## 5. Black-box inputs and outputs

### Inputs

- Start and Stop touch actions.
- Microphone PCM samples.
- Wi-Fi connection, loss, and reconnection events.
- Locally configured Wi-Fi credentials, upload endpoint, and demo collection token.
- Display timeout and double-tap wake events.

### Outputs

- Device states visible to the user: `Connecting to Wi-Fi`, `Ready`, `Recording`, `Recording · uploading segment N`, `Stopped · uploading N`, `Complete · N segments uploaded`, `Wi-Fi lost · recording locally`, and `Upload error`.
- Ordered, finalized WAV files sent by HTTP POST.
- Receiver acknowledgments tied to the submitted session and segment.
- Stored WAV files and enough metadata to verify format, order, duration, and checksum.

The final acceptance receiver is:

```text
http://192.168.15.195:8000/v1/device/audio-segments
```

This address is a same-LAN demo endpoint, not a public deployment.

## 6. Testable requirements

Every row in this section is normative.

### 6.1 Demo boundary

| ID | Requirement |
| --- | --- |
| A1 | “Always-on” means continuously recording from Start until Stop, not automatically from boot. |
| A2 | The proof ends when complete, ordered, playable audio segments reach the receiver. |
| A3 | Voiceprint, transcription, extraction, Notion, idle detection, and product intelligence remain outside this demo. |
| A4 | The demo supports only the ESP32-S3-Touch-AMOLED-1.75C board. |
| A5 | Work must simplify the existing implementation rather than create another architecture. |

### 6.2 Audio quality

| ID | Requirement |
| --- | --- |
| B1 | “Best quality” means clear speech without clipping, corruption, dropouts, or boundary gaps, not music-grade hi-fi. |
| B2 | The initial capture format is 16 kHz, 16-bit, two-channel PCM. |
| B3 | An actual board recording must be tested before that format is permanently accepted. |
| B4 | Mono may replace two-channel capture only if board testing shows that one channel adds no useful audio. |
| B5 | Recording has higher CPU, memory, and timing priority than uploading. |
| B6 | Wi-Fi activity must not produce audible recording gaps. |
| B7 | Segment transitions must not discard, repeat, or reorder audio samples. |
| B8 | AGC, denoising, filtering, and other DSP must not be added unless the raw board recording shows a need. |

### 6.3 Compression

| ID | Requirement |
| --- | --- |
| C1 | The first working proof uses uncompressed WAV/PCM. |
| C2 | Lossless compression is allowed later only if it does not interrupt capture and the receiver can decode it. |
| C3 | Lossy compression is allowed only after listening and downstream-processing tests show no meaningful quality loss. |
| C4 | Any compression must not delay or drop incoming microphone samples. |
| C5 | No codec may be added merely to reduce storage before the uncompressed path works. |

### 6.4 Segmentation

| ID | Requirement |
| --- | --- |
| D1 | The default segment duration is 10 seconds. |
| D2 | A segment may end mid-word if no samples are lost and the receiver can join segments in order. |
| D3 | Segments contain no intentional overlap or duplicated audio. |
| D4 | Only fully closed WAV files may be uploaded; the active segment is never uploaded. |
| D5 | Closing one segment and opening the next must not stop I²S microphone capture. |
| D6 | Stop closes and queues the final partial segment for upload even when it is shorter than 10 seconds. |
| D7 | The duration may change to 20 or 30 seconds only if hardware testing proves that 10 seconds causes capture gaps, excessive request overhead, or an uploader backlog on normal Wi-Fi. |
| D8 | Segment duration remains fixed within a recording session rather than changing dynamically. |

### 6.5 Wi-Fi and uploading

| ID | Requirement |
| --- | --- |
| E1 | The device uses STA/client mode to join an existing Wi-Fi network; an event-driven initialization model is appropriate even though the linked [Waveshare example](https://docs.waveshare.com/ESP32-ESP-IDF-Tutorials/Wi-Fi) demonstrates SoftAP. |
| E2 | The device automatically connects to one preconfigured network at boot. |
| E3 | Start is enabled only after Wi-Fi connects successfully. |
| E4 | A brief interruption after recording begins does not stop capture; closed segments wait locally. |
| E5 | One closed segment uploads while the next segment records. |
| E6 | Upload transport is HTTP POST, not live WebSocket streaming. |
| E7 | One uploader worker sends segments sequentially in segment order. |
| E8 | On the normal network, each 10-second segment must upload faster than a new segment is produced. |
| E9 | A failed upload retries after a short delay and leaves the original local file untouched. |
| E10 | Local HTTP is acceptable for development; any public Internet endpoint must use HTTPS. |

At the initial format, the device produces approximately 640 KB per 10 seconds, so normal sustained upload throughput must exceed roughly 64 KB/s before protocol overhead.

### 6.6 Storage and failures

| ID | Requirement |
| --- | --- |
| F1 | Closed segments remain on the device until the receiver explicitly acknowledges them. |
| F2 | Acknowledged local segments are deleted to release storage. |
| F3 | Pending closed segments survive reboot and resume uploading after reconnection. |
| F4 | Recovery of an unfinished active segment after sudden power loss is not required. |
| F5 | The device never silently overwrites or discards an unuploaded segment. |
| F6 | If local storage fills, recording stops with a visible error instead of lowering quality or deleting audio. |
| F7 | The demo promises approximately 60 seconds of Wi-Fi-outage buffering, not hours of offline recording. |
| F8 | Stop ends microphone capture immediately; pending uploads may continue afterward. |

### 6.7 Device interface

| ID | Requirement |
| --- | --- |
| G1 | The interface exposes only Start and Stop controls. |
| G2 | The Play control and device-local playback are excluded. |
| G3 | The screen clearly identifies Wi-Fi as Connecting, Connected, or Offline. |
| G4 | The screen clearly shows Recording while capture is active. |
| G5 | The screen shows uploaded and pending segment counts. |
| G6 | After Stop, the screen shows `Stopped · uploading N` until the final upload finishes. |
| G7 | The screen shows Complete only after every segment is acknowledged. |
| G8 | During recording, the display may sleep after its current timeout while recording and upload continue; double-tap wakes it without starting, stopping, or otherwise changing the session. |
| G9 | A second recording session may start after the first session completes. |

### 6.8 Receiver

| ID | Requirement |
| --- | --- |
| H1 | The repository includes only the minimal receiver needed to prove device uploads. |
| H2 | Each Start creates a unique session/run ID. |
| H3 | Segments are numbered from 1 and stored under their run ID. |
| H4 | The receiver validates WAV structure and checksum before acknowledging a segment. |
| H5 | Duplicate upload attempts must not create duplicate audio artifacts. |
| H6 | The receiver preserves format, index, run ID, duration, and checksum metadata for later processing. |
| H7 | The receiver does not transcribe, analyze, or concatenate audio during this demo. |
| H8 | A dashboard, accounts, database, Redis, and production infrastructure are not required. |
| H9 | The proof supports one device at a time; multi-device scaling is excluded. |

### 6.9 Configuration, privacy, and security

| ID | Requirement |
| --- | --- |
| I1 | Wi-Fi credentials and the upload endpoint are configured locally through ESP-IDF settings. |
| I2 | On-device Wi-Fi selection or provisioning is not required. |
| I3 | Credentials and tokens remain ignored by Git and never appear in logs. |
| I4 | A simple demo collection token is sufficient; user accounts and production authentication are excluded. |
| I5 | Start is explicit consent to record, and Stop ends recording completely. |
| I6 | `Recording` is clearly visible whenever the display is awake; no new always-on indicator is required while it sleeps. |
| I7 | Raw audio may be uploaded to the selected receiver for this proof. |
| I8 | Receiver audio retention and deletion may be handled manually during the demo. |

### 6.10 Hardware and power

| ID | Requirement |
| --- | --- |
| J1 | The device may remain USB-powered during the demonstration. |
| J2 | Battery-life optimization is outside this proof. |
| J3 | Bluetooth headphones and speaker playback are outside this proof. |
| J4 | Microphone capture, Wi-Fi, and concurrent upload must be tested on the physical board, not only through software tests. |
| J5 | Memory and power are optimized only if measurements show they prevent the core loop from working. |

## 7. End-to-end demo journey

1. The device boots, joins the preconfigured Wi-Fi network, and becomes Ready.
2. The user presses Start, creating a unique run and beginning microphone capture.
3. The device records a short WAV, closes it, queues it, and immediately continues capture into the next WAV.
4. The single uploader sends closed files in order while capture continues.
5. A matching acknowledgment makes a local closed file eligible for deletion.
6. If Wi-Fi briefly drops, capture continues and pending files wait locally; upload resumes after reconnection.
7. The user presses Stop, capture ends, and the final partial WAV is closed and queued.
8. The display reports stopped-but-uploading until no segment remains pending, then reports Complete.

A 65-second run is the quick smoke test: it should produce six complete 10-second WAV files and one final partial WAV. The normative acceptance run is longer and is defined below.

## 8. Frozen behavioral invariants

The normative rows in Section 6 and the measurable checks in Section 10 are frozen for v1. A behavior change must first be agreed in the whiteboard and then promoted into this document. Implementation convenience, a successful build, or an existing code path does not override the contract.

## 9. Failure and edge-case behavior

- **Wi-Fi absent at boot:** the device remains visibly unable to start a session until connection succeeds.
- **Transient upload failure:** the uploader retains the closed file, waits briefly, and retries without taking capture out of its active state.
- **Short outage:** pending files accumulate locally and drain in order after reconnection.
- **Storage exhaustion:** capture ends visibly; audio quality is never reduced and older pending audio is never sacrificed to keep recording.
- **Stop between boundaries:** the current WAV is finalized as a shorter file before upload work completes.
- **Repeated request:** the receiver treats an identical retry idempotently and does not create a second artifact.
- **Reboot:** finalized pending files are recoverable; the file that was open at sudden power loss may be abandoned.
- **Display sleep:** capture and upload state continue independently of whether the screen is awake.
- **Next session:** it cannot create naming or storage collisions with pending audio from the prior session.

## 10. Measurable acceptance criteria

Every row below must pass before the proof is declared complete.

| ID | Acceptance criterion |
| --- | --- |
| K1 | The primary proof records for at least two continuous minutes. |
| K2 | With 10-second segments, every expected index arrives with no missing or duplicate indexes. |
| K3 | The summed receiver WAV duration is within 1 second of the device-recorded duration. |
| K4 | Listening across every segment boundary reveals no obvious gap, corruption, or repeated audio. |
| K5 | Recording remains active while uploads occur. |
| K6 | On normal Wi-Fi, pending uploads normally do not exceed two segments and return to zero. |
| K7 | On normal Wi-Fi, the final partial segment is acknowledged within 15 seconds after Stop. |
| K8 | A secondary test disconnects Wi-Fi for 15 seconds, confirms recording continues, and then confirms the queued uploads clear after reconnection. |
| K9 | During recording, let the screen sleep, wake it with a double-tap, and confirm that it still shows Recording with correct upload counts. |

The acceptance set intentionally contains only measurable demo checks.
Multi-hour and all-day endurance testing is not required before accepting this
proof. Once K1-K9 pass with the required evidence, feature work stops and the
device-to-cloud proof is declared complete.

## 11. Fixed constraints and parameters

| Parameter | Contract value |
| --- | --- |
| Hardware | ESP32-S3-Touch-AMOLED-1.75C only |
| Session boundary | Explicit Start through explicit Stop |
| Initial audio | 16 kHz, 16-bit, two-channel PCM in WAV |
| Segment duration | 10 seconds, fixed during a run |
| Transport | Sequential HTTP POST of closed files |
| Upload concurrency | One uploader worker |
| Receiver | One minimal FastAPI service and local artifact storage |
| Network | One preconfigured Wi-Fi network in STA/client mode |
| Offline promise | Approximately 60 seconds |
| Development security | Same-LAN HTTP plus a simple demo collection token |
| Power | USB power is acceptable |

The duration and channel count are adjustable only through the specific board-test conditions defined in the normative requirements. No adaptive behavior is authorized during a session.

## 12. Intentional demo limitations

- The same-LAN URL, simple token, and manual retention are acceptable only for this proof.
- Only one board and one device at a time are supported.
- Local storage buffers a brief outage, not prolonged offline capture.
- Sudden power-loss recovery covers closed files, not the file active at failure.
- There is no provisioning flow, production identity, encryption requirement for local HTTP, database, dashboard, or operational control plane.
- Software tests and compilation reduce implementation risk but do not substitute for the physical-board acceptance run.

These are deliberate scope limits, not invitations to add production architecture.

## 13. Questions answered by the design

[`design.md`](design.md) must answer only the implementation questions needed by this contract:

- Which firmware states and transitions produce the required UI behavior?
- How does capture cross a file boundary without losing samples?
- How are closed files persisted, ordered, retried, acknowledged, and deleted?
- What exact HTTP headers and acknowledgment fields form the device/receiver contract?
- How does the receiver validate a request and lay out its artifacts?
- How are capture, upload, display, and storage work prioritized?
- How does reboot recovery distinguish closed pending files from an unfinished active file?
- Which behaviors are implemented, partial, or still unverified on physical hardware?

The design may choose the smallest mechanism that answers these questions, but it may not expand the product scope or weaken this specification.
