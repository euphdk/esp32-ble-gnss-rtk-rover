# Battery power

[Back to README](../README.md) · [Hardware and wiring](hardware.md)

## XIAO ESP32-C6 with Grove Base

Use the Grove Base's battery connection for one rechargeable 3.7 V lithium cell.
Check the polarity, cell protection, and permitted charge current before connection.
Seeed lists a maximum charge current of 500 mA. Confirm the actual board revision and ratings.
The base supplies 3.3 V to the XIAO and Grove ports. Its charger and regulator need no firmware initialization.
Do not add the C3 plan's TP4056 and external 3.3 V regulator to this supply.

The XIAO C6 also has a charger and battery pads.
Leave those pads unused when the battery connects to the Grove Base.
Do not connect one cell to both chargers.
Check the base revision's power-path design before combining USB power, battery power, and receiver supplies.
Do not assume the base switch disconnects USB power or a separately powered receiver.

The user confirmed that the LC29H carrier works from 3.3 V.
Connect its `V` input to the Serial Grove port's VCC contact. No 5 V boost converter is required for this setup.
The OLED uses the I2C Grove port's VCC contact. Both ports supply 3.3 V.
Seeed lists an 800 mA load capacity for the base.
Check the combined XIAO, receiver, antenna, and OLED load against the actual board's rating.
Measure supply voltage and polarity before connecting the receiver.

The firmware does not measure battery voltage or report charge status.
See [Seeed's Grove Base guide](https://wiki.seeedstudio.com/Grove-Shield-for-Seeeduino-XIAO-embedded-battery-management-chip/)
and [XIAO C6 battery notes](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/#battery-usage).

## ESP32-C3 battery power plan

This plan uses one 18650 Li-ion cell, a TP4056-style charger with battery protection, and a regulator verified to output 3.3 V.
Confirm the remaining ratings below before assembly. This is not a verified complete power system.

```text
  5 V USB supply
       |
  charger USB-C input
  +------------------+
  | charger          |
  | B+  B-  OUT+ OUT-|
  +--|---|----|----|-+
     |   |    |    |
  cell+  |    |    +-------------------+------ C3 GND
  cell- -+    |                        +------ receiver GND
              |
              +---- switch --------- regulator VI
                                       regulator GND ---- OUT- / common ground
                                       regulator VO (3.3 V) --+-- C3 3V3
                                                            +-- LC29H(DA) V
```

Connect the cell only to `B+` and `B-`. Connect the load to `OUT+` and `OUT-` so it uses the charger's protection circuit.
The switch disconnects the regulator and its loads, not the charger or the cell.

The supplied charger product photo shows the USB-C connector at the top.
In that view, its bottom pads read left to right: `OUT+`, `B+`, `B-`, `OUT-`.
The top input pads are `+` on the left and `-` on the right. Check the labels on the actual board because other modules can differ.

The supplied regulator photo shows the label side with the pins pointing right.
From top to bottom, the labels are `VO`, `GND`, `VI`.
Connect `VI` to the switched charger output, `GND` to `OUT-`, and `VO` to the C3 `3V3` pin.
Do not use the C3 `5V` pin for this supply.

The user confirmed that the LC29H carrier works from 3.3 V. Connect its `V` input to the regulated 3.3 V supply.
Rate that supply for the combined C3, OLED, receiver, and antenna load.
The ZED-F9P input must exceed 3.3 V by enough to account for regulator dropout. Its exact input voltage range is unknown.
Do not connect the ZED-F9P carrier's power input to the external 3.3 V regulator output.

The switched `OUT+` supply is approximately the cell voltage, about 3.0–4.2 V, not regulated 3.3 V.
Do not connect it directly to the LC29H carrier without confirming the carrier's full input voltage range.

The C3 uses GPIO4 for GNSS RX and GPIO5 for GNSS TX. If enabled, the OLED uses GPIO6 for SDA and GPIO7 for SCL.
Connect the LC29H(DA) carrier's `G` contact to `OUT-`.

## Checks before assembly

- Confirm the cell chemistry, charge voltage, permitted charge current, polarity, and protection requirements.
- Confirm the charger's actual charge current and protection functions.
- Confirm the regulator's input range, output current, dropout or conversion behavior, and stability across the cell voltage range.
  A verified 3.3 V output alone does not establish these ratings.
- Measure the regulator output and polarity without the C3 or receiver connected. Check the output again under the intended load.
- Do not assume the charger supports powering the load while charging. Use a verified power-path circuit or switch the load off during charging.
- Do not connect the C3 USB supply and external 3.3 V at the same time unless the board's power path is verified safe against backfeeding.

Battery voltage measurement would need a divider to keep the ADC input within its safe range.
The current firmware does not measure battery voltage. Do not connect the cell directly to an ESP32 GPIO.
