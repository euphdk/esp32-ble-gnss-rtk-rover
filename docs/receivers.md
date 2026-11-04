# Receiver configuration

[Back to README](../README.md)

- [LC29H(DA)](#lc29hda)
- [ZED-F9P](#zed-f9p)

## LC29H(DA)

Use `esp32-c3-lc29h-da-oled` for the final hardware target.
The non-display environments are `esp32-c3-lc29h-da` and `esp32-s3-lc29h-da`. Other LC29H variants are not covered.
These builds use **115200 baud, 8N1**. They keep the selected ESP32 board's UART pins and BLE service unchanged.
Connect receiver TX to ESP32 GNSS RX, receiver RX to ESP32 GNSS TX, and connect both grounds.
Use 3.3 V UART logic. Check the LC29H(DA) carrier's power ratings and connector labels before connection.
The ZED-F9P power instructions and connector pinout do not describe the LC29H(DA) carrier.

### Startup commands

On every ESP32 boot, the firmware waits 1200 ms after UART startup, then sends these command bodies in order:

```text
PQTMVERNO
PQTMCFGRCVRMODE,W,1
PQTMCFGRTK,W,1,1
PQTMCFGPROT,W,1,1,00000005,00000005
PQTMCFGMSGRATE,W,GGA,1
PQTMCFGMSGRATE,W,RMC,1
PQTMCFGMSGRATE,W,GSA,1
PQTMCFGMSGRATE,W,GSV,1
PQTMCFGMSGRATE,W,GST,1
PQTMCFGMSGRATE,W,VTG,0
PQTMCFGMSGRATE,W,GLL,0
PQTMCFGMSGRATE,W,ZDA,0
```

Each command uses `$body*XX\r\n` framing. `XX` is the two-digit hexadecimal XOR checksum of the body.
The firmware waits 250 ms between commands and for 250 ms after the last command.
BLE starts after this sequence, approximately 4.20 seconds after UART startup.
During these waits, the firmware drains receiver UART bytes and prints them to the serial console if serial diagnostics are enabled.
`PQTMVERNO` requests the firmware version. The configuration commands come from the supplied LC29H(DA) reference project.
The firmware sends them again because they are not persisted.

The firmware does not validate replies or retry commands. A short UART write stops setup and prints a console error, but BLE still starts.
The success log confirms writes only, not receiver acceptance. Inspect the startup replies for acceptance or errors.
If the version query produces no reply, check the ESP32 TX connection to the receiver's command RX input and carrier UART routing.
Power the receiver before resetting the ESP32. After a receiver-only reset, reset the ESP32 to send the commands again.

The installed firmware returned `PQTMCFGPROT,ERROR,3` for the protocol command above.
That command remains unchanged. Do not assume it configured the port successfully.

### NMEA and GST

This message set requests GGA, RMC, GSA, GSV, and GST once per navigation cycle.
GST support depends on receiver firmware. Check for `$GNGST` or `$GPGST` output and the accuracy fields in SW Maps.
Verified on firmware `LC29HDANR11A05S_RSA` (2026/07/01): the GST command returns `OK`, and the receiver outputs `$GNGST`.
The ZED-F9P `CFG-MSGOUT-NMEA_ID_GST_UART1` setting does not apply to the LC29H(DA).

The bridge forwards all receiver bytes, including command responses, without filtering.
Supply RTCM corrections through the BLE client. The firmware adds no Wi-Fi or NTRIP client.
See [hardware validation and remaining checks](troubleshooting.md#validation-status) for the tested setup and limitations.

## ZED-F9P

The original `esp32-s3` and `esp32-c3` environments select ZED-F9P without startup commands.
Configure the receiver through its USB connection with u-blox u-center or another compatible configuration tool.

Factory UART1 settings are normally **38400 baud, 8N1**, with NMEA output and RTCM 3 input enabled.
A seller or previous owner can change these settings. This firmware expects **38400 baud, 8N1** by default.

1. Connect to the receiver through USB and read its firmware version and current configuration.
2. Set receiver UART1 to 38400 baud, 8 data bits, no parity, and one stop bit.
3. Enable RTCM 3 input on UART1 (`CFG-UART1INPROT-RTCM3X`).
4. Enable NMEA output on UART1 (`CFG-UART1OUTPROT-NMEA`).
5. Enable the UART1 NMEA messages listed below.
6. Disable periodic UBX and RTCM output on UART1 to keep the rover's output stream NMEA-only.
7. Disable time mode (`CFG-TMODE-MODE = 0`) for rover use. Do not enable base-station survey-in or fixed-position mode.
8. Start with a 1 Hz navigation rate and a small NMEA message set to limit BLE traffic.
9. Save the configuration to supported persistent storage, not only RAM.
10. Restart the receiver and verify that the settings persist.

### UART1 NMEA messages

[SW Maps documents](https://aviyaantech.com/swmaps/) these NMEA sentence requirements:

| Sentences | SW Maps display | UART1 configuration |
| --- | --- | --- |
| GGA, RMC | Position and time | Enable both messages in the message-output settings |
| GSA, GSV | Satellite status and skyplot | Enable both messages in the message-output settings |
| GST | Latitude Error, Longitude Error, Horizontal Accuracy, and Vertical Accuracy | Set `CFG-MSGOUT-NMEA_ID_GST_UART1` to `1` |

The GST rate of `1` means output once per navigation cycle. Save these message settings with the rest of the receiver configuration.

If accuracy fields show `N/A`, verify `$GNGST` or `$GPGST` output on **UART1**, not only USB.
USB output settings are separate. Reconnect SW Maps and check the accuracy fields under **GNSS Status**.

The bridge already forwards GST without a firmware change. Its periodic serial samples capture only part of the stream.
A missing GST sentence in those samples does not prove that UART1 GST output is absent.
GST values describe receiver-estimated errors, not guaranteed real-world accuracy. A standalone GNSS fix can provide error estimates without RTK corrections.

### Baud rate, rover operation, and precision

Configuration names and tool menus depend on the receiver firmware.
If you change the receiver's UART1 baud rate, change `kGnssBaud` in `include/gnss_receiver.h` to match.
The serial console remains at 115200 baud.
UART1 pin numbers come from each environment's `GNSS_RX_PIN` and `GNSS_TX_PIN` build flags in `platformio.ini`.
`include/config.h` keeps the ESP32-S3 pair as fallback defaults.

UART2 normally serves correction traffic and does not output NMEA by default.
Connecting this bridge to `TX2`/`RX2` without receiver configuration will not provide the expected NMEA stream.

For RTK, supply compatible RTCM 3 corrections from an NTRIP service or base station.
The correction stream must cover the constellations and signals used by the receiver.
The ZED-F9P computes the RTK solution, not the ESP32.

For higher position precision in NMEA, consider `CFG-NMEA-HIGHPREC` after checking the client application's support.
The status parser discards sentences longer than 127 characters. Raw UART bytes still pass to BLE.

References: [ZED-F9P-01B data sheet](https://content.u-blox.com/sites/default/files/documents/ZED-F9P-01B_DataSheet_UBX-17051259.pdf)
and [ZED-F9P integration manual](https://content.u-blox.com/sites/default/files/ZED-F9P_IntegrationManual_UBX-18010802.pdf).
Use the interface description that matches the installed receiver firmware.
