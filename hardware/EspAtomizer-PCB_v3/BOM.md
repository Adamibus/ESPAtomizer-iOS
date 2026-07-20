# ESP Atomizer PCB v3 BOM

Date: 2026-07-20

This BOM is derived from `hardware/EspAtomizer-PCB_v3/EspAtomizer-PCB_v3.kicad_pcb` and matches the current PCB v3 assembly. It excludes logos, mounting holes, and other non-assembly artifacts.

## Required Parts

| Qty | References | Part / Value | Footprint | Notes |
|---:|---|---|---|---|
| 1 | U3 | Seeed Studio XIAO ESP32-C6 module | `XIAO:XIAOESP32C6_CUSTOM` | PCB footprint is C3/C6-compatible; populate with the module used by firmware. |
| 1 | U1 | ADS1115IDGS | `Package_SO:TSSOP-10_3x3mm_P0.5mm` | I2C thermocouple ADC. |
| 1 | Q1 | PSMN1R0-30YLDX | `Package_TO_SOT_SMD:LFPAK56` | Heater MOSFET. |
| 1 | SW2 | Rotary encoder with push switch | `Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm` | User control encoder. |
| 1 | SW1 | SPST slide switch | `footprints:SW_JS102011SAQN` | Main board switch. |
| 1 | D2 | SS34 or 1N5819 | `Diode_SMD:D_0805_2012Metric_Pad1.15x1.40mm_HandSolder` | Schottky protection diode. |
| 1 | D1 | WS2812B-2020 | `LED_SMD:LED_WS2812B-2020_PLCC4_2.0x2.0mm` | RGB status LED, optional for firmware features. |
| 1 | J1 | JST-PH 2-pin thermocouple connector | `Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical` | Thermocouple input. |
| 1 | J2 | JST-PH 2-pin battery connector | `Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical` | Battery input. |
| 1 | J4 | JST-PH 2-pin heater connector | `Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical` | Heater/load output. |
| 1 | J3 | 1x04 through-hole socket/header | `Socket_Strips:Socket_Strip_Straight_1x04_Pitch2.54mm` | OLED header. |

## Passive Parts

| Qty | References | Part / Value | Footprint | Notes |
|---:|---|---|---|---|
| 3 | C1, C2, C3 | 100 nF | `Capacitor_SMD:C_0805_2012Metric_Pad1.18x1.45mm_HandSolder` | Local decoupling / filtering. |
| 1 | C5 | 100 uF tantalum | `Capacitor_Tantalum_SMD:CP_EIA-3528-12_Kemet-T` | Bulk decoupling on the power rail. |
| 3 | R2, R3, R4 | 100 kΩ | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | Battery divider / bias network. |
| 2 | R8, R9 | 4.7 kΩ | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | I2C pull-ups. |
| 1 | R1 | 100 Ω | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | MOSFET gate series resistor. |

## Assembly Notes

- The PCB uses a 3.3 V XIAO C3/C6-compatible footprint, but the current documentation and firmware target the XIAO ESP32-C6.
- If you do not need the RGB indicator, D1 can be left unpopulated.
- J3 is the OLED header; J4 is the heater/load connector.
- The ADS1115 uses I2C with the OLED display, so R8 and R9 are the shared bus pull-ups.
