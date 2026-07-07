import tempfile
import unittest
import wave
from pathlib import Path

from cloud.runtime.audio_debug_sink import AudioDebugSink
from cloud.runtime.audio_frame import build_test_audio_frame, parse_audio_frame


class AudioDebugSinkTest(unittest.TestCase):
    def test_enabled_empty_session_closes_without_wav_header_error(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            sink = AudioDebugSink("empty-session", output_dir=tmp_dir, enabled=True)

            sink.close()

            self.assertFalse((Path(tmp_dir) / "empty-session.wav").exists())

    def test_enabled_session_writes_valid_wav_after_first_frame(self):
        payload = b"\x00\x00\x01\x00\x02\x00\x03\x00"

        with tempfile.TemporaryDirectory() as tmp_dir:
            sink = AudioDebugSink("valid-session", output_dir=tmp_dir, enabled=True)
            sink.write_frame(parse_audio_frame(build_test_audio_frame(payload=payload)))
            sink.close()

            wav_path = Path(tmp_dir) / "valid-session.wav"
            self.assertTrue(wav_path.exists())
            with wave.open(str(wav_path), "rb") as wav_file:
                self.assertEqual(wav_file.getnchannels(), 2)
                self.assertEqual(wav_file.getsampwidth(), 2)
                self.assertEqual(wav_file.getframerate(), 16000)
                self.assertEqual(wav_file.readframes(wav_file.getnframes()), payload)

    def test_rejects_format_change_after_first_frame(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            sink = AudioDebugSink("format-change", output_dir=tmp_dir, enabled=True)
            try:
                sink.write_frame(parse_audio_frame(build_test_audio_frame(channels=2)))

                with self.assertRaises(ValueError):
                    sink.write_frame(parse_audio_frame(build_test_audio_frame(channels=1)))
            finally:
                sink.close()


if __name__ == "__main__":
    unittest.main()
