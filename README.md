# ESP32 Device-to-Cloud Audio Demo

Proof of concept for the ESP32-S3-Touch-AMOLED-1.75C.

The demo has one job: after the user presses **Start**, continuously capture
audio into 10-second WAV segments and upload each closed segment over Wi-Fi
while the next segment records. Pressing **Stop** closes and uploads the final
partial segment.

This is intentionally a demo, not a production platform. The agreed scope and
acceptance conditions live in [docs/idea.md](docs/idea.md).

## Firmware

Configure these values with `idf.py menuconfig` under **Project Runtime
Configuration**:

- Wi-Fi SSID and password
- Cloud audio segment URL
- Device ID
- Collection token

The current same-LAN receiver URL is:

```text
http://192.168.15.195:8000/v1/device/audio-segments
```

Build the firmware with ESP-IDF:

```bash
idf.py build
```

## Demo receiver

Install the small Python receiver and run it from the repository root:

```bash
python -m venv .venv-cloud
source .venv-cloud/bin/activate
python -m pip install -r cloud/requirements.txt
export CLOUD_COLLECTION_TOKEN="replace-with-the-device-token"
uvicorn cloud.api.audio_receiver:app --host 0.0.0.0 --port 8000
```

Accepted WAV files and their summaries are written to
`device-audio-segments/` by default. Set `CLOUD_AUDIO_SEGMENT_DIR` to use a
different local directory.

Run the receiver tests with:

```bash
python -m pytest -q cloud/tests
```

## Deliberately absent

There is no WebSocket audio transport, local playback, transcription,
voiceprint processing, dashboard, account system, or production
infrastructure in this proof.
