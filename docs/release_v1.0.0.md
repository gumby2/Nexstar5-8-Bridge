# v1.0.0 release and baseline record

## Source and hardware

- Committed source: `firmware/Nexstar_Protocol_Converter_v7.03`
- Target: classic ESP32 Dev Module, Huge APP partition
- Arduino ESP32 core used for validation: 3.3.11
- Test port: COM12
- Network address during validation: `192.168.0.104`

This repository release packages the validated v7.03 firmware. The snapshot
reuses the existing HTTPS certificate include rather than duplicating private
key material. The firmware continues to identify itself as v7.03.

## Included firmware baseline

- Telnet GOTO RA/Dec and Alt/Az commands share Web UI safety and asynchronous
  slew queuing.
- Telnet Control and Setup menus expose the corresponding Web UI actions and
  clearly identify browser-only functions.
- Telnet menu label bounds were corrected for the expanded Control and Setup
  menus.
- The precompressed Web UI asset reports v7.03 and preserves on-demand BSC5
  loading.
- A stale `statusAdvanced` browser preference can no longer hide the only
  available Status panel.

## Validation evidence

- Compile succeeded: 2,565,539 bytes of program storage, 80,848 bytes of
  globals.
- Flash upload to COM12 completed with verified flash hashes.
- Serial boot reported v7.03 with no crashdump, panic, brownout, or reset
  marker.
- TCP listeners accepted connections on ports 23, 80, 4030, 10001, and 11111.
- Web `/status` returned HTTP 200 JSON.
- Web Status rendered populated Observer Status and System Health data.
- Telnet menu navigation through Control, Status, Setup, and Back completed.
- Invalid Telnet GOTO arguments were rejected without issuing a mount command.

The repository is tagged `v1.0.0`. The compatibility matrix records coverage
that remains partial or pending; this tag identifies the stable starting point,
not completion of every planned client test.
