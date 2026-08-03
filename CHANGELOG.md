# Changelog

## v1.0.0

- Initial clean repository release based on the validated v7.03 firmware.

## v7.03

- Added Telnet RA/Dec and Alt/Az GOTO commands using the same safety checks and
  asynchronous queue as the Web UI.
- Aligned Telnet Control and Setup menus with the Web UI and documented
  browser-only catalog, time, NTP, and safety actions.
- Corrected expanded Telnet menu label bounds.
- Regenerated the precompressed Web UI asset with on-demand BSC5 loading.
- Prevented stale browser `statusAdvanced` preferences from hiding the basic
  Status panel.
- Validated compile, flash, serial boot, Web, Telnet, and listener availability
  on classic ESP32 COM12.

## v6.99

- Clarified serial and Telnet help for `rawmount` and `rawgoto`.
- Documented the difference between completion-consuming `wait` diagnostics and
  low-level `nowait` diagnostics.

## v6.98

- Added bounded post-completion mount-byte draining for blocking and asynchronous
  GOTO paths.
- Preserved the single-command mount rule while preventing trailing bytes from
  contaminating the next position poll.
- Validated on classic ESP32 hardware with COM12, Web status, Telnet menu, and
  completion-consuming self-GOTO diagnostics.

## v5.75

- Completed the working refactor baseline from the current modularized source.
- Preserved existing Bluetooth/SkySafari, Telnet, Wi-Fi, web, mount, and protocol behavior with no intended functional changes.
