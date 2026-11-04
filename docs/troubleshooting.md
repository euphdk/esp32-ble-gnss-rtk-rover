# SW Maps, diagnostics, and validation

[Back to README](../README.md)

- [SW Maps setup](#sw-maps-setup)
- [Serial diagnostics](#serial-diagnostics)
- [Receiver checks](#receiver-checks)
- [Validation status](#validation-status)

## SW Maps setup

1. Connect to `GNSS-RTK-Rover-XXXX` through SW Maps' BLE GNSS/external receiver workflow.
2. Select NMEA input from the TX characteristic if the app offers characteristic selection.
3. Configure the correction source/NTRIP client in the app.
4. Select the RX characteristic as correction output if supported.
5. Confirm NMEA updates and RTK status on the receiver.
6. Compare the receiver's status output with the firmware status.

Exact menu names vary by app version. See the [BLE protocol](ble-protocol.md) for characteristic UUIDs.
`XXXX` is the last four uppercase hexadecimal digits of the rover's Bluetooth MAC address, including leading zeros.
The name stays the same after a restart. The startup console prints it as `[ble] name=...`.
For satellite and accuracy displays, use the [receiver's NMEA settings](receivers.md).

### Accuracy fields show N/A

SW Maps needs GGA and RMC for position and time, GSA and GSV for satellite data, and GST for accuracy.
Confirm that the receiver firmware supports GST and accepts the command to enable it.
Check for complete `$GNGST` or `$GPGST` sentences on the receiver UART and through BLE.
The serial console's 64-byte samples do not capture every sentence.

GST values describe receiver-estimated errors, not guaranteed real-world accuracy.
They do not establish that RTK corrections are active.

### Connected, but updates stop

If updates resume only while a serial monitor reads the C3 console, check for blocking USB diagnostic writes.
This caused a confirmed stall: an unread USB console delayed the main loop, which also handles GNSS and BLE forwarding.
The firmware now sets the native USB console transmit timeout to zero. Unread diagnostics can be dropped instead of delaying forwarding.
A connected USB cable does not mean a host application is reading the console.

For other connection problems, disconnect and reconnect SW Maps before restarting the rover.
Reconnection helped during earlier tests, but did not resolve the USB diagnostic stall.
ArduSimple's [BT+BLE Bridge guide](https://www.ardusimple.com/btble-bridge-hookup-guide/) also notes that the first connection after power-up can require two connection attempts.

Check the serial BLE counters as well. A BLE connection alone does not confirm notification subscription or delivery.

## Serial diagnostics

Serial diagnostics are enabled by default through `kSerialDebug` in `include/config.h`.

1. Upload the firmware for the selected environment. See [build and port commands](development.md).
2. Open its serial monitor at 115200 baud on the ESP32 console port.
3. Press the ESP32 reset button to see the startup baud rate and GPIO assignments.
4. Connect SW Maps and capture several seconds of console output.

For the ESP32-S3, use the USB-to-UART port. For the ESP32-C3 SuperMini, use the native USB CDC port.
The generic monitor command is `pio device monitor -b 115200`; specify the correct port when multiple devices are connected.

Each second, the console shows BLE connection state, GGA fix quality, sentence counts, checksum errors, and correction bytes.
Additional fields show:

| Field | Meaning |
| --- | --- |
| `gnss_rx_bytes` | Total bytes read from the receiver UART since boot |
| `nmea` | Total complete NMEA sentences with valid checksums |
| `ble_nmea_notify_calls` | NMEA chunk notification attempts, not byte counts, confirmed delivery, or client subscription |
| `last_rx_age_ms` | Time since the last UART byte |
| `sample_bytes` | Bytes captured for this sample, up to 64 per reporting interval |

`[gnss hex]` shows the sample as hexadecimal bytes.
`[gnss text]` shows the same bytes, with dots for nonprintable bytes.
Samples are partial data, not necessarily complete sentences. The console does not echo the full UART stream.

`[ble tx] errors` counts rejection callbacks, including attempts before subscription. `last_code` shows the latest rejection code.
On the ESP32-C3 NimBLE stack, code `6` means transmit-buffer exhaustion. Rejected chunks remain buffered for retry.
`[ble buffers]` shows free NimBLE packet buffers and free heap to help diagnose stalls.

- If `gnss_rx_bytes` stays zero, check receiver power, shared ground, and receiver TX to GNSS RX wiring.
  Use GPIO4 on the ESP32-C3 or GPIO16 on the ESP32-S3.
- If bytes arrive without valid NMEA, compare the receiver's UART1 baud rate with the startup log.
- If the text sample is unreadable, check baud rate and signal levels. Binary UBX output can also produce unreadable text.
- If `nmea` increases but `gga` stays zero, enable GGA output on receiver UART1.
- If `gga` increases with `fix=0`, check the antenna and sky view.
- If valid NMEA and notification attempts increase, check SW Maps' TX characteristic subscription and BLE compatibility.
- If `last_rx_age_ms` increases continuously, the receiver stream has stopped. The displayed fix value can be stale.

Set `kSerialDebug` to `false` and rebuild to disable periodic console diagnostics.
The startup free-heap message remains enabled. Diagnostics do not change the BLE payload format or prove that SW Maps accepts it.
Native USB console writes are non-blocking. Diagnostic output can be incomplete if the host does not keep up.

## Receiver checks

Check the following with a BLE client and the receiver's own diagnostics:

| Observation | Check |
| --- | --- |
| No NMEA data | Receiver power, shared ground, crossed TX/RX wires, UART1 baud rate, and UART1 NMEA message settings |
| Corrupt NMEA or checksum errors | Baud rate, logic voltage, wiring, and serial signal quality |
| `gga` increases, but `fix=0` | Antenna connection, sky view, and receiver fix status |
| Standalone fix, but no RTK | Correction source, RTCM compatibility, correction age, and UART1 RTCM input settings |
| `rtcm_bytes` increases, but no RTK | Receiver diagnostics for accepted RTCM messages and the resulting solution |

GGA fix quality normally reports `1` for standalone GNSS, `2` for differential GNSS, `4` for RTK fixed, and `5` for RTK float.
Serial diagnostics report the last parsed GGA value, which can be stale.
The OLED marks GGA status stale after five seconds without a valid update.

`rtcm_bytes` counts bytes accepted into the ESP32 correction buffer.
It does not confirm delivery to the receiver or successful RTCM decoding.
The firmware does not validate RTCM framing or guarantee RTK operation.

## Validation status

The earlier prototype documentation described all receiver hardware as unverified. Live checks now establish the following:

- ESP32-S3 and ESP32-C3 target builds compile, and host protocol tests pass.
- The user confirmed valid SW Maps positioning, incoming corrections, and RTK FLOAT operation with the C3 and LC29H(DA).
- The user confirmed live accuracy fields and continuous updates in iOS SW Maps after reconnecting with paced notifications.
- A later stall depended on the serial monitor: updates resumed while it read the console and stopped after it closed.
  After the USB transmit timeout changed to zero, the user confirmed continuous updates with all serial monitors closed.
- A direct Mac BLE capture received approximately 80 KB over 44 seconds with zero invalid NMEA checksums, including 44 GST sentences.
  This capture did not include incoming NTRIP corrections.
- Serial diagnostics showed receiver traffic and incoming corrections during a 90-second capture. BLE packet buffers recovered rather than remaining exhausted.
- The startup console reports free heap. This is not a battery or power measurement.

The user confirmed clean, readable output with the 128×64 SSD1306 attached after correcting the framebuffer dimensions.
Remaining hardware checks include long-duration reliability and sustained BLE throughput with the OLED attached.
The ESP32-S3 build remains unverified against the receiver hardware.

### Monitor-closed regression check

1. Upload the C3 firmware and close all serial monitors.
2. Keep USB connected, without a host application reading the console.
3. Connect SW Maps and enable NTRIP corrections.
4. Check that position, time, and accuracy continue updating for at least a minute.
5. Open a serial monitor and confirm updates continue.
6. Close the monitor and repeat the update check.

A test performed only with the monitor open does not cover this failure mode.
