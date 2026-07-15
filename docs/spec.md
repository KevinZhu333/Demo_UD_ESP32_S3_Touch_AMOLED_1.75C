# ESP32 Device-to-Cloud Audio Demo Specification

> **Document role:** Stable, self-contained contract for what the demo must prove and why.
>
> **Authority:** Agreed decisions are staged in [`idea.md`](idea.md) and become normative when promoted here. [`design.md`](design.md) must describe how this contract is met without redefining it. If the whiteboard and this specification disagree, implementation pauses until they are reconciled.
>
> **Status:** v1 · Agreed demo contract
> **Date:** 2026-07-15
> **Stability:** Change only after an explicit scope or behavior decision is agreed on the whiteboard.

## 1. Background and problem

This proof of concept must show that one ESP32 wearable device can capture speech continuously during a user-controlled session while short, complete audio segments are uploaded over Wi-Fi in the background. Upload work must not interrupt capture, and the receiver must obtain ordered, playable files.

For this demo, **always-on** means continuous recording between an explicit Start and Stop. Automatic recording from boot is future product context, not part of this hardware proof.

## 2. Goals and non-goals

### Goals

- Let a user start and stop one continuous recording session from the device.
- Close nominal 10-second WAV segments without gaps and upload closed segments while capture continues.
- Tolerate at least 60 continuous seconds of Wi-Fi interruption by retaining finalized segments locally and retrying them in order.
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
| Active segment | The WAV currently receiving microphone samples; it is outside the durability promise and is not eligible for upload. |
| Finalized / closed segment | A WAV with complete finalized metadata that may be queued for upload; segment durability begins at this point. |
| Nominal full segment | A segment whose target is 10.000 seconds and whose rotation follows the first whole admitted capture buffer that reaches or exceeds that target; metadata records its actual duration. |
| Pending segment | A finalized segment that has not received a matching receiver acknowledgment. |
| Normal Wi-Fi | A deterministic test condition in which the configured fixed same-LAN receiver is continuously reachable, no network impairment is intentionally induced, and the run starts with zero pending segment artifacts or reservations. |
| Cloud / receiver | The minimal HTTP receiver used to prove delivery; it may run on the same LAN. |
| Complete | Every finalized segment from the stopped session has received its matching acknowledgment, acknowledgment-driven local cleanup has succeeded, and no segment WAV, metadata sidecar, temporary sidecar, or upload/cleanup reservation remains. Start is unavailable until then. |

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

- Device states visible to the user: `Connecting to Wi-Fi`, `Ready`, `Recording`, `Recording · uploading segment N`, `Stopped · uploading N`, `Stopped · finalizing`, `Complete · N segments uploaded`, and `Wi-Fi lost · recording locally`; `Upload error` is a nonterminal indicator layered on the current recording or stopped state until the affected segment receives its matching acknowledgment.
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
| A2 | The receiver-side scope ends when complete, ordered, playable audio segments arrive at the receiver; receipt alone does not declare the demo complete. Completion still requires applicable evidence for every A-J requirement and physical-board passage of K1-K9. |
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
| B5 | The application capture task has strictly higher scheduler priority than application-controlled display, command, network-orchestration, and upload tasks. Capture-critical I²S, recorder, buffer, and task allocation completes before upload work begins; under concurrent load, there are no I²S read, recorder write, or capture-resource allocation failures, and terminal `local_gap_count` and `storage_overflow_count` are zero. |
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
| D1 | The default full-segment target is nominally 10.000 seconds. Rotation occurs after the first whole admitted capture buffer that reaches or exceeds the target; actual duration is at least 10.000 seconds and less than one capture-buffer duration beyond it, and metadata records the actual duration. |
| D2 | A segment may end mid-word if no samples are lost and the receiver can join segments in order. |
| D3 | Segments contain no intentional overlap or duplicated audio. |
| D4 | Only finalized WAV files with complete metadata may be uploaded. Durability begins at finalization; an active or otherwise unfinished WAV is never upload-eligible. |
| D5 | Closing one segment and opening the next must not stop I²S microphone capture. |
| D6 | Stop finalizes and queues the final partial segment when it contains admitted PCM, even when it is shorter than 10 seconds; it contains only buffers admitted under the exact F8 Stop boundary. A zero-data placeholder is not a segment and is removed without entering finalized or pending counts. |
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
| E8 | On normal Wi-Fi, every full nominal 10-second segment must receive its matching acknowledgment less than 10 seconds after the segment is finalized. |
| E9 | Failed uploads or acknowledgment attempts retry with a short delay; the original file remains untouched, the affected segment remains pending, and a visible nonterminal `Upload error` indicator remains until its matching acknowledgment arrives. |
| E10 | Local HTTP is acceptable for development; any public Internet endpoint must use HTTPS. |

At the initial format, the device produces approximately 640 KB per 10 seconds, so normal sustained upload throughput must exceed roughly 64 KB/s before protocol overhead.

### 6.6 Storage and failures

| ID | Requirement |
| --- | --- |
| F1 | Durability begins only when a segment is finalized. Each finalized segment remains on the device until the receiver returns its matching acknowledgment; deletion occurs only as acknowledgment-driven cleanup, and Complete waits for that cleanup to succeed. |
| F2 | Acknowledged local segments are deleted to release storage. |
| F3 | Pending finalized segments survive reboot and resume uploading after reconnection. |
| F4 | An unfinished active WAV after sudden power loss is outside the recovery and durability contract. It is never uploaded as-is and may require explicit local cleanup before another session. |
| F5 | The device never silently overwrites or discards a finalized unuploaded segment. |
| F6 | If local storage fills, recording stops with a visible error instead of lowering quality or deleting audio. |
| F7 | The device must buffer at least 60 continuous seconds of Wi-Fi outage while recording; this is a short-outage promise, not hours of offline recording. |
| F8 | Stop defines an exact PCM-admission boundary: a buffer already admitted to an in-progress recorder write may finish. Once Stop acquires the recorder/session lock, no later buffer is admitted; a buffer already read from I²S but not yet admitted, and every later buffer, is discarded. Low-level I²S may continue to drain outside the stopped session, but discarded PCM is never persisted or transmitted. Pending finalized uploads may continue afterward. |

### 6.7 Device interface

| ID | Requirement |
| --- | --- |
| G1 | The interface exposes only Start and Stop controls. |
| G2 | The Play control and device-local playback are excluded. |
| G3 | The screen clearly identifies Wi-Fi as Connecting, Connected, or Offline. |
| G4 | The screen clearly shows Recording while capture is active. |
| G5 | The screen shows uploaded and pending segment counts. A retryable upload or acknowledgment failure raises the E9 `Upload error` indicator without ending capture or changing Start/Complete gating. |
| G6 | After Stop, the screen shows `Stopped · uploading N` while any finalized segment awaits a matching acknowledgment. When the pending acknowledgment count is zero but cleanup artifacts or reservations remain, it shows `Stopped · finalizing`; it advances to G7 Complete only after cleanup succeeds. |
| G7 | The screen shows Complete only after every finalized segment has received its matching acknowledgment, acknowledgment-driven cleanup has succeeded, and no segment WAV, metadata sidecar, temporary sidecar, or upload/cleanup reservation remains. |
| G8 | During recording, the display may sleep after its current timeout while recording and upload continue; double-tap wakes it without starting, stopping, or otherwise changing the session. |
| G9 | Start is unavailable until the prior session reaches the G7 Complete state; after that, a second recording session may start. |

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
| I5 | Start is explicit consent to record, and Stop ends session recording completely at the exact PCM-admission boundary in F8. Any low-level I²S drain outside that session is discarded and is not recording. |
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
3. The device records a short WAV, finalizes it and its metadata, queues it, and immediately continues capture into the next WAV.
4. The single uploader sends finalized files in order while capture continues.
5. A matching acknowledgment starts local cleanup; Complete remains unavailable until that cleanup succeeds and no segment artifact or reservation remains.
6. If Wi-Fi briefly drops, capture continues and pending files wait locally; upload resumes after reconnection.
7. The user presses Stop; an already-admitted recorder write may finish and later PCM is discarded under F8. If admitted PCM exists in the active WAV, the resulting nonempty final partial is finalized and queued; otherwise the zero-data placeholder is removed under D6.
8. The display reports `Stopped · uploading N` while acknowledgments remain,
   `Stopped · finalizing` after the final matching acknowledgment while cleanup
   remains, and Complete only when no segment artifact or reservation is left.

A 65-second run is the quick smoke test: it should produce six nominal full
WAV files and one final partial WAV. Final acceptance uses a primary
normal-Wi-Fi run for K1-K7 and K9, a separate secondary outage run for K8, and
any additional targeted evidence required by A-J. Neither run alone declares
the demo complete.

## 8. Frozen behavioral invariants

The normative rows in Section 6 and the measurable checks in Section 10 are frozen for v1. A behavior change must first be agreed in the whiteboard and then promoted into this document. Implementation convenience, a successful build, or an existing code path does not override the contract.

## 9. Failure and edge-case behavior

- **Wi-Fi absent at boot:** the device remains visibly unable to start a session until connection succeeds.
- **Transient upload failure:** the uploader retains the closed file, waits briefly, retries without taking capture out of its active state, and keeps the nonterminal `Upload error` indicator visible until the affected segment receives its matching acknowledgment.
- **Short outage:** the device buffers at least 60 continuous seconds while recording; pending files accumulate locally and drain in order after reconnection.
- **Storage exhaustion:** capture ends visibly; audio quality is never reduced and older pending audio is never sacrificed to keep recording.
- **Stop between boundaries:** an already-admitted in-progress recorder write may finish; after Stop acquires the recorder/session lock, read-but-not-admitted and later PCM is discarded and never persisted or transmitted. The admitted PCM is finalized as a shorter WAV before upload work completes.
- **Repeated request:** the receiver treats an identical retry idempotently and does not create a second artifact.
- **Reboot:** finalized pending files are recoverable; the file that was open at sudden power loss is outside recovery, is never uploaded as-is, and may require explicit local cleanup.
- **Display sleep:** capture and upload state continue independently of whether the screen is awake.
- **Acknowledgment cleanup failure:** the screen remains `Stopped · finalizing`; the session is not Complete and Start remains unavailable until cleanup succeeds and no WAV, sidecar, temporary sidecar, or reservation remains.
- **Next session:** Start remains unavailable until the prior session is Complete, so it cannot create naming or storage collisions with prior-session artifacts.

## 10. Measurable acceptance criteria

Every row below must pass, but K1-K9 are not sufficient by themselves. The
proof is complete only when every normative A-J requirement in Section 6 is
also satisfied with its applicable evidence.

| ID | Acceptance criterion |
| --- | --- |
| K1 | The primary proof records for at least two continuous minutes. |
| K2 | With nominal 10-second segments, let `T` be the terminal `finalized` value in the same-run `session_event=stopped` record and require `T >= 1`. Same-run finalized events and receiver artifacts plus `summary.json` must each contain exactly the indexes `1..T`, with no missing or duplicate index. |
| K3 | The absolute difference between the summed receiver WAV PCM duration and `session_duration_ms` is at most 1,000 ms. The device measures that monotonic duration from immediately after the first active WAV opens successfully through the Stop timestamp captured immediately after Stop acquires the recorder/session lock and before finalization begins. |
| K4 | Listening across every segment boundary reveals no obvious gap, corruption, or repeated audio. |
| K5 | Recording remains active while uploads occur. |
| K6 | During the normal-Wi-Fi run, derive the same-run `pending_count` from timestamped finalize and matching-ACK events as `finalized_count - matching_acked_count`; it never exceeds 2 and reaches zero within 15 seconds after Stop. |
| K7 | On normal Wi-Fi, exercise final-partial timing by pressing Stop after at least one buffer has entered the new segment and before its full boundary. That nonempty final partial segment must receive its matching acknowledgment within 15 seconds after the Stop boundary. |
| K8 | A secondary run starts with zero pending segment artifacts or reservations. While recording and with the same-run `pending_count` at zero, disconnect Wi-Fi for 60 continuous seconds and confirm recording continues. After reconnection, `pending_count` must reach zero within 60 seconds while capture continues. |
| K9 | During recording, let the screen sleep, wake it with a double-tap, and confirm that it still shows Recording with correct upload counts. |

At the initial format, K8 requires roughly **128 KB/s before protocol
overhead**, about twice the real-time PCM production rate: the uploader must
clear the 60-second backlog while another 60 seconds of audio is finalized.
This catch-up check is intentionally stronger than E8's normal-Wi-Fi
close-to-acknowledgment requirement.

The acceptance set intentionally contains only measurable end-to-end demo
checks and does not replace the normative requirements in Section 6. Multi-hour
and all-day endurance testing is not required before accepting this proof. Once
every normative A-J requirement is satisfied and K1-K9 pass with the required
physical-board evidence, feature work stops and the device-to-cloud proof is
declared complete.

## 11. Fixed constraints and parameters

| Parameter | Contract value |
| --- | --- |
| Hardware | ESP32-S3-Touch-AMOLED-1.75C only |
| Session boundary | Explicit Start through explicit Stop |
| Initial audio | 16 kHz, 16-bit, two-channel PCM in WAV |
| Segment duration | Nominal 10.000-second target, whole-buffer-aligned and fixed during a run; actual duration is recorded in metadata |
| Transport | Sequential HTTP POST of closed files |
| Upload concurrency | One uploader worker |
| Receiver | One minimal FastAPI service and local artifact storage |
| Network | One preconfigured Wi-Fi network in STA/client mode |
| Offline promise | At least 60 continuous seconds while recording |
| Development security | Same-LAN HTTP plus a simple demo collection token |
| Power | USB power is acceptable |

The duration and channel count are adjustable only through the specific board-test conditions defined in the normative requirements. No adaptive behavior is authorized during a session.

## 12. Intentional demo limitations

- The same-LAN URL, simple token, and manual retention are acceptable only for this proof.
- Only one board and one device at a time are supported.
- Local storage buffers at least 60 continuous seconds of outage while recording, not prolonged offline capture.
- Sudden power-loss recovery covers finalized files, not the file active at failure; an unfinished active WAV is never uploaded as-is.
- There is no provisioning flow, production identity, encryption requirement for local HTTP, database, dashboard, or operational control plane.
- Software tests and compilation reduce implementation risk but do not substitute for the physical-board acceptance run.

These are deliberate scope limits, not invitations to add production architecture.

## 13. Questions answered by the design

[`design.md`](design.md) must answer only the implementation questions needed by this contract:

- Which firmware states and transitions produce the required UI behavior?
- How does capture cross a file boundary without losing samples?
- How are finalized files persisted, ordered, retried, acknowledged, and cleaned up before Complete?
- What exact HTTP headers and acknowledgment fields form the device/receiver contract?
- How does the receiver validate a request and lay out its artifacts?
- How are capture, upload, display, and storage work prioritized?
- How does reboot recovery distinguish finalized pending files from an unfinished active file that must never be uploaded as-is?
- Which behaviors are implemented, partial, or still unverified on physical hardware?

The design may choose the smallest mechanism that answers these questions, but it may not expand the product scope or weaken this specification.
