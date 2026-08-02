# NexStar Protocol Converter

ESP32 firmware for bridging telescope-control clients to an original Celestron
NexStar mount while preserving the mount's single-command protocol behavior.

## Tested baseline

The repository's latest integrated reference is v6.99. The latest hardware
validation record is v7.03, built from the modular source currently held in the
Downloads release directory; see `docs/release_v7.03.md`.

The validated v7.03 source is now committed under
`firmware/Nexstar_Protocol_Converter_v7.03`. The older development sketch is
retained for comparison and rollback until the next integration pass.

## Target

- Board: ESP32 Dev Module
- ESP32 Arduino core: 3.3.10 target; v7.03 hardware validation used 3.3.11
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
```

For a validated release build, compile the matching versioned directory, for
example `firmware/Nexstar_Protocol_Converter_v6.99`.

## Documentation map

- `docs/architecture.md` - module ownership and runtime invariants.
- `docs/diagrams.md` - rendered architecture, runtime, protocol-flow, and recovery diagrams.
- `docs/release_v7.03.md` - v7.03 build, flash, and validation baseline.
- `docs/compatibility_matrix.md` - current Web, Telnet, protocol-client, and stability coverage.
- `docs/telnet_web_parity.md` - shared actions and browser-only behavior.
- `docs/protocol.md` - mount transaction rules and recovery boundaries.
- `docs/protocol_stack.md` - client request paths through the firmware.
- `docs/client_matrix.md` - client capabilities, ports, and response behavior.
- `docs/web_endpoints.md` - Web UI, HTTPS, and Alpaca route inventory.
- `docs/recovery_runbook.md` - diagnostic and recovery procedure.
- `docs/build_environment.md` - reproducible build and validation workflow.
- `docs/repository_layout.md` - source, release, tooling, and local-artifact policy.
- `docs/testing.md` - software and hardware validation expectations.
- `docs/console_help.md` - serial/Telnet command descriptions and diagnostics.
