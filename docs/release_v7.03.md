# v7.03 release and baseline record

## Source and hardware

- Committed source: `firmware/Nexstar_Protocol_Converter_v7.03`
- Original validation source: `C:\Users\scott\Downloads\Nexstar_Protocol_Converter_v7.03`
- Target: classic ESP32 Dev Module, Huge APP partition
- Arduino ESP32 core used for validation: 3.3.11
- Test port: COM12
- Network address during validation: `192.168.0.104`

The v7.03 source is the modular v6.99-derived source tree now committed in the
repository. The repository snapshot reuses the existing HTTPS certificate
include rather than duplicating private key material.

## Changes documented by this release

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

This is a validation record, not a Git tag. A formal tag should wait until the
full compatibility matrix is complete.
