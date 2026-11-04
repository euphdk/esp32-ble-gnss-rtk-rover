# 64×48 I2C OLED

[Back to README](../README.md)

The final hardware target uses the `esp32-c3-lc29h-da-oled` environment.
OLED support is disabled in the original environments. Append `-oled` to an environment name to enable it:

```sh
make build ENV=esp32-c3-lc29h-da-oled
make build ENV=esp32-s3-lc29h-da-oled
make build ENV=esp32-c3-oled
make build ENV=esp32-s3-oled
```

Use the same environment for upload and monitor. These environments add the Adafruit SSD1306 and GFX libraries.
The firmware assumes a 64×48 SSD1306 at I2C address `0x3C`, internal charge pump, and no separate reset wire.
It uses the library's default controller initialization and column/page addressing, without a module-specific offset.
Some 64×48 modules need different offsets or use another controller. Those modules are not covered by this configuration.

## Wiring

Disconnect power before connection. Use a module that supports 3.3 V power and 3.3 V I2C signals.
Check the module's pin labels and pull-up voltage. Do not use pull-ups to 5 V.

| OLED | ESP32-C3 SuperMini | ESP32-S3 |
| --- | --- | --- |
| VCC | 3V3 | 3V3 |
| GND | GND | GND |
| SDA | GPIO6 | GPIO8 |
| SCL | GPIO7 | GPIO9 |

These pins avoid the configured GNSS UART pins, USB pins, and boot strapping pins on each target.
They assume the board exposes these GPIOs without other peripherals attached. Check your board before connection.
Pin flags are in `platformio.ini`. The I2C address is `kOledAddress` in `include/config.h`.

## Status labels

The display updates about once per second. It shows the latest valid GGA quality and BLE connection state:

| Display | Meaning |
| --- | --- |
| `NO GGA` | No checksum-valid GGA with a valid quality field received since boot |
| `GGA STALE` | No valid GGA quality update for at least five seconds |
| `NO FIX` | GGA quality 0 |
| `GNSS` | GGA quality 1 |
| `DGPS` | GGA quality 2 |
| `RTK FIX` | GGA quality 4 |
| `RTK FLOAT` | GGA quality 5 |
| `OTHER FIX` | Another single-digit GGA quality, not interpreted as RTK |
| `BLE UP` / `BLE DOWN` | BLE connected / disconnected, not notification subscription or correction reception |

Checksum failures, malformed quality fields, and other NMEA messages do not refresh GGA status.
Freshness tracking does not depend on serial diagnostics. A new valid GGA replaces the stale indication.
The OLED does not confirm correction delivery or position accuracy. It adds no data to the BLE stream.

## Failure behavior and checks

The firmware probes the display at startup. If it is absent, the firmware skips display updates and continues GNSS/BLE operation.
I2C operations use a five-millisecond driver timeout. Display transfers are synchronous and add loop latency, not a separate task.
If a later probe fails, updates stop until the ESP32 restarts. Hot-plug recovery is not supported.
OLED wiring, controller offsets, display appearance, and forwarding throughput with the display require hardware checks.
