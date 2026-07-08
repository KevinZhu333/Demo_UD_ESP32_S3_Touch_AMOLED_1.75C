# ADR 0004: Track B Uses Segmented Recording Upload

## Status

Accepted.

## Context

Track B's product goal is always-on recording, not low-latency live streaming.
The device must continuously preserve local recording quality, then upload
complete audio artifacts to cloud storage when network and provisioned storage
allow. A failed hardware proof showed that frame-by-frame WebSocket PCM upload
can compete with I2S capture and degrade both duration and clarity.

The failed 2-minute WebSocket proof is accepted as useful design evidence: it
showed that frame-by-frame cloud transport competes with capture and is the
wrong proof transport for the product goal.

## Decision

For Track B proof runs, the primary audio transport is segmented upload:

- record WAV segments locally on device
- close and checksum each segment
- upload the complete segment to `POST /v1/device/audio-segments`
- require cloud ACK of `run_id`, `segment_index`, and checksum before deleting
  the local segment
- treat missing segments, checksum mismatch, storage overflow, or local recording
  gaps as proof failures

Segment indexes are monotonic and proof artifact filenames use
`segment_000001.wav`, `segment_000002.wav`, and so on. `AUDIO_CHUNK` and
`audio_streamer_submit_pcm()` are not proof evidence and must not be called from
the always-on proof recording path.

The WebSocket path may remain for control/status/debug, but it is no longer the
proof transport for audio unless a later ADR explicitly reintroduces a live
audio mode.

## Consequences

Cloud proof artifacts are ordered, playable WAV files suitable for later
`xiuyu0000/UD_Demo` processing. The proof no longer requires every PCM frame to
reach the cloud immediately, and backpressure from upload must not block capture.
Raw audio remains demo debt under the privacy model and must keep token/raw PCM
out of logs.

Local files are deleted only after the cloud returns a `SEGMENT_ACK` matching the
uploaded `run_id`, `segment_index`, and SHA-256 checksum. HTTP success without a
matching ACK is not sufficient proof of receipt.
