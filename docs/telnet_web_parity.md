# Telnet and Web UI parity

The Web UI is the complete configuration and catalog surface. Telnet provides a
safe operational subset and must use the same mount safety and command queue
rules.

## Shared operational actions

These actions are available from both surfaces:

- Read RA/Dec and Alt/Az
- Manual nudge
- RA/Dec and Alt/Az GOTO
- Mount status and system health
- Polling and handshake settings
- Logging controls
- Mode and network status

Telnet GOTO commands are:

```text
goto radec <RA hours 0..24) <Dec degrees -90..90>
goto altaz <altitude degrees -90..90> <azimuth degrees 0..360)
```

They validate input, apply the same horizon/altitude safety checks as the Web
handlers, reject an active or pending slew, and enqueue the asynchronous slew.
They must never write directly to the mount UART.

## Browser-only actions

The following remain Web UI actions because they require forms, selectors, or
larger client-side state:

- Catalog search and object selection
- Nearby-object selection and catalog GOTO
- On-demand BSC5 catalog loading
- Site/time forms and GPS Sync
- NTP configuration
- Safety limits and nudge-rate forms
- Wi-Fi/AP and server configuration forms

The Telnet Setup menu exposes these as explicit Web UI entries rather than
pretending that they are available through a hidden or incomplete text form.

## Status preference compatibility

Older UI builds stored `statusAdvanced=1` in browser local storage. If a newer
build does not include an advanced-status container, that preference must not
hide the basic Status panel. v1.0.0 forces the available basic panel visible in
that case.
