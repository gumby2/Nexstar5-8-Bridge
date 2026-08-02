# Compatibility and stability matrix

Status values:

- **Checked** — exercised during the current v7.03 validation sequence.
- **Partial** — some paths were exercised, but the complete matrix remains.
- **Pending** — needs a dedicated repeatable test run.

| Surface | v7.03 status | Evidence or remaining coverage |
| --- | --- | --- |
| Web UI — Status | Checked | Status JSON and rendered Observer/System Health verified. |
| Web UI — Setup | Checked | Setup fields populated and navigation verified. |
| Web UI — Catalog | Partial | Nearby-object selection verified; BSC5 load-button behavior needs a dedicated full catalog run. |
| Web UI — Control | Partial | GOTO/read/nudge paths exist; complete safe-action matrix remains. |
| Telnet console | Checked | Prompt, help, invalid GOTO validation, menu navigation, and back navigation verified. |
| SkySafari/LX200 | Partial | Listener availability checked; full client transaction matrix remains. |
| Alpaca | Partial | Port 11111 availability checked; discovery and device transaction matrix remains. |
| Stellarium | Partial | Port 10001 availability checked; packet and tracking transaction matrix remains. |
| Bluetooth | Pending | Must be tested in the applicable Bluetooth bridge mode. |
| Mount timeout/recovery | Partial | Recovery behavior observed; delayed/malformed response matrix should be automated. |
| Long-duration network stability | Partial | Prior stress runs exist; a clean v7.03 endurance run remains. |

The matrix should be updated with date, firmware version, client version, test
duration, command counts, failures, serial warnings, minimum heap, and whether
each listener remained reachable.
