# BLE GATT protocol

[Back to README](../README.md)

The firmware provides a bidirectional UART bridge over BLE, without Bluetooth Classic.
The BLE RX characteristic accepts correction bytes for the receiver. The BLE TX characteristic provides only receiver UART bytes.
Status output uses the serial console and [OLED](display.md).

## Service and characteristics

Service UUID: `6e400001-b5a3-f393-e0a9-e50e24dcca9e` (Nordic-UART-style UUIDs).

| Characteristic | UUID | Properties |
| --- | --- | --- |
| RX, client to receiver | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | Write, write without response |
| TX, receiver to client | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | Notifications, read |

Each RX write payload is forwarded unchanged to GNSS UART.
Payloads are opaque binary, not UTF-8 or NMEA. The firmware does not validate RTCM framing.

## Notification stream

Receiver UART bytes use chunks of up to 20 bytes, compatible with the default ATT MTU of 23 bytes.
Full chunks send with at least five milliseconds between notification attempts.
After each UART drain, a partial chunk sends if at least 20 ms have elapsed since the previous notification attempt.

Concatenate notifications to reconstruct the stream, including original CR/LF sentence endings.
A notification can contain part of a sentence or cross a sentence boundary.
Reads return the latest chunk, not a complete sentence or history. No diagnostic status text is inserted into this stream.

If BLE rejects a chunk, the bridge retains it and pauses UART reads until BLE accepts it.
A 4096-byte UART receive buffer absorbs receiver bursts during these pauses.
Sustained congestion can still overflow that buffer. Disconnecting clears the pending BLE chunk.

## Limits

Use the configuration for [LC29H(DA)](receivers.md#lc29hda) or [ZED-F9P](receivers.md#zed-f9p).
The bridge does not filter binary output or change talker IDs.
It does not replay bytes received while disconnected or guarantee notification delivery.
Batching reduces notification overhead. See [validation status](troubleshooting.md#validation-status) for live test results and remaining checks.
