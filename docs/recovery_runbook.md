# Network and mount recovery runbook

Use this sequence when the Web UI, Telnet, or client protocol appears to hang.

## Establish scope

Check whether the failure affects one client or the whole device:

```text
ping <device-ip>
HTTP GET /http_health
TCP connect to Telnet port 23
TCP connect to LX200 and Stellarium ports when enabled
```

If ping and every service fail, treat it as a Wi-Fi/device availability issue,
not a UI rendering issue.

## Capture COM12

Keep the serial monitor open at 115200 baud and record reset reason, heap,
handshake/payload/completion timeouts, ignored bytes, output stalls, and panic
messages. Do not repeatedly reset before capturing the first failure.

## Isolate mount behavior

With the mount disconnected, use `get` or Telnet Control -> Read RA/Dec to
verify bounded timeout and recovery. With the mount connected and tracking, use:

```text
rawgoto 1 wait 0
```

The `wait` form consumes `@` and performs the post-completion drain. Compare
raw handshake, payload, completion, and ignored-byte records with a `nowait`
run only when deliberately testing stale completion behavior.

## Recovery interpretation

- Missing `#`: handshake or stale-input problem; do not send the payload.
- Missing four payload bytes: keep the transaction bounded and defer polling.
- Unexpected bytes before `@`: record them and continue only to the bounded timeout.
- `@` followed by trailing bytes: drain them before the next E/Z poll.
- Web/Telnet failure during a mount wait: inspect yielding and heap health.

Record the exact command sequence, timing, raw bytes, heap values, and whether
the client closed first. Do not classify an intentionally early-closing test
client as a firmware output-stall defect.
