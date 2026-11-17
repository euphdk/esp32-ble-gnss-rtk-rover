# ESP32 BLE RTK rover

A **Seeed Studio XIAO ESP32-C6 + Quectel LC29H(DA) + 128×64 SSD1306 OLED** rover for SW Maps.
The XIAO uses the **Grove Base for XIAO with battery management**. ESP32-C3 SuperMini and ESP32-S3 targets remain available.
The ESP32 forwards receiver data over BLE and sends the phone's RTCM corrections to the receiver.
The OLED shows fix quality, UTC date/time, and GST accuracy, with brief BLE connection notifications. The receiver computes the RTK solution.

## Connect the hardware

Disconnect power before wiring. Use 3.3 V UART and I2C logic, with a shared ground.

| XIAO C6 Grove Base | Connection |
| --- | --- |
| Serial RX, D7/GPIO17 | LC29H UART TX |
| Serial TX, D6/GPIO16 | LC29H command/RTCM UART RX |
| Serial GND | LC29H ground |
| Serial VCC (3.3 V) | LC29H `V` |
| I2C SDA, D4/GPIO22 | OLED SDA |
| I2C SCL, D5/GPIO23 | OLED SCL |
| I2C GND | OLED GND |
| I2C VCC (3.3 V) | OLED VCC |

The display must be a 128×64 SSD1306 at I2C address `0x3C`.
Confirm the receiver carrier's UART routing and power connections before use.
The user confirmed that the LC29H carrier works from 3.3 V at `V`. Check the supply's total current capacity.
See [hardware details](docs/hardware.md) and the [battery power plan](docs/battery-power.md) before assembly.

For the XIAO C6, connect the OLED to the base's labeled **I2C** port and GNSS signals to **Serial**.
Use GPIO22/SDA, GPIO23/SCL, GPIO17/RX, and GPIO16/TX.
See [Grove port wiring and power limits](docs/hardware.md#xiao-esp32-c6-with-grove-base).

## Build and upload

Install Make and PlatformIO Core. Host tests also need a C++17 compiler.
Make and PlatformIO default to **`xiao-esp32-c6-lc29h-da-oled`**. Override Make with `ENV` for other hardware.

```sh
make test
make build
make upload PORT=/dev/tty.usbmodem12301
make monitor PORT=/dev/tty.usbmodem12301
```

Replace the port with your XIAO C6's USB port. The console uses 115200 baud.
The firmware sends receiver configuration at startup, including a request for GST accuracy data.

## Use SW Maps

1. Power the receiver before starting the ESP32.
2. Connect to **`GNSS-RTK-Rover-XXXX`** through SW Maps' BLE receiver workflow (`XXXX` is the Bluetooth MAC suffix).
3. Enable your NTRIP correction source in SW Maps.
4. Check position updates, accuracy fields, and fix quality.

No serial monitor is required for operation. See [troubleshooting](docs/troubleshooting.md) if updates stop while BLE remains connected.
Positioning, GST accuracy fields, and RTK FLOAT have been confirmed with the C3 and LC29H(DA).
The 128×64 OLED output is confirmed readable. Sustained throughput with the display and long-duration reliability still need hardware checks.

## Documentation

| Guide | Contents |
| --- | --- |
| [Hardware and wiring](docs/hardware.md) | Carrier photos, Grove Base ports, UART wiring, alternate ESP32-S3 and ZED-F9P hardware |
| [OLED](docs/display.md) | Display wiring, status labels, failure behavior |
| [Receiver configuration](docs/receivers.md) | LC29H startup commands, GST, ZED-F9P setup |
| [SW Maps and troubleshooting](docs/troubleshooting.md) | Connection setup, serial diagnostics, validation and limitations |
| [Battery power](docs/battery-power.md) | Grove Base battery supply, C3 charger and regulator plan, safety checks |
| [Development](docs/development.md) | All build environments, serial ports, host tests |
| [BLE protocol](docs/ble-protocol.md) | GATT UUIDs, packet format, buffering and retries |
