# Cloud Audio Segment Upload Setup

Track B proof runs are **always-on recording with reliable segmented upload**,
not live PCM streaming. The device records local WAV segments, uploads closed
segments in the background, and deletes a local segment only after the server
accepts its checksum and `segment_index`.

## Start The Receiver

```sh
CLOUD_COLLECTION_TOKEN=dev-token \
CLOUD_AUDIO_SEGMENT_DIR=device-audio-segments \
uvicorn cloud.api.device_ws:app --host 0.0.0.0 --port 8000
```

The segment endpoint is:

```text
POST http://<host>:8000/v1/device/audio-segments
```

Configure the firmware with:

- `RUNTIME_CONFIG_CLOUD_AUDIO_SEGMENT_URL=http://<host>:8000/v1/device/audio-segments`
- `RUNTIME_CONFIG_COLLECTION_TOKEN=dev-token`
- Wi-Fi SSID/password
- `AUDIO_SEGMENT_SECONDS=60` by default

`RUNTIME_CONFIG_CLOUD_WSS_URL` is optional control/status plumbing. It is no
longer the audio proof transport.

## Proof Artifacts

Accepted uploads write:

```text
device-audio-segments/<run_id>/segment_000001.wav
device-audio-segments/<run_id>/segment_000001.summary.json
device-audio-segments/<run_id>/summary.json
```

The run summary records segment order, missing segment count, checksum mismatch
count, received duration, device counters, artifact paths, failure reasons, and
`proof_pass`.

Firmware may delete a local segment only after parsing a `SEGMENT_ACK` whose
`run_id`, `segment_index`, and `sha256` match the uploaded file.

Firmware also sends `X-Device-Local-Gap-Count`,
`X-Device-Storage-Overflow-Count`, and `X-Upload-Retry-Count`. Missing local gap
or storage overflow counters fail the run summary; upload retries are recorded
but allowed.

## Acceptance

A proof run passes only when:

- `missing_segment_count = 0`
- `checksum_mismatch_count = 0`
- no device local recording gaps
- no device storage overflow
- device local gap and storage overflow counters are present
- every cloud WAV is playable and clear
- total received duration matches captured duration within documented tolerance
- cloud artifact paths are suitable for later `xiuyu0000/UD_Demo` processing

Upload retries are allowed. Silent overwrite, dropping unacked segments, partial
WAVs, bad checksums, or accepting a gapped run as proof are not allowed.

## Manual Ladder

1. Run a 2-minute smoke test and expect two 60-second segments.
2. Promote to the 5-minute proof only if the smoke artifacts are complete,
   ordered, playable, and clear.
3. Promote to 30-minute and 2-hour soaks only after the 5-minute run passes.
4. Promote to 8-10 hour endurance only after the 2-hour run passes.

Raw audio is demo debt. Keep generated WAVs out of git, delete them when no
longer needed, and do not log collection tokens or raw PCM.
