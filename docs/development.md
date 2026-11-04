# Build, upload, and host tests

[Back to README](../README.md)

## Tools and environments

Install Make and PlatformIO Core. Host tests also require a C++17 compiler.
The project pins PlatformIO's Espressif32 platform to `~8.0.0`.
BLE uses the Arduino ESP32 core's bundled `BLEDevice` GATT API.

The final hardware target uses **`esp32-c3-lc29h-da-oled`**.
Make defaults to `esp32-c3-lc29h-da-oled`. Override it with `ENV` for other hardware.
Direct PlatformIO commands still default to `esp32-s3` unless you specify `-e`.

| Environment | ESP32 board | Receiver | OLED |
| --- | --- | --- | --- |
| `esp32-c3-lc29h-da-oled` | C3 SuperMini | LC29H(DA) | Yes |
| `esp32-c3-lc29h-da` | C3 SuperMini | LC29H(DA) | No |
| `esp32-c3-oled` | C3 SuperMini | ZED-F9P | Yes |
| `esp32-c3` | C3 SuperMini | ZED-F9P | No |
| `esp32-s3-lc29h-da-oled` | S3 N16R8 | LC29H(DA) | Yes |
| `esp32-s3-lc29h-da` | S3 N16R8 | LC29H(DA) | No |
| `esp32-s3-oled` | S3 N16R8 | ZED-F9P | Yes |
| `esp32-s3` | S3 N16R8 | ZED-F9P | No |

The ESP32-S3 N16R8 has 16 MB flash and 8 MB octal PSRAM.
The ESP32-C3 SuperMini uses the `esp32-c3-devkitm-1` board definition, with 4 MB flash and no PSRAM.
The original ZED-F9P environments use 38400 baud without startup commands.
LC29H(DA) environments use 115200 baud and [startup commands](receivers.md#startup-commands).
Custom environments can set `-DGNSS_RECEIVER=GNSS_RECEIVER_LC29H_DA`. Unsupported receiver values fail compilation.

## Commands

`make` defaults to `make build` for the C3 + LC29H(DA) + OLED target:

```sh
make test
make build
make upload
make monitor
```

Use the same environment for build, upload, and monitor. Other supported examples:

```sh
make build ENV=esp32-s3
make upload ENV=esp32-s3
make monitor ENV=esp32-s3

make build ENV=esp32-c3
make upload ENV=esp32-c3
make monitor ENV=esp32-c3

make build ENV=esp32-s3-lc29h-da
make upload ENV=esp32-s3-lc29h-da
make monitor ENV=esp32-s3-lc29h-da

make build ENV=esp32-c3-lc29h-da
make upload ENV=esp32-c3-lc29h-da
make monitor ENV=esp32-c3-lc29h-da
```

The monitor uses the 115200 baud setting from `platformio.ini`.
It shows startup diagnostics, including the free-heap measurement at boot.

## Upload and console ports

ESP32-S3: use the board's USB-to-UART connector for upload and diagnostic output. The receiver's USB port is separate.

ESP32-C3 SuperMini: the board exposes only its native USB connection, without a USB-to-UART bridge.
The C3 builds enable the USB CDC console with `ARDUINO_USB_CDC_ON_BOOT=1` and `ARDUINO_USB_MODE=1`.
`Serial` output appears on the board's USB port at 115200 baud. Upload and monitor through that same port.

Many SuperMini clones do not enter the bootloader automatically.
If upload times out, press `BOOT`/`GPIO9` while resetting or plugging in the board.
The CDC port can disappear while the board resets or enters the bootloader. Its name can change between phases.
These behaviors vary by board and have not all been checked on this hardware.

Only `esp32-c3-lc29h-da` currently sets `upload_port` and `monitor_port` to `/dev/tty.usbmodem12301`.
The OLED environment does **not** inherit these port settings.
Use explicit ports for the final hardware target to avoid automatic selection of another device:

```sh
pio run -e esp32-c3-lc29h-da-oled -t upload --upload-port /dev/tty.usbmodem12301
pio device monitor -e esp32-c3-lc29h-da-oled --port /dev/tty.usbmodem12301
```

Change the port to match the connected ESP32. To override other environments:

```sh
pio run -e esp32-s3 -t upload --upload-port /path/to/esp32-port
pio device monitor -e esp32-s3 --port /path/to/esp32-port
pio run -e esp32-c3 -t upload --upload-port /path/to/esp32-port
pio device monitor -e esp32-c3 --port /path/to/esp32-port
```

You can also change `upload_port` and `monitor_port` in `platformio.ini`.

## Host tests

Run `make test` or:

```sh
sh test/host/run.sh
```

The tests cover:

- NMEA checksum and fix parsing.
- BLE UART batching, rejected-chunk retention, retries, and clearing a pending chunk.
- Binary RTCM byte-preserving forwarding, including short writes.
- Status quality labels, malformed input, freshness boundaries, timer wraparound, and BLE labels.
- Receiver baud selection, command bytes, checksums, delays, repeated boots, and write failure without receiver responses.

The host tests do not compile or emulate the ESP32 BLE stack or OLED hardware.
