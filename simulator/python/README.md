# Python simulator package

This directory contains small deterministic protocol tests that run without
hardware. A full serial/network mount emulator remains future work.

## Current tests

`src/nexstar_sim/transaction.py` and `tests/test_transaction_vectors.py`
encode the byte-boundary rules exercised by the hardware diagnostics:

- stale bytes before `#` are ignored and recorded;
- fixed-length payloads do not consume the next frame;
- unexpected bytes before `@` are recorded;
- missing handshake and short payload are bounded failures;
- trailing bytes after completion are drained before the next transaction.

Run the vectors with:

```sh
pytest simulator/python/tests
```

The planned mount, LX200, Alpaca, Stellarium, and fault-injection emulators can
be added later as separate changes with their own hardware-correlation tests.
