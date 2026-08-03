# ESP32 DevKit wiring

This wiring documents the CP2102 6-in-1 serial converter used in the FIT0781
enclosure. The converter is the electrical level translator between the ESP32
3.3 V UART and the NexStar RS-232 hand-controller cable.

## ESP32-to-converter wiring

| ESP32 | Converter TTL header | Direction |
| --- | --- | --- |
| GPIO 17 / UART2 TX | TTL RX | ESP32 to mount |
| GPIO 16 / UART2 RX | TTL TX | Mount to ESP32 |
| GND | GND | Common reference |

Do not connect the converter's RS-232 pins directly to ESP32 GPIO pins. The
TTL-side ground must be common with the ESP32 ground.

## Converter switch positions

Power the converter off before changing switches. For the TTL-to-RS-232 mode
used by this project, set the CP2102 board as follows:

| Control | Position |
| --- | --- |
| DIP switch 1 | OFF |
| DIP switch 2 | OFF |
| 232-TTL / 232-485 selector | 232-TTL / UP |
| Logic voltage selector | 3.3 V / 3V3 |

The board must be in **TTL to 232**, not USB-to-TTL, USB-to-232, or RS-232-to-
RS-485 mode. Switch legends vary between manufacturers; verify the labels on
the physical board before applying these positions.

## NexStar RS-232 cable

The Celestron hand-controller cable presents the computer side as a DB-9
serial connection. Wire the converter's RS-232 header to the DB-9 side as:

| Converter RS-232 pin | DB-9 pin | Function |
| --- | --- | --- |
| 232-TX | 3 | Computer/mount transmit |
| 232-RX | 2 | Computer/mount receive |
| GND | 5 | Signal ground |

The other end of the cable plugs into the RS-232 jack on the bottom of the
NexStar hand controller. This is not the hand-controller **HAND CONTROL** or
**AUX** socket. Use the RS-232 cable supplied for, or correctly wired for, the
NexStar controller; the RJ-22 cable is not a normal Ethernet cable.

The mount protocol runs at 9600 baud, 8 data bits, no parity, and one stop bit.
The firmware configures this automatically.

References: [Celestron NexStar 5 manual RS-232 appendix](https://www.celestrondownloads.com/repository/_Manuals%20by%20Product%20Number/11031_nexstar_5_manual.pdf),
[CP2102 6-in-1 converter manual](https://manuals.plus/asin/B0F37QWLYH.pdf).
