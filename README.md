# NexStar Protocol Converter

ESP32 firmware for bridging telescope-control clients to an original Celestron NexStar mount while preserving the mount's single-command protocol behavior.

## Tested baseline

The latest validated ESP32 release is v6.99. It includes the asynchronous HTTP
compatibility layer, nonblocking Telnet output handling, and bounded mount-byte
draining after GOTO completion.

The repository also retains earlier versioned release snapshots under
`firmware/` for comparison and rollback. The v6.99 snapshot is the current
hardware-tested reference.

## Target

- Board: ESP32 Dev Module
- ESP32 Arduino core: 3.3.10
- FQBN: `esp32:esp32:esp32:PartitionScheme=huge_app`
- Mount UART: RX GPIO 16, TX GPIO 17

## Build

Install Arduino CLI and the ESP32 core:

```bash
arduino-cli core install esp32:esp32@3.3.10
```

Compile the development sketch:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app firmware/Nexstar_Protocol_Converter
```

Upload:

```bash
arduino-cli upload -p <PORT> --fqbn esp32:esp32:esp32:PartitionScheme=huge_app firmware/Nexstar_Protocol_Converter

For a validated release build, compile the matching versioned directory, for
example `firmware/Nexstar_Protocol_Converter_v6.99`.

## Documentation map

- `docs/architecture.md` — module ownership and runtime invariants.
- `docs/protocol.md` — mount transaction rules and recovery boundaries.
- `docs/build_environment.md` — reproducible build and validation workflow.
- `docs/repository_layout.md` — source, release, tooling, and local-artifact policy.
- `docs/testing.md` — software and hardware validation expectations.
- `docs/console_help.md` — serial/Telnet command descriptions and diagnostics.
```
