# ESP32 Device-to-Cloud Audio Demo

## Demo definition

> The ESP32 demo proves that a user can start a recording session, the device can continuously capture audio while closing and uploading short segments over Wi‑Fi, and the process continues until the user presses Stop.

For this demo, “always-on” means **continuous between Start and Stop**. The product mockup describes a future device with no manual start, but that is outside this hardware proof.

## One chosen approach

Use **short WAV segments uploaded over HTTP**, not live WebSocket streaming.

Why:

- Easier to demonstrate and debug.
- A transient upload failure does not stop recording; recording stops with a visible error only if pending audio eventually exhausts local storage under F6.
- The active segment is never uploaded prematurely.
- The previous segment uploads while the next segment records.
- The cloud receives normal, playable WAV files.
- Much of this already exists in the repository.

The agreed full-segment target is **nominally 10 seconds**. Whole admitted
capture buffers are never split: rotation occurs after the first whole buffer
that reaches or exceeds the target. With the current 1,024-frame buffers, a
full segment is 10.048 seconds and 643,072 PCM bytes—small enough for fast
feedback and several locally queued segments.

A segment becomes part of the durable upload set only after its WAV and
metadata are finalized. An unfinished active WAV is outside the durability
promise: after sudden power loss it is not recovered for this demo and is
never uploaded as-is.

The loop is:

```text
Boot
  → connect to Wi‑Fi
  → Ready
  → press Start
  → record segment 1
  → close segment 1 and begin uploading it
  → immediately record segment 2
  → close/upload segment 2 while recording segment 3
  → repeat
  → press Stop
  → close and upload the final partial segment
  → Complete after matching acknowledgments and local cleanup
```

The Waveshare tutorial’s event-driven initialization model applies, but its worked example creates a SoftAP. Our device should use **STA/client mode** to join an existing Wi‑Fi network, which the current `wifi_manager` already does. [Waveshare Wi‑Fi tutorial](https://docs.waveshare.com/ESP32-ESP-IDF-Tutorials/Wi-Fi)

## Device UI

Keep only the states the user needs:

- `Connecting to Wi‑Fi`
- `Ready`
- `Recording`
- `Recording · uploading segment N`
- `Stopped · uploading N`
- `Stopped · finalizing`
- `Complete · N segments uploaded`
- `Wi‑Fi lost · recording locally`
- `Upload error`

`Upload error` is a nonterminal indicator layered on the current recording or
stopped state. It remains visible for the affected pending segment until that
same run and index receives its matching acknowledgment; it does not stop
capture or change Start/Complete gating.

`Complete` means that every finalized segment has received its matching
receiver acknowledgment, acknowledgment-driven local cleanup has succeeded,
and no segment WAV, metadata sidecar, temporary sidecar, or upload/cleanup
reservation remains. Start remains unavailable until that state is reached.

Keep only:

- Start button
- Stop button
- Wi‑Fi status
- Recording indicator
- Uploaded/pending segment count

The Play button and local playback are not part of this proof.

## Keep

- ESP32 board/audio/display support
- Wi‑Fi station connection
- Continuous audio capture
- Short WAV segmentation
- Background HTTP uploader
- Minimal FastAPI receiver
- Delete-after-successful-upload behavior
- Simple retry after brief Wi‑Fi loss

## Remove or disable

- WebSocket live-audio transport
- Legacy `audio_streamer`
- Legacy `offline_buffer`
- Legacy WebSocket stream metrics and tests; retain WAV validation, checksum, ordering, and segment receiver tests
- Local playback/mirror path
- Endurance-test promises
- Multiple competing cloud protocols

## Explicitly not now

The broader product-mockup ideas listed below remain product context, not this implementation:

- Voiceprint detection
- Speech-to-text
- To-do extraction
- Notion/task integration
- Idle detection
- Bluetooth earphones
- Wi‑Fi provisioning screens
- OTA updates
- Production authentication
- Multi-hour offline recording
- Full cloud platform

## Demo acceptance test

The 65-second run is the quick smoke test:

1. Boot the device and connect to Wi‑Fi.
2. Press Start.
3. Record for about 65 seconds.
4. Observe that recording never leaves the Recording state during uploads.
5. Press Stop.
6. Confirm six nominal full WAV files and one final partial WAV arrive at the receiver.
7. Play them in order and confirm they contain the continuous session.
8. Confirm the device reports zero pending uploads.

Final acceptance receiver: `http://192.168.15.195:8000/v1/device/audio-segments` on the same LAN.

Final acceptance uses a primary normal-Wi-Fi run for K1-K7 and K9, a separate
secondary outage run for K8, and any additional targeted evidence required by
the A-J conditions. Neither run alone declares the demo complete. No additional
architecture should be added unless it directly makes this loop work.

---

## Decision contract

The following conditions are agreed and binding for this demo. A change to one
of them must be recorded on this whiteboard before implementation changes.

### A. Demo boundary

| ID | Condition | Decision |
| --- | --- | :---: |
| A1 | “Always-on” means continuously recording from Start until Stop—not automatically from boot. | Yes |
| A2 | The receiver-side scope ends when complete, ordered, playable audio segments arrive at the receiver; receipt alone does not declare the demo complete. Completion still requires applicable evidence for every A-J requirement and physical-board passage of K1-K9. | Yes |
| A3 | Voiceprint, transcription, extraction, Notion, idle detection, and product intelligence are outside this demo. | Yes |
| A4 | We support only this exact ESP32-S3-Touch-AMOLED-1.75C board. | Yes |
| A5 | We simplify the existing implementation instead of creating another architecture. | Yes |

### B. Audio quality

| ID | Condition | Decision |
| --- | --- | :---: |
| B1 | “Best quality” means clear speech without clipping, corruption, dropouts, or boundary gaps—not music-grade hi-fi. | Yes |
| B2 | Start with the current 16 kHz, 16-bit, two-channel PCM capture format. | Yes |
| B3 | We test an actual recording from the board before permanently accepting that format. | Yes |
| B4 | If one microphone channel contains no useful additional audio, we may use mono after testing. | Yes |
| B5 | The application capture task has strictly higher scheduler priority than application-controlled display, command, network-orchestration, and upload tasks. Capture-critical I²S, recorder, buffer, and task allocation completes before upload work begins; under concurrent load, there are no I²S read, recorder write, or capture-resource allocation failures, and terminal `local_gap_count` and `storage_overflow_count` are zero. | Yes |
| B6 | Wi‑Fi activity must not produce audible recording gaps. | Yes |
| B7 | Segment transitions must not discard, repeat, or reorder audio samples. | Yes |
| B8 | We do not add AGC, denoising, filtering, or other DSP unless the raw recording test shows it is necessary. | Yes |

### C. Compression

| ID | Condition | Decision |
| --- | --- | :---: |
| C1 | The first working proof uses uncompressed WAV/PCM. | Yes |
| C2 | Lossless compression is allowed later if it does not interrupt capture and the cloud can decode it. | Yes |
| C3 | Lossy compression is allowed only after listening tests and cloud-processing tests show no meaningful quality loss. | Yes |
| C4 | Compression must happen without delaying or dropping incoming microphone samples. | Yes |
| C5 | We will not add a codec merely to reduce storage before the uncompressed path works. | Yes |

### D. Segmentation

| ID | Condition | Decision |
| --- | --- | :---: |
| D1 | The default full-segment target is nominally 10.000 seconds. Rotation occurs after the first whole admitted capture buffer that reaches or exceeds the target; actual duration is at least 10.000 seconds and less than one capture-buffer duration beyond it, and metadata records the actual duration. | Yes |
| D2 | A segment may end in the middle of a word, provided no audio samples are lost and the cloud can join segments in order. | Yes |
| D3 | Segments contain no intentional overlap or duplicate audio. | Yes |
| D4 | Only finalized WAV files with complete metadata may be uploaded. Durability begins at finalization; an active or otherwise unfinished WAV is never upload-eligible. | Yes |
| D5 | Closing one segment and opening the next must not stop I²S microphone capture. | Yes |
| D6 | Pressing Stop finalizes and queues the final partial segment when it contains admitted PCM, even if shorter than 10 seconds; it contains only buffers admitted under the exact F8 Stop boundary. A zero-data placeholder is not a segment and is removed without entering finalized or pending counts. | Yes |
| D7 | We change to 20 or 30 seconds only if hardware testing proves 10 seconds causes capture gaps, excessive request overhead, or an uploader backlog on normal Wi‑Fi. | Yes |
| D8 | Segment duration remains fixed during a recording session rather than changing dynamically. | Yes |

### E. Wi‑Fi and uploading

For E8 and the normal-network acceptance checks, **normal Wi‑Fi** is a
deterministic condition: the configured fixed same-LAN receiver is continuously reachable,
no network impairment is intentionally induced, and the run starts with zero
pending segment artifacts or reservations.

| ID | Condition | Decision |
| --- | --- | :---: |
| E1 | The ESP32 uses STA/client mode to join an existing Wi‑Fi network. The Waveshare page’s worked example is SoftAP, but its event-driven model also covers STA operation. [Waveshare Wi‑Fi tutorial](https://docs.waveshare.com/ESP32-ESP-IDF-Tutorials/Wi-Fi) | Yes |
| E2 | The device connects automatically to one preconfigured network at boot. | Yes |
| E3 | Start is enabled only after Wi‑Fi has connected successfully. | Yes |
| E4 | Once recording begins, a brief Wi‑Fi interruption does not stop capture. Closed segments wait locally. | Yes |
| E5 | Uploading one closed segment occurs while the next segment records. | Yes |
| E6 | Use HTTP POST uploads, not live WebSocket streaming. | Yes |
| E7 | Uploads run sequentially in segment order, with one uploader worker. | Yes |
| E8 | On normal Wi‑Fi, every full nominal 10-second segment must receive its matching acknowledgment less than 10 seconds after the segment is finalized. | Yes |
| E9 | Failed uploads or acknowledgment attempts retry with a short delay; the original file remains untouched, the affected segment remains pending, and a visible nonterminal `Upload error` indicator remains until its matching acknowledgment arrives. | Yes |
| E10 | Local HTTP is acceptable during development; a public Internet endpoint must use HTTPS. | Yes |

At the current format, the device creates approximately **640 KB every 10 seconds**, so sustained upload throughput must remain above roughly **64 KB/s**, excluding protocol overhead.

### F. Storage and failures

| ID | Condition | Decision |
| --- | --- | :---: |
| F1 | Durability begins only when a segment is finalized. Each finalized segment remains on the device until the receiver returns its matching acknowledgment; deletion occurs only as acknowledgment-driven cleanup, and Complete waits for that cleanup to succeed. | Yes |
| F2 | Acknowledged local segments are deleted to free storage. | Yes |
| F3 | Pending finalized segments survive a reboot and resume uploading after reconnection. | Yes |
| F4 | An unfinished active WAV after sudden power loss is outside the recovery and durability contract. It is never uploaded as-is and may require explicit local cleanup before another session. | Yes |
| F5 | The device never silently overwrites or discards a finalized unuploaded segment. | Yes |
| F6 | If local storage fills, recording stops with a visible error rather than lowering quality or deleting audio. | Yes |
| F7 | The device must buffer at least 60 continuous seconds of Wi‑Fi outage while recording; this is a short-outage promise, not hours of offline recording. | Yes |
| F8 | Stop defines an exact PCM-admission boundary: a buffer already admitted to an in-progress recorder write may finish. Once Stop acquires the recorder/session lock, no later buffer is admitted; a buffer already read from I²S but not yet admitted, and every later buffer, is discarded. Low-level I²S may continue to drain outside the stopped session, but discarded PCM is never persisted or transmitted. Pending finalized uploads may continue afterward. | Yes |

### G. Device interface

| ID | Condition | Decision |
| --- | --- | :---: |
| G1 | The interface has only Start and Stop controls. | Yes |
| G2 | The existing Play button and device-local playback are removed from this proof. | Yes |
| G3 | The screen clearly shows Wi‑Fi state: Connecting, Connected, or Offline. | Yes |
| G4 | While active, the screen clearly shows Recording. | Yes |
| G5 | The screen shows uploaded and pending segment counts. A retryable upload or acknowledgment failure raises the E9 `Upload error` indicator without ending capture or changing Start/Complete gating. | Yes |
| G6 | After Stop, the screen shows `Stopped · uploading N` while any finalized segment awaits a matching acknowledgment. When the pending acknowledgment count is zero but cleanup artifacts or reservations remain, it shows `Stopped · finalizing`; it advances to G7 Complete only after cleanup succeeds. | Yes |
| G7 | It shows Complete only after every finalized segment has received its matching acknowledgment, acknowledgment-driven cleanup has succeeded, and no segment WAV, metadata sidecar, temporary sidecar, or upload/cleanup reservation remains. | Yes |
| G8 | During recording, the display may sleep after the current timeout. Recording and uploading continue uninterrupted. Double-tap wakes the display without starting, stopping, or changing the recording session. | Yes |
| G9 | Start is unavailable until the prior session reaches the G7 Complete state; after that, a second recording session can start. | Yes |

### H. Cloud receiver

| ID | Condition | Decision |
| --- | --- | :---: |
| H1 | The repository includes only a minimal receiver needed to prove device uploads. | Yes |
| H2 | Each Start creates a unique recording/run ID. | Yes |
| H3 | Segments are numbered from 1 and stored under their recording ID. | Yes |
| H4 | The receiver validates the WAV structure and checksum before acknowledging it. | Yes |
| H5 | Duplicate upload attempts must not create duplicate audio artifacts. | Yes |
| H6 | The receiver preserves enough metadata for later processing: format, index, run ID, duration, and checksum. | Yes |
| H7 | The receiver does not need to transcribe, analyze, or concatenate audio during this demo. | Yes |
| H8 | A cloud dashboard, accounts, database, Redis, and production infrastructure are not required. | Yes |
| H9 | The proof supports one device at a time; multi-device scaling is outside scope. | Yes |

### I. Configuration, privacy, and security

| ID | Condition | Decision |
| --- | --- | :---: |
| I1 | Wi‑Fi credentials and the upload endpoint are configured locally through ESP-IDF settings. | Yes |
| I2 | On-device Wi‑Fi selection or provisioning is not required. | Yes |
| I3 | Credentials and tokens must remain ignored by Git and must not appear in logs. | Yes |
| I4 | A simple demo collection token is sufficient; user accounts and production authentication are outside scope. | Yes |
| I5 | Pressing Start is explicit consent to record; pressing Stop ends session recording completely at the exact PCM-admission boundary in F8. Any low-level I²S drain outside that session is discarded and is not recording. | Yes |
| I6 | “Recording” must be clearly visible whenever the display is awake. No new always-on indicator is required while the display sleeps. | Yes |
| I7 | Uploading raw audio to the selected receiver is allowed for this proof. | Yes |
| I8 | Cloud audio retention and deletion can be handled manually during the demo. | Yes |

### J. Hardware and power

| ID | Condition | Decision |
| --- | --- | :---: |
| J1 | The device may remain USB-powered during the demonstration. | Yes |
| J2 | Battery-life optimization is outside this proof. | Yes |
| J3 | Bluetooth headphones and speaker playback are outside this proof. | Yes |
| J4 | We test microphone capture, Wi‑Fi, and concurrent uploading on the physical board—not only with unit tests. | Yes |
| J5 | We do not optimize memory or power unless measurements show they prevent the core loop from working. | Yes |

### K. Acceptance conditions

The K-series contains measurable end-to-end demo checks; it does not replace
the binding A-J conditions. Multi-hour and all-day endurance testing is not
required before accepting this proof. The proof is complete and implementation
stops only after every A-J condition is satisfied with its applicable evidence
and K1-K9 pass with the required physical-board evidence.

| ID | Condition | Decision |
| --- | --- | :---: |
| K1 | The primary proof records for at least two continuous minutes. | Yes |
| K2 | With nominal 10-second segments, let `T` be the terminal `finalized` value in the same-run `session_event=stopped` record and require `T >= 1`. Same-run finalized events and receiver artifacts plus `summary.json` must each contain exactly the indexes `1..T`, with no missing or duplicate index. | Yes |
| K3 | The absolute difference between the summed receiver WAV PCM duration and `session_duration_ms` is at most 1,000 ms. The device measures that monotonic duration from immediately after the first active WAV opens successfully through the Stop timestamp captured immediately after Stop acquires the recorder/session lock and before finalization begins. | Yes |
| K4 | Listening across every segment boundary reveals no obvious gap, corruption, or repeated audio. | Yes |
| K5 | Recording remains active while uploads occur. | Yes |
| K6 | During the normal-Wi‑Fi run, derive the same-run `pending_count` from timestamped finalize and matching-ACK events as `finalized_count - matching_acked_count`; it never exceeds 2 and reaches zero within 15 seconds after Stop. | Yes |
| K7 | On normal Wi-Fi, exercise final-partial timing by pressing Stop after at least one buffer has entered the new segment and before its full boundary. That nonempty final partial segment must receive its matching acknowledgment within 15 seconds after the Stop boundary. | Yes |
| K8 | A secondary run starts with zero pending segment artifacts or reservations. While recording and with the same-run `pending_count` at zero, disconnect Wi‑Fi for 60 continuous seconds and confirm recording continues. After reconnection, `pending_count` must reach zero within 60 seconds while capture continues. | Yes |
| K9 | Let the screen sleep during recording, wake it with a double-tap, and confirm it still shows Recording with the correct upload counts. | Yes |

At the initial format, K8 requires roughly **128 KB/s before protocol
overhead**, about twice the real-time PCM production rate: the uploader must
clear the 60-second backlog while another 60 seconds of audio is finalized.
This catch-up check is intentionally stronger than E8's normal-Wi‑Fi
close-to-acknowledgment requirement.

---

## Implementation invariants

1. Audio capture has priority over display, Wi-Fi, and uploading.
2. Closing a segment must not intentionally stop microphone capture.
3. An active/open or otherwise unfinished WAV file is never uploaded as-is;
   durability begins only when the segment is finalized.
4. A finalized segment is never deleted before a matching cloud acknowledgment,
   and Complete waits for successful acknowledgment-driven cleanup.
5. Unacknowledged finalized audio is never silently overwritten or discarded.
6. Segment indexes remain ordered and unique within one recording session.
7. Stop finalizes the current partial segment after any already-admitted write
   finishes; PCM not admitted before Stop acquires the recorder/session lock is
   discarded and never persisted or transmitted.
8. Display sleep and wake must not alter the recording session.
9. Every implementation change must map to an agreed decision ID.

## Working agreement

1. Only one implementation step is active at a time.
2. Each code change must cite the decision IDs it implements.
3. A scope change is written here and agreed before code changes.
4. We prefer simplifying or deleting an unused path over introducing another abstraction.
5. Software-complete work may be handed off as `Implemented / Unverified` after
   every check that was run passes and every unrun applicable gate is named;
   partial work remains `Partial / Unverified` as applicable. It may be called
   verified complete only after every applicable software and physical check
   passes.
6. Implementation stops only after every A-J condition is satisfied and K1-K9 pass with the required evidence.
