# ESP32 BLE RTK rover

An **ESP32-C3 SuperMini + Quectel LC29H(DA) + 128×64 SSD1306 OLED** rover for SW Maps.
The ESP32 forwards receiver data over BLE and sends the phone's RTCM corrections to the receiver.
The OLED shows fix quality and GST accuracy, with brief BLE connection notifications. The receiver computes the RTK solution.

## Connect the hardware

Disconnect power before wiring. Use 3.3 V UART and I2C logic, with a shared ground.

| ESP32-C3 | Connection |
| --- | --- |
| GPIO4 (GNSS RX) | LC29H UART TX |
| GPIO5 (GNSS TX) | LC29H command/RTCM UART RX |
| GND | LC29H ground and OLED GND |
| GPIO6 | OLED SDA |
| GPIO7 | OLED SCL |
| 3V3 | OLED VCC |

The display must be a 128×64 SSD1306 at I2C address `0x3C`.
Confirm the receiver carrier's UART routing and power connections before use.
The LC29H carrier accepts 5 V at `V`; do not power that input from the C3's 3V3 pin.
See [hardware details](docs/hardware.md) and the [battery power plan](docs/battery-power.md) before assembly.

## Build and upload

Install Make and PlatformIO Core. Host tests also need a C++17 compiler.
Make defaults to **`esp32-c3-lc29h-da-oled`**. Override it with `ENV` for other hardware.

```sh
make test
make build
pio run -e esp32-c3-lc29h-da-oled -t upload --upload-port /dev/tty.usbmodem12301
pio device monitor -e esp32-c3-lc29h-da-oled --port /dev/tty.usbmodem12301
```

Replace the port with your C3's USB port. The console uses 115200 baud.
The firmware sends receiver configuration at startup, including a request for GST accuracy data.

## Use SW Maps

1. Power the receiver before starting the ESP32.
2. Connect to **`GNSS-RTK-Rover`** through SW Maps' BLE receiver workflow.
3. Enable your NTRIP correction source in SW Maps.
4. Check position updates, accuracy fields, and fix quality.

No serial monitor is required for operation. See [troubleshooting](docs/troubleshooting.md) if updates stop while BLE remains connected.
Positioning, GST accuracy fields, and RTK FLOAT have been confirmed with the C3 and LC29H(DA).
The 128×64 OLED output is confirmed readable. Sustained throughput with the display and long-duration reliability still need hardware checks.

## Documentation

| Guide | Contents |
| --- | --- |
| [Hardware and wiring](docs/hardware.md) | Carrier photos, UART wiring, alternate ESP32-S3 and ZED-F9P hardware |
| [OLED](docs/display.md) | Display wiring, status labels, failure behavior |
| [Receiver configuration](docs/receivers.md) | LC29H startup commands, GST, ZED-F9P setup |
| [SW Maps and troubleshooting](docs/troubleshooting.md) | Connection setup, serial diagnostics, validation and limitations |
| [Battery power](docs/battery-power.md) | 18650 charger, regulator, boost converter, safety checks |
| [Development](docs/development.md) | All build environments, serial ports, host tests |
| [BLE protocol](docs/ble-protocol.md) | GATT UUIDs, packet format, buffering and retries |
