# 128×64 I2C OLED

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
The firmware uses a 128×64 SSD1306 at I2C address `0x3C`, internal charge pump, and no separate reset wire.
It uses the library's default controller initialization and column/page addressing, without a module-specific offset.
Panels with other dimensions or controllers, such as SH1106, are not covered by this configuration.

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

The display updates about once per second. It shows GGA fix quality in the top half and GST accuracy in the bottom half.

Both halves use double-size text. There is no title row.
The bottom half normally shows two lines, `ΔH` and `ΔV`, with adaptive units.
Values below one meter use centimeters: one decimal below 10 cm, otherwise whole centimeters.
Values of one meter or more use meters with one decimal. Examples: `92cm`, `2.0cm`, and `1.3m`.
Horizontal accuracy is `sqrt(latitude deviation² + longitude deviation²)` from GST. Vertical accuracy is the GST altitude deviation.
Missing GST, or no valid GST update for five seconds, shows `ΔH N/A` and `ΔV N/A`.

When the observed BLE state changes, `BLE UP` or `BLE DOWN` replaces both accuracy lines for five seconds.
The accuracy lines then return. A new state change restarts the five-second overlay.
Booting without a BLE connection does not trigger an overlay.

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
| `BLE UP` / `BLE DOWN` | Five-second connection/disconnection overlay, not notification subscription or correction reception |

Checksum failures, malformed quality fields, and other NMEA messages do not refresh GGA status.
Freshness tracking does not depend on serial diagnostics. A new valid GGA replaces the stale indication.
The OLED does not confirm correction delivery or position accuracy. It adds no data to the BLE stream.
Only checksum-valid GST sentences with finite, nonnegative latitude, longitude, and altitude deviations update the accuracy values.
Malformed GST does not replace the last valid values or refresh their age.

## Failure behavior and checks

The firmware probes the display at startup. If it is absent, the firmware skips display updates and continues GNSS/BLE operation.
I2C operations use a five-millisecond driver timeout. Display transfers are synchronous and add loop latency, not a separate task.
If a later probe fails, updates stop until the ESP32 restarts. Hot-plug recovery is not supported.
The user confirmed clean, readable output on the 128×64 SSD1306 after correcting the earlier 64×48 configuration.
Sustained forwarding throughput with the display still requires hardware checks.
