# ESP32-C3 battery power plan

[Back to README](../README.md) · [Hardware and wiring](hardware.md)

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
              |                        |
              +---- switch ----+---- regulator VI
                               |       regulator GND ---- OUT- / common ground
                               |       regulator VO (3.3 V) ---- C3 3V3
                               |
                               +---- [optional 5 V boost] ---- LC29H(DA) V
                                     (see conditions below)
```

Connect the cell only to `B+` and `B-`. Connect the load to `OUT+` and `OUT-` so it uses the charger's protection circuit.
The switch disconnects both load branches, not the charger or the cell.

The supplied charger product photo shows the USB-C connector at the top.
In that view, its bottom pads read left to right: `OUT+`, `B+`, `B-`, `OUT-`.
The top input pads are `+` on the left and `-` on the right. Check the labels on the actual board because other modules can differ.

The supplied regulator photo shows the label side with the pins pointing right.
From top to bottom, the labels are `VO`, `GND`, `VI`.
Connect `VI` to the switched charger output, `GND` to `OUT-`, and `VO` to the C3 `3V3` pin.
Do not use the C3 `5V` pin for this supply.

Both receiver carriers have their own 3.3 V regulators. Do not connect their power inputs to the external 3.3 V regulator output.
The ZED-F9P input must exceed 3.3 V by enough to account for regulator dropout. Its exact input voltage range is unknown.

The switched `OUT+` supply is approximately the cell voltage, about 3.0–4.2 V, not boosted 5 V.
For the LC29H(DA), connect switched `OUT+` to a suitable 5 V boost converter input.
Connect its output to the carrier's `V` contact. The carrier accepts 5 V, but its maximum input voltage is not confirmed.
Omit the boost only if the carrier's input range covers the full cell voltage range.
Use a boost converter rated for the cell voltage range and receiver load. Confirm the ZED-F9P `5V` pin rating before using it.

The C3 uses GPIO4 for GNSS RX and GPIO5 for GNSS TX. If enabled, the OLED uses GPIO6 for SDA and GPIO7 for SCL.
Connect the LC29H(DA) carrier's `G` contact and any boost converter ground to `OUT-`.

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
