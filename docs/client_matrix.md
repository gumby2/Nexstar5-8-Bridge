# Client and endpoint matrix

Default v1.0.0 ports are HTTP 80, Telnet 23, SkySafari/LX200 4030,
Stellarium 10001, Alpaca HTTP 11111, and Alpaca discovery UDP 32227. Port
settings may be changed in Setup; use device status output as authoritative.

| Client or service | Transport / port | Position reads | GOTO / motion | Response source | Notes |
| --- | --- | --- | --- | --- | --- |
| SkySafari Wi-Fi | LX200 TCP / `LX200_PORT` | Cached position or async mount read | Queued RA/Dec or Alt/Az path | LX200 text | Single-command mount rules apply. |
| SkySafari Bluetooth | Bluetooth SPP | Shared LX200 core | Shared GOTO queue | LX200 text over SPP | Bluetooth parsing is not accepted during mount waits. |
| Alpaca | HTTP / `ALPACA_PORT` | Cached position APIs | Queued RA/Dec or Alt/Az request | Alpaca JSON | Requests are staged where needed to protect the loop. |
| Stellarium | Binary TCP / `STELLARIUM_PORT` | Cached RA/Dec | Queued RA/Dec request | Binary position packet | Binary packet buffering is handled by the Stellarium service. |
| Web UI | HTTP / `HTTP_WEB_PORT` | `/status` and cached display data | Web GOTO and mount-test routes | JSON, HTML, or text | Browser UI refreshes status asynchronously. |
| Serial console | USB UART | `get`, `getaltaz`, `pos`, `status` | `rawgoto`, nudge, diagnostics | Human-readable text | Best surface for raw mount diagnostics. |
| Telnet console | TCP / `TELNET_PORT` | Menu status and commands | Menu control and diagnostics | ANSI/text | Output is bounded so stalled clients cannot block services. |

All client paths share the position cache, GOTO queue, NexStar encoding, and
mount transport lock. Client-specific parsers must not directly write to
`MountSerial` or invent alternate coordinate encoding.
