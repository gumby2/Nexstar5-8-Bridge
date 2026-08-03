# NexStar5/8-Bridge

ESP32 firmware for bridging telescope-control clients to an original Celestron
NexStar mount while preserving the mount's single-command protocol behavior.

## Tested baseline

This v1.0.0 repository release contains the validated NexStar5/8-Bridge
firmware. Its single canonical source is
`firmware/NexStar5-8-Bridge`; see [`docs/release_v1.0.0.md`](docs/release_v1.0.0.md)
for the hardware validation record.

## Target

- Board: ESP32 Dev Module
- ESP32 Arduino core: 3.3.10 target; release validation used 3.3.11
- FQBN: `esp32:esp32:esp32:PartitionScheme=huge_app`
- Mount UART: RX GPIO 16, TX GPIO 17

## Build

Install Arduino CLI and the ESP32 core:

```bash
arduino-cli core install esp32:esp32@3.3.10
```

Compile the validated source. Build output is kept outside the firmware source
tree:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --build-path .build/esp32 --output-dir dist --jobs 2 firmware/NexStar5-8-Bridge
```

Upload the compiled image from `dist/`:

```bash
arduino-cli upload -p <PORT> --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --input-file dist/NexStar5-8-Bridge.ino.bin --verify
```

The HTTPS setup server requires local certificate and private-key material.
Before compiling a fresh clone, create the ignored file
`firmware/https_credentials.h` as described in
[`docs/build_guide.md`](docs/build_guide.md). Never commit that file.

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
