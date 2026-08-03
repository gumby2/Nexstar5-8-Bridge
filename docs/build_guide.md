# Build and flash guide

The supported target is a classic ESP32 Dev Module using the Huge APP
partition scheme. The repository release is v1.0.0; the firmware identifies as
NexStar5/8-Bridge v1.0.0.

## Arduino IDE

1. Install Arduino IDE and the ESP32 board package.
2. Select `ESP32 Dev Module`.
3. Select partition scheme `Huge APP (3MB No OTA/1MB SPIFFS)`.
4. Open `firmware/NexStar5-8-Bridge/`.
5. Select the matching `.ino` file, choose the board's COM port, compile, and
   upload.

The mount UART uses RX GPIO 16 and TX GPIO 17. Keep the serial monitor at
115200 baud for boot and diagnostic output.

## Arduino CLI

From the repository root:

```sh
arduino-cli core install esp32:esp32@3.3.10
./scripts/test.sh
./scripts/build.sh
```

Build caches and generated output remain outside the firmware source tree. Set
`ARDUINO_CLI`, `BUILD_PATH`, or `OUTPUT_DIR` when the defaults do not fit the
host environment.
