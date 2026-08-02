# Testing and validation

## Fast checks

Run these from the repository root:

```sh
python3 tools/browser-test/static_check.py
./scripts/test.sh
./scripts/build.sh
git diff --check
```

Build output must remain outside the firmware source directory. The primary
target is the classic ESP32 Dev Module with the Huge APP partition scheme.

## Hardware checks

For a firmware change, compile and flash the candidate to the designated ESP32
test board, then monitor the USB serial port for resets, assertions, heap
collapse, mount faults, and network-service stalls.

With the mount disconnected, verify graceful timeout and recovery. With the
mount connected and tracking, verify:

- repeated RA/Dec and Alt/Az polls;
- GOTO completion `@` handling and post-completion drain;
- invalid or delayed bytes before `#`, in payloads, and after `@`;
- Telnet menu navigation, including Control → Read RA/Dec;
- Web status, setup, catalog, and control actions;
- SkySafari/LX200, Alpaca, Stellarium, and Bluetooth compatibility as applicable.

Long-duration tests should record command counts, response failures, ignored
bytes, recovery pauses, free heap, and whether HTTP/Telnet remain reachable.

## Evidence standard

Do not call a test successful merely because the device eventually responds.
Record the exact command path, number of attempts, failures, serial warnings,
and recovery behavior. Separate expected client disconnects from firmware
output stalls.
