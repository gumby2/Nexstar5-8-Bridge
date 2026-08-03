# NexStar Protocol Converter

ESP32 firmware for bridging telescope-control clients to an original Celestron
NexStar mount while preserving the mount's single-command protocol behavior.

## Tested baseline

This v1.0.0 repository release is based on the validated v7.03 firmware. Its
canonical source is `firmware/Nexstar_Protocol_Converter_v7.03`; see
[`docs/release_v1.0.0.md`](docs/release_v1.0.0.md) for the hardware validation
record. The older
unversioned sketch is retained for comparison only.

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

Compile the validated source:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app firmware/Nexstar_Protocol_Converter_v7.03
```

Upload:

```bash
arduino-cli upload -p <PORT> --fqbn esp32:esp32:esp32:PartitionScheme=huge_app firmware/Nexstar_Protocol_Converter_v7.03
```

For repeatable builds, compile the versioned directory matching the release
being tested. Do not overwrite a published release snapshot in place.

## Documentation map

Start here:

- [`docs/user_guide.md`](docs/user_guide.md) - collapsible operator guide and quick actions.
- [`docs/build_guide.md`](docs/build_guide.md) - Windows, Arduino IDE, and CLI setup.
- [`docs/release_checklist.md`](docs/release_checklist.md) - repeatable release validation.
- [`docs/release_v1.0.0.md`](docs/release_v1.0.0.md) - this release's baseline record.

Understand the system:

- [`docs/architecture.md`](docs/architecture.md) - module ownership and invariants.
- [`docs/diagrams.md`](docs/diagrams.md) - architecture and protocol-flow diagrams.
- [`docs/protocol.md`](docs/protocol.md) - mount transaction and recovery rules.
- [`docs/client_matrix.md`](docs/client_matrix.md) - clients and service ports.
- [`docs/web_endpoints.md`](docs/web_endpoints.md) - Web, HTTPS, and Alpaca routes.

Operate and troubleshoot:

- [`docs/console_help.md`](docs/console_help.md) - serial/Telnet commands.
- [`docs/telnet_web_parity.md`](docs/telnet_web_parity.md) - console/Web behavior.
- [`docs/recovery_runbook.md`](docs/recovery_runbook.md) - network and mount recovery.
- [`docs/compatibility_matrix.md`](docs/compatibility_matrix.md) - tested coverage.

For repository policy, testing, memory, simulator, and layout details, see the
remaining documents in [`docs/`](docs/).
