import unittest

from cloud.runtime.audio_frame import build_test_audio_frame, parse_audio_frame
from cloud.runtime.stream_metrics import StreamMetrics


class StreamMetricsTest(unittest.TestCase):
    def test_ordered_frames_with_zero_device_counters_pass(self):
        metrics = StreamMetrics(run_id="run-1", session_id="session-1", device_id="device-1")
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=10, duration_ms=32)))
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=11, duration_ms=32)))
        metrics.record_device_stats(
            {
                "device_audio_loss_count": 0,
                "offline_overflow_count": 0,
                "capture_gap_count": 0,
            }
        )

        summary = metrics.close()

        self.assertEqual(summary["frames_received"], 2)
        self.assertEqual(summary["first_seq"], 10)
        self.assertEqual(summary["last_seq"], 11)
        self.assertEqual(summary["expected_next_seq"], 12)
        self.assertEqual(summary["highest_ack_seq"], 11)
        self.assertEqual(summary["missing_sequence_count"], 0)
        self.assertEqual(summary["duplicate_or_old_sequence_count"], 0)
        self.assertEqual(summary["out_of_order_sequence_count"], 0)
        self.assertEqual(summary["received_audio_seconds"], 0.064)
        self.assertTrue(summary["debug_pass"])

    def test_missing_sequence_fails(self):
        metrics = StreamMetrics(run_id="run-1", session_id="session-1", device_id="device-1")
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=1)))
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=3)))
        metrics.record_device_stats(
            {
                "device_audio_loss_count": 0,
                "offline_overflow_count": 0,
                "capture_gap_count": 0,
            }
        )

        summary = metrics.close()

        self.assertEqual(summary["missing_sequence_count"], 1)
        self.assertFalse(summary["debug_pass"])
        self.assertIn("missing audio sequence", summary["debug_failure_reasons"])

    def test_duplicate_and_old_sequence_fail(self):
        metrics = StreamMetrics(run_id="run-1", session_id="session-1", device_id="device-1")
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=2)))
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=4)))
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=4)))
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=3)))
        metrics.record_device_stats(
            {
                "device_audio_loss_count": 0,
                "offline_overflow_count": 0,
                "capture_gap_count": 0,
            }
        )

        summary = metrics.close()

        self.assertEqual(summary["missing_sequence_count"], 1)
        self.assertEqual(summary["duplicate_or_old_sequence_count"], 2)
        self.assertEqual(summary["out_of_order_sequence_count"], 1)
        self.assertFalse(summary["debug_pass"])

    def test_nonzero_device_counter_fails(self):
        metrics = StreamMetrics(run_id="run-1", session_id="session-1", device_id="device-1")
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=1)))
        metrics.record_device_stats(
            {
                "device_audio_loss_count": 1,
                "offline_overflow_count": 0,
                "capture_gap_count": 0,
            }
        )

        summary = metrics.close()

        self.assertFalse(summary["debug_pass"])
        self.assertIn("device_audio_loss_count is nonzero", summary["debug_failure_reasons"])

    def test_format_change_is_rejected_before_metrics_advance(self):
        metrics = StreamMetrics(run_id="run-1", session_id="session-1", device_id="device-1")
        metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=1, channels=2)))

        with self.assertRaises(ValueError):
            metrics.record_frame(parse_audio_frame(build_test_audio_frame(seq=2, channels=1)))

        summary = metrics.close()
        self.assertEqual(summary["frames_received"], 1)
        self.assertEqual(summary["last_seq"], 1)
        self.assertEqual(summary["expected_next_seq"], 2)


if __name__ == "__main__":
    unittest.main()
