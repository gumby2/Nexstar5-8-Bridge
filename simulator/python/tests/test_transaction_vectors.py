import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parents[1] / "src"))

from nexstar_sim.transaction import (
    consume_completion,
    consume_handshake,
    consume_payload,
    drain_after_completion,
)


class TransactionVectorTests(unittest.TestCase):
    def test_stale_bytes_before_handshake_are_not_payload(self):
        result = consume_handshake(b"\xff+@#")
        self.assertTrue(result.ok)
        self.assertEqual(result.value, b"#")
        self.assertEqual(result.ignored, b"\xff+@")
        self.assertEqual(result.remaining, b"")


    def test_fixed_payload_does_not_consume_next_transaction(self):
        result = consume_payload(b"\x01\x02\x03\x04#", 4)
        self.assertTrue(result.ok)
        self.assertEqual(result.value, b"\x01\x02\x03\x04")
        self.assertEqual(result.remaining, b"#")


    def test_unexpected_bytes_before_completion_are_recorded(self):
        result = consume_completion(b"A\xff@tail")
        self.assertTrue(result.ok)
        self.assertEqual(result.value, b"@")
        self.assertEqual(result.ignored, b"A\xff")
        self.assertEqual(result.remaining, b"tail")


    def test_missing_handshake_is_bounded_failure(self):
        result = consume_handshake(b"+\xff")
        self.assertFalse(result.ok)
        self.assertEqual(result.ignored, b"+\xff")


    def test_short_payload_is_bounded_failure(self):
        result = consume_payload(b"\x01\x02\x03", 4)
        self.assertFalse(result.ok)
        self.assertEqual(result.ignored, b"\x01\x02\x03")


    def test_post_completion_drain_discards_trailing_bytes(self):
        result = drain_after_completion(b"\xff+tail")
        self.assertTrue(result.ok)
        self.assertEqual(result.ignored, b"\xff+tail")
        self.assertEqual(result.remaining, b"")
