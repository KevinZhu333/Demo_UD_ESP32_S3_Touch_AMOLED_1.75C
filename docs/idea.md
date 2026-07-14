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

The agreed segment duration is **10 seconds**. At the current audio format, each segment is roughly 640 KB—small enough for fast feedback and several locally queued segments.

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
  → Done
```

The Waveshare tutorial’s event-driven initialization model applies, but its worked example creates a SoftAP. Our device should use **STA/client mode** to join an existing Wi‑Fi network, which the current `wifi_manager` already does. [Waveshare Wi‑Fi tutorial](https://docs.waveshare.com/ESP32-ESP-IDF-Tutorials/Wi-Fi)

## Device UI

Keep only the states the user needs:

- `Connecting to Wi‑Fi`
- `Ready`
- `Recording`
- `Recording · uploading segment N`
- `Stopped · uploading N`
- `Complete · N segments uploaded`
- `Wi‑Fi lost · recording locally`
- `Upload error`

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

The broader ideas in [the product mockup](</Users/starspulse/Downloads/demo-kickoff.en(1) (2).html>) remain product context, not this implementation:

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
6. Confirm six complete 10-second WAV files and one final partial WAV arrive at the receiver.
7. Play them in order and confirm they contain the continuous session.
8. Confirm the device reports zero pending uploads.

Final acceptance receiver: `http://192.168.15.195:8000/v1/device/audio-segments` on the same LAN.

The final acceptance run lasts at least two continuous minutes and applies the
same checks. That is the entire proof. No additional architecture should be
added unless it directly makes this loop work.

---

## Decision contract

The following conditions are agreed and binding for this demo. A change to one
of them must be recorded on this whiteboard before implementation changes.

### A. Demo boundary

| ID | Condition | Decision |
| --- | --- | :---: |
| A1 | “Always-on” means continuously recording from Start until Stop—not automatically from boot. | Yes |
| A2 | The proof ends when complete, ordered, playable audio segments reach the cloud. | Yes |
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
| B5 | Recording always has higher CPU, memory, and timing priority than uploading. | Yes |
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
| D1 | The default segment duration is 10 seconds. | Yes |
| D2 | A segment may end in the middle of a word, provided no audio samples are lost and the cloud can join segments in order. | Yes |
| D3 | Segments contain no intentional overlap or duplicate audio. | Yes |
| D4 | Only fully closed WAV files may be uploaded. The active segment is never uploaded. | Yes |
| D5 | Closing one segment and opening the next must not stop I²S microphone capture. | Yes |
| D6 | Pressing Stop closes and uploads the final partial segment, even if shorter than 10 seconds. | Yes |
| D7 | We change to 20 or 30 seconds only if hardware testing proves 10 seconds causes capture gaps, excessive request overhead, or an uploader backlog on normal Wi‑Fi. | Yes |
| D8 | Segment duration remains fixed during a recording session rather than changing dynamically. | Yes |

### E. Wi‑Fi and uploading

| ID | Condition | Decision |
| --- | --- | :---: |
| E1 | The ESP32 uses STA/client mode to join an existing Wi‑Fi network. The Waveshare page’s worked example is SoftAP, but its event-driven model also covers STA operation. [Waveshare Wi‑Fi tutorial](https://docs.waveshare.com/ESP32-ESP-IDF-Tutorials/Wi-Fi) | Yes |
| E2 | The device connects automatically to one preconfigured network at boot. | Yes |
| E3 | Start is enabled only after Wi‑Fi has connected successfully. | Yes |
| E4 | Once recording begins, a brief Wi‑Fi interruption does not stop capture. Closed segments wait locally. | Yes |
| E5 | Uploading one closed segment occurs while the next segment records. | Yes |
| E6 | Use HTTP POST uploads, not live WebSocket streaming. | Yes |
| E7 | Uploads run sequentially in segment order, with one uploader worker. | Yes |
| E8 | The normal network must upload each 10-second segment faster than one new segment is produced. | Yes |
| E9 | Failed uploads retry with a short delay; the original file remains untouched. | Yes |
| E10 | Local HTTP is acceptable during development; a public Internet endpoint must use HTTPS. | Yes |

At the current format, the device creates approximately **640 KB every 10 seconds**, so sustained upload throughput must remain above roughly **64 KB/s**, excluding protocol overhead.

### F. Storage and failures

| ID | Condition | Decision |
| --- | --- | :---: |
| F1 | Closed segments remain on the device until the cloud explicitly acknowledges them. | Yes |
| F2 | Acknowledged local segments are deleted to free storage. | Yes |
| F3 | Pending closed segments survive a reboot and resume uploading after reconnection. | Yes |
| F4 | Recovering an unfinished active segment after sudden power loss is not required for this demo. | Yes |
| F5 | The device never silently overwrites or discards an unuploaded segment. | Yes |
| F6 | If local storage fills, recording stops with a visible error rather than lowering quality or deleting audio. | Yes |
| F7 | We only promise short Wi‑Fi-outage buffering—approximately 60 seconds—not hours of offline recording. | Yes |
| F8 | Stop ends microphone capture immediately, but pending uploads may continue afterward. | Yes |

### G. Device interface

| ID | Condition | Decision |
| --- | --- | :---: |
| G1 | The interface has only Start and Stop controls. | Yes |
| G2 | The existing Play button and device-local playback are removed from this proof. | Yes |
| G3 | The screen clearly shows Wi‑Fi state: Connecting, Connected, or Offline. | Yes |
| G4 | While active, the screen clearly shows Recording. | Yes |
| G5 | The screen shows uploaded and pending segment counts. | Yes |
| G6 | After Stop, it shows “Stopped · uploading N” until the final upload finishes. | Yes |
| G7 | It shows Complete only after every segment has been acknowledged. | Yes |
| G8 | During recording, the display may sleep after the current timeout. Recording and uploading continue uninterrupted. Double-tap wakes the display without starting, stopping, or changing the recording session. | Yes |
| G9 | A second recording session can start after the first session completes. | Yes |

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
| I5 | Pressing Start is explicit consent to record; pressing Stop ends recording completely. | Yes |
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

The K-series contains only measurable demo checks. Multi-hour and all-day
endurance testing is not required before accepting this proof. Once K1-K9 pass
with the required evidence, implementation stops; those are scope and workflow
rules rather than additional acceptance conditions.

| ID | Condition | Decision |
| --- | --- | :---: |
| K1 | The primary proof records for at least two continuous minutes. | Yes |
| K2 | With 10-second segments, all expected segment indexes arrive with no missing or duplicate indexes. | Yes |
| K3 | The summed cloud WAV duration is within 1 second of the device-recorded duration. | Yes |
| K4 | Listening across every segment boundary reveals no obvious gap, corruption, or repeated audio. | Yes |
| K5 | Recording remains active while uploads occur. | Yes |
| K6 | On normal Wi-Fi, pending uploads normally do not exceed two segments and return to zero. | Yes |
| K7 | On normal Wi-Fi, the final partial segment is acknowledged within 15 seconds after Stop. | Yes |
| K8 | A secondary test disconnects Wi‑Fi for 15 seconds, confirms recording continues, then confirms queued uploads clear after reconnection. | Yes |
| K9 | Let the screen sleep during recording, wake it with a double-tap, and confirm it still shows Recording with the correct upload counts. | Yes |

---

## Implementation invariants

1. Audio capture has priority over display, Wi-Fi, and uploading.
2. Closing a segment must not intentionally stop microphone capture.
3. An active/open WAV file is never uploaded.
4. A closed segment is never deleted before a matching cloud acknowledgment.
5. Unacknowledged audio is never silently overwritten or discarded.
6. Segment indexes remain ordered and unique within one recording session.
7. Stop finalizes the current partial segment.
8. Display sleep and wake must not alter the recording session.
9. Every implementation change must map to an agreed decision ID.

## Working agreement

1. Only one implementation step is active at a time.
2. Each code change must cite the decision IDs it implements.
3. A scope change is written here and agreed before code changes.
4. We prefer simplifying or deleting an unused path over introducing another abstraction.
5. A step is complete only after its relevant test or physical-device check passes.
6. Once K1–K9 pass, implementation stops.
