# NexStar 5/8 Bridge user guide

Repository release: **v1.0.0**  
Included firmware identity: **v7.03**

This is an operator guide for setting up and using the bridge. Open only the
task you need. Technical details are linked from the relevant section.

<details open>
<summary id="quick-start"><strong>Quick start: get connected</strong></summary>

1. Connect the mount UART using RX GPIO 16 and TX GPIO 17.
2. Power the ESP32 and mount.
3. Find the bridge's IP address from the serial console, router, or access-point
   setup.
4. Open `http://<device-ip>/` in a browser.
5. In **Setup**, confirm Wi-Fi, site/time, mount polling, and service ports.
6. Open **Status** and confirm that the mount, position, time, and network
   services are populated.

If the Web UI does not open, start with the [troubleshooting section](#troubleshooting),
then use the [recovery runbook](recovery_runbook.md).

</details>

<details>
<summary id="web-ui"><strong>Web UI: what each area does</strong></summary>

| Area | Use it for |
| --- | --- |
| Status | Current mount state, position, time, location, heap, and services. |
| Setup | Wi-Fi, site/time, polling, connection settings, NTP, and catalog loading. |
| Control | Read position, nudge, and submit safe GOTO requests. |
| Catalog | Load the BSC5 catalog on demand and find nearby catalog objects. |
| Mount Test | Controlled mount diagnostics when the mount is connected. |

The basic Status panel may be shown instead of the advanced panel. Status data
is fetched asynchronously; a populated JSON response with an empty display
usually indicates a browser-side issue rather than missing device data.

See [`web_endpoints.md`](web_endpoints.md) for the route inventory and
[`telnet_web_parity.md`](telnet_web_parity.md) for Web/Telnet differences.

</details>

<details>
<summary id="setup"><strong>Setup: configure the bridge</strong></summary>

Use **Setup** for:

- Wi-Fi station and access-point settings.
- LX200, Stellarium, Alpaca, and Telnet ports.
- Site latitude, longitude, elevation, and time settings.
- NTP synchronization or manual device time.
- Mount polling and handshake timing.
- Loading the BSC5 catalog only when it is needed.

After changing network ports or settings that request a restart, reconnect to
the new address and verify the values on Status. Do not repeatedly press a
setting button while a previous action still says **Working**.

Default ports are HTTP 80, Telnet 23, LX200 4030, Stellarium 10001, Alpaca
HTTP 11111, and Alpaca discovery UDP 32227. The device's Status output is the
authority if ports have been changed.

</details>

<details>
<summary id="clients"><strong>Clients: connect your astronomy software</strong></summary>

| Client | Connection | Main setting |
| --- | --- | --- |
| SkySafari Wi-Fi | LX200 TCP | Device IP and LX200 port, normally 4030. |
| SkySafari Bluetooth | Bluetooth SPP | Pair with the bridge's Bluetooth name. |
| Stellarium | Stellarium TCP | Device IP and Stellarium port, normally 10001. |
| Alpaca software | Alpaca HTTP | Device IP and Alpaca port, normally 11111. |

Use one client at a time when first commissioning the system. Once position
reads work, add GOTO testing, then test simultaneous Web/Telnet activity.

All clients share the mount's single-command transport. A client may receive a
cached or estimated position while a GOTO is active; this is expected behavior.

</details>

<details>
<summary id="telnet"><strong>Telnet: monitor and operate from a console</strong></summary>

Connect to port 23 by default. The Telnet menu provides status, setup, control,
and diagnostic paths. Useful commands include:

```text
status
health
current_state
menu
```

For controlled mount diagnostics, use `rawmount` or the completion-consuming
form of `rawgoto`. Avoid `nowait` unless deliberately testing stale completion
bytes; it can leave data for the next mount transaction.

See [`console_help.md`](console_help.md) for the complete command reference.

</details>

<details>
<summary id="mount-operation"><strong>Mount operation: safe reads and GOTO</strong></summary>

The original NexStar mount accepts one command at a time. The bridge therefore:

- waits for the `?` / `#` handshake when required;
- prevents a second mount command while one is active;
- does not poll position during an active GOTO;
- consumes the `@` completion marker and drains trailing bytes;
- reports cached or estimated coordinates during motion.

Do not power-cycle the mount during a slew unless recovery requires it. If the
mount sends malformed or unexpected bytes, stop issuing new tests and capture
the serial output before resetting anything.

See [`protocol.md`](protocol.md) and [`recovery_runbook.md`](recovery_runbook.md)
for the transaction and recovery rules.

</details>

<details>
<summary id="troubleshooting"><strong>Troubleshooting: choose the symptom</strong></summary>

<details>
<summary><strong>Web UI or Telnet is unreachable</strong></summary>

Check the device IP, ping the device, and test `/http_health`. Then check the
serial monitor for resets, brownouts, heap collapse, or network-service stalls.
If every service fails, treat it as a Wi-Fi/device problem rather than a page
rendering problem.
</details>

<details>
<summary><strong>Mount shows disconnected or times out</strong></summary>

Check power, RX/TX orientation, common ground, and the mount cable. Use a
bounded RA/Dec read and confirm that no new command is sent after a handshake or
payload timeout.
</details>

<details>
<summary><strong>Telnet drops during a command</strong></summary>

Record the command, serial output, free heap, minimum heap, and whether Web UI
requests were active at the same time. Avoid repeated reconnect loops until the
first failure is captured.
</details>

<details>
<summary><strong>Status is loading or appears blank</strong></summary>

Open `/status` directly. If JSON is returned, reload the page with cache
disabled and inspect the browser console for JavaScript errors. Confirm that the
browser is using the same device IP and protocol as the API request.
</details>

More detailed procedures are in [`recovery_runbook.md`](recovery_runbook.md).

</details>

<details>
<summary id="update-firmware"><strong>Update firmware</strong></summary>

Use [`build_guide.md`](build_guide.md) for Arduino IDE or CLI instructions.
Before flashing, record the current firmware version and settings. After
flashing, keep the serial monitor open at 115200 baud and verify:

- firmware version and clean boot;
- no panic, brownout, or reset markers;
- Web UI and `/status` availability;
- Telnet connection and `status` output;
- mount connection and position polling.

For a release candidate, follow [`release_checklist.md`](release_checklist.md).

</details>

<details>
<summary><strong>Reference documents</strong></summary>

- [`client_matrix.md`](client_matrix.md) - client capabilities and ports.
- [`compatibility_matrix.md`](compatibility_matrix.md) - tested and pending coverage.
- [`architecture.md`](architecture.md) - module ownership and invariants.
- [`protocol_stack.md`](protocol_stack.md) - request paths through the firmware.
- [`diagrams.md`](diagrams.md) - functional and recovery diagrams.

</details>
