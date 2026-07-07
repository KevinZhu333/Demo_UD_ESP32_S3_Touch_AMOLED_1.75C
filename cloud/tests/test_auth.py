import os
import unittest

from cloud.runtime.auth import validate_device_hello


def hello(token="dev-token"):
    return {
        "type": "HELLO",
        "device_id": "esp32-test",
        "device_class": "hw",
        "caps": ["audio.pcm16"],
        "collection_token": token,
    }


class DeviceHelloAuthTest(unittest.TestCase):
    def setUp(self):
        self._old_token = os.environ.pop("CLOUD_COLLECTION_TOKEN", None)

    def tearDown(self):
        if self._old_token is not None:
            os.environ["CLOUD_COLLECTION_TOKEN"] = self._old_token
        else:
            os.environ.pop("CLOUD_COLLECTION_TOKEN", None)

    def test_accepts_any_nonempty_token_when_dev_token_unset(self):
        valid, reason = validate_device_hello(hello("anything"))

        self.assertTrue(valid, reason)

    def test_rejects_missing_token(self):
        message = hello("")

        valid, reason = validate_device_hello(message)

        self.assertFalse(valid)
        self.assertIn("collection_token", reason)

    def test_rejects_invalid_token_when_env_token_is_set(self):
        os.environ["CLOUD_COLLECTION_TOKEN"] = "expected"

        valid, reason = validate_device_hello(hello("wrong"))

        self.assertFalse(valid)
        self.assertIn("rejected", reason)

    def test_accepts_expected_env_token(self):
        os.environ["CLOUD_COLLECTION_TOKEN"] = "expected"

        valid, reason = validate_device_hello(hello("expected"))

        self.assertTrue(valid, reason)


if __name__ == "__main__":
    unittest.main()
