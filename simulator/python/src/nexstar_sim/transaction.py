"""Reference framing rules used by the mount transaction tests."""

from dataclasses import dataclass


@dataclass(frozen=True)
class FrameResult:
    ok: bool
    value: bytes
    ignored: bytes
    remaining: bytes


def consume_handshake(stream: bytes) -> FrameResult:
    """Consume stale bytes until the NexStar ``#`` handshake terminator."""
    try:
        index = stream.index(b"#")
    except ValueError:
        return FrameResult(False, b"", stream, b"")
    return FrameResult(True, b"#", stream[:index], stream[index + 1 :])


def consume_payload(stream: bytes, length: int) -> FrameResult:
    """Read exactly one fixed-length payload without consuming later bytes."""
    if len(stream) < length:
        return FrameResult(False, b"", stream, b"")
    return FrameResult(True, stream[:length], b"", stream[length:])


def consume_completion(stream: bytes) -> FrameResult:
    """Consume unexpected bytes until the movement completion ``@`` marker."""
    try:
        index = stream.index(b"@")
    except ValueError:
        return FrameResult(False, b"", stream, b"")
    return FrameResult(True, b"@", stream[:index], stream[index + 1 :])


def drain_after_completion(stream: bytes) -> FrameResult:
    """Model the bounded post-completion drain for already-arrived bytes."""
    return FrameResult(True, b"", stream, b"")
