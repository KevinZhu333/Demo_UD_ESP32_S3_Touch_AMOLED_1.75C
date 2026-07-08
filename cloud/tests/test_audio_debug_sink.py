import json
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
            summary = sink.close()

            wav_path = Path(tmp_dir) / "valid-session.wav"
            summary_path = Path(tmp_dir) / "valid-session.summary.json"
            self.assertTrue(wav_path.exists())
            self.assertTrue(summary_path.exists())
            self.assertEqual(summary["wav_path"], str(wav_path))
            self.assertEqual(summary["summary_path"], str(summary_path))
            self.assertEqual(summary["sample_rate"], 16000)
            self.assertEqual(summary["channels"], 2)
            self.assertEqual(summary["bit_depth"], 16)
            self.assertEqual(summary["frames_received"], 1)
            self.assertEqual(summary["missing_sequence_count"], 0)
            self.assertTrue(summary["debug_pass"])
            self.assertEqual(json.loads(summary_path.read_text(encoding="utf-8")), summary)
            with wave.open(str(wav_path), "rb") as wav_file:
                self.assertEqual(wav_file.getnchannels(), 2)
                self.assertEqual(wav_file.getsampwidth(), 2)
                self.assertEqual(wav_file.getframerate(), 16000)
                self.assertEqual(wav_file.readframes(wav_file.getnframes()), payload)

    def test_default_output_dir_is_project_local_debug_folder(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            original_cwd = Path.cwd()
            try:
                import os

                os.chdir(tmp_dir)
                sink = AudioDebugSink("default-dir", enabled=True)
                self.assertEqual(sink.path, Path.cwd() / "device-audio-debug" / "default-dir.wav")
            finally:
                os.chdir(original_cwd)

    def test_sequence_gap_marks_summary_failed(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            sink = AudioDebugSink("gap-session", output_dir=tmp_dir, enabled=True)
            sink.write_frame(parse_audio_frame(build_test_audio_frame(seq=1)))
            sink.write_frame(parse_audio_frame(build_test_audio_frame(seq=3)))

            summary = sink.close()

            self.assertEqual(summary["missing_sequence_count"], 1)
            self.assertTrue(summary["has_sequence_gaps"])
            self.assertFalse(summary["debug_pass"])
            self.assertEqual(summary["artifact_status"], "failed")

    def test_duplicate_or_old_sequence_marks_summary_failed(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            sink = AudioDebugSink("duplicate-session", output_dir=tmp_dir, enabled=True)
            sink.write_frame(parse_audio_frame(build_test_audio_frame(seq=2)))
            sink.write_frame(parse_audio_frame(build_test_audio_frame(seq=2)))

            summary = sink.close()

            self.assertEqual(summary["duplicate_or_old_sequence_count"], 1)
            self.assertFalse(summary["debug_pass"])

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
