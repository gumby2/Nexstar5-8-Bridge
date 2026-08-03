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

The Celestron cable uses a small 4P4C handset-style plug, commonly called an
RJ22. It plugs into the dedicated **RS-232** jack on the bottom of the NexStar
hand controller. Do not plug it into the **HAND CONTROL** or **AUX** socket.

### RJ22 plug orientation and pinout

Look into the plug from the contact/opening end, with the gold contacts facing
you and the locking latch pointing down. Number the four contact positions
from left to right. With that orientation, connect the converter's RS-232
header as follows:

```text
                 Looking into RJ22 plug contacts
                 latch / locking tab pointing down

       left                                             right
        1                 2                 3                 4
     [ TX ]            [ NC ]            [ GND ]            [ RX ]
       |                 |                 |                 |
       +-- converter     +-- leave        +-- converter      +-- converter
           232-TX           unconnected      GND                 232-RX
```

| RJ22 contact | Connect to converter | Function |
| --- | --- | --- |
| 1 | 232-TX | Converter transmit to mount |
| 2 | No connection | Unused |
| 3 | GND | Signal ground |
| 4 | 232-RX | Mount transmit to converter |

This is the contact-side view. If the plug is viewed from the cable-entry
side, or flipped over, left and right are reversed. Do not use wire colors as
the pin reference; handset cables can be reversed and colors vary. Use the
contact number or verify each conductor with a continuity tester.

The cable is a four-conductor RJ22 cable, not a normal Ethernet/RJ45 cable.
Only the three contacts shown above are used by the mount serial interface.

The mount protocol runs at 9600 baud, 8 data bits, no parity, and one stop bit.
The firmware configures this automatically.

References: [Celestron NexStar RS-232 cable diagram](https://www.nexstarsite.com/PCControl/RS232Cable.htm),
[Celestron NexStar 5 manual](https://www.celestrondownloads.com/repository/_Manuals%20by%20Product%20Number/11031_nexstar_5_manual.pdf),
[CP2102 6-in-1 converter manual](https://manuals.plus/asin/B0F37QWLYH.pdf).
