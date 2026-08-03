# Release checklist

Use this checklist for a new firmware release or a substantial refactor.

## Source and repository

- [ ] Confirm the canonical versioned firmware directory.
- [ ] Confirm no credentials, build output, or unrelated dirty files are included.
- [ ] Update the changelog, release note, compatibility matrix, and version strings.
- [ ] Run `git diff --check` and review `git status`.

## Build and software checks

- [ ] Run `python3 tools/browser-test/static_check.py`.
- [ ] Compile with `esp32:esp32:esp32:PartitionScheme=huge_app`.
- [ ] Record program and global-memory usage.
- [ ] Confirm the generated output is outside the firmware source directory.

## Hardware checks

- [ ] Flash the candidate to the designated ESP32 test board.
- [ ] Monitor serial output at 115200 baud for resets, panics, brownouts, and
      heap or mount faults.
- [ ] Verify Web UI, Telnet, mount polling, and recovery behavior.
- [ ] Exercise applicable SkySafari/LX200, Alpaca, Stellarium, and Bluetooth
      paths.
- [ ] Record firmware version, board, core version, test duration, failures,
      minimum heap, and listener reachability.

## Publish

- [ ] Commit the narrow, validated change.
- [ ] Create the matching Git tag.
- [ ] Push the branch and tag.
- [ ] Update the release note with the final evidence.
