# Changelog

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
