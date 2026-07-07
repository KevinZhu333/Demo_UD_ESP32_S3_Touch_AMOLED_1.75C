import unittest

from cloud.runtime.audio_frame import AudioFrameError, build_test_audio_frame, parse_audio_frame


class AudioFrameParserTest(unittest.TestCase):
    def test_parses_valid_task14_frame(self):
        frame = parse_audio_frame(build_test_audio_frame(seq=42, payload=b"\x00\x00\x01\x00"))

        self.assertEqual(frame.seq, 42)
        self.assertEqual(frame.sample_rate, 16000)
        self.assertEqual(frame.channels, 2)
        self.assertEqual(frame.bits_per_sample, 16)
        self.assertEqual(frame.payload_bytes, 4)

    def test_rejects_bad_magic(self):
        data = b"BAD!" + build_test_audio_frame()[4:]

        with self.assertRaises(AudioFrameError):
            parse_audio_frame(data)

    def test_rejects_payload_length_mismatch(self):
        data = build_test_audio_frame(payload=b"\x00\x00")

        with self.assertRaises(AudioFrameError):
            parse_audio_frame(data + b"\x00")


if __name__ == "__main__":
    unittest.main()
