# TODO

## LC29H(DA) GST output

GST availability on the LC29H(DA) depends on the receiver firmware. The [2026 Quectel forum thread](https://forums.quectel.com/t/nmea-gst-message-for-lc29h-da/43994/11) conflicts with older documentation. The bridge already forwards receiver sentences unchanged.

- [ ] Identify the installed firmware with `$PQTMVERNO*58\r\n`.
- [ ] Get GST-capable DA firmware, release notes, and a verified GST activation command from Quectel.
- [ ] Do not guess the activation command. `PAIR062` is not verified for the DA.
- [ ] After Quectel verifies the firmware and command, enable GST at every boot.
- [ ] Confirm GST sentences appear on the receiver UART.
- [ ] Confirm the accuracy fields in [SW Maps](https://aviyaantech.com/swmaps/).
