# ESP32 BLE GNSS RTK rover prototype

This firmware is a non-blocking Arduino prototype for an ESP32 DevKit and a UART GNSS receiver. It exposes a BLE GATT UART-style service: write correction bytes to the RX characteristic and read/subscribe to the TX characteristic for GNSS NMEA bytes and periodic status. It uses BLE only; Bluetooth Classic is not enabled.

## Wiring

| ESP32 | GNSS receiver |
| --- | --- |
| GPIO16 (RX2) | GNSS TX |
| GPIO17 (TX2) | GNSS RX |
| GND | GND |

Use compatible logic levels and power for the specific receiver. Configure `kGnssBaud` and pin constants in `include/config.h` to match the hardware. The default is 115200 baud. UART2 is dedicated to the receiver; USB serial is diagnostic output.

## Build and host tests

Install PlatformIO Core, then run `pio run -e esp32dev` to build and `pio run -e esp32dev -t upload` to upload. `pio device monitor -b 115200` shows startup diagnostics, including the free-heap measurement taken at boot. Run host-only protocol tests with `sh test/host/run.sh`.

The project pins PlatformIO's Espressif32 platform to `~8.0.0`; BLE uses the Arduino ESP32 core's bundled `BLEDevice` GATT API. Host tests cover NMEA checksum/fix parsing and binary RTCM byte-preserving forwarding, including short writes. The host tests do not compile or emulate the ESP32 BLE stack.

## GATT protocol

Service UUID: `6e400001-b5a3-f393-e0a9-e50e24dcca9e` (Nordic-UART-style UUIDs).

* RX `6e400002-b5a3-f393-e0a9-e50e24dcca9e`: write or write-without-response. Each write payload is forwarded unchanged to GNSS UART; payloads are treated as opaque binary (not UTF-8 or NMEA).
* TX `6e400003-b5a3-f393-e0a9-e50e24dcca9e`: notifications/read. NMEA is notified as individual bytes. Once per second the characteristic is set to a text status line: BLE connection state, GGA fix quality, GGA count, checksum errors, and forwarded RTCM bytes. A status update replaces the most recent TX value. This prototype is for evaluation, not a framed or lossless telemetry protocol.

## SW Maps setup checklist

1. Pair/connect to `GNSS-RTK-Rover` using SW Maps' BLE GNSS/external receiver workflow (exact menu names vary by app version).
2. Select the BLE device and configure NMEA input from the TX characteristic if SW Maps offers characteristic selection.
3. Configure the correction source/NTRIP client in the app and select the RX characteristic as correction output if supported.
4. Confirm NMEA updates and RTK status on the receiver; compare the receiver's own status output with the firmware status.

The GATT service layout is implemented in source, but SW Maps interoperability and its ability to select the two characteristics have not been verified on hardware. Some app flows expect a standard BLE GNSS profile rather than a generic UART service.

## LC29H limitations and verification

The LC29H's exact UART defaults, supported NMEA sentences, correction protocol/configuration, and RTK fix reporting depend on module firmware and configuration. Check the module documentation and configure baud/protocol accordingly. This firmware forwards arbitrary correction payloads but does not validate RTCM framing, configure the receiver, or guarantee RTK operation.

Verified in this repository: host protocol tests and source-level BLE/UART implementation. Hardware-dependent and not verified here: PlatformIO target build (unless successful in your environment), BLE connection behavior, notification throughput/fragmentation, LC29H compatibility, SW Maps workflow, RTCM acceptance, and runtime free heap. A boot-time free-heap value is printed to serial; no hardware measurement is available from this implementation environment.
