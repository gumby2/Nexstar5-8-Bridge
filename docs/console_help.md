# Console and Telnet help

The serial and Telnet consoles expose the same operational concepts. Keep
descriptions consistent when adding or renaming commands.

## Safe diagnostic commands

- `status` — firmware, network, clients, heap, loop, polling, and mount state.
- `health` — system-health counters and fault detail.
- `current_state` / `pos` — cached coordinates and bridge state.
- `mountpoll` — show or change the mount polling interval; `0` disables it.
- `handshake` — show or change the bounded NexStar handshake timeout.
- `drain` — explicitly display and remove bytes currently waiting on the mount UART.
- `rawmount` — exercise handshake and fixed-length E/Z payload reads.
- `rawgoto` — read current RA/Dec and send it back as an R GOTO.

## `rawgoto` safety

Use `rawgoto ... wait ...` for normal diagnostics. It waits for and consumes
the mount's `@` completion marker and performs the normal post-completion
drain. `nowait` is intentionally lower-level: it sends the GOTO without
consuming completion and can leave delayed bytes for the next transaction.

Example:

```text
rawgoto 1 wait 0
```

## Direct GOTO and menu commands

```text
goto radec <RA hours 0..24) <Dec degrees -90..90>
goto altaz <altitude degrees -90..90> <azimuth degrees 0..360)
menu
```

The GOTO commands use the same safety checks and asynchronous queue as the Web
UI. The ANSI menu identifies browser-only catalog, time, NTP, and safety-form
actions instead of duplicating those larger interfaces in Telnet.

## Operational invariants

- The mount accepts one command at a time.
- A new command must not be sent while a prior transaction is active.
- Unexpected bytes are recorded and discarded within bounded waits.
- Do not use `rawgoto` as a substitute for a user-facing slew test while the
  mount is already executing another GOTO.

When help text changes, update this page and the corresponding serial/Telnet
help output together. Firmware help changes belong in a new versioned release,
not by editing an already-published snapshot.
