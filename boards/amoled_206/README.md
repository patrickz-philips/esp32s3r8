# Waveshare ESP32-S3 Touch AMOLED 2.06

## Profile

This project profile follows the APIs and pin definitions in the resolved
`waveshare/esp32_s3_touch_amoled_2_06` version `2.0.0` component. That component
initializes an SH8601 QSPI display and an FT5x06-compatible touch controller.
The current Waveshare product repository describes a CO5300 display and FT3168
touch controller, so confirm the physical board revision before changing or
upgrading the BSP.

| Item | Project Configuration |
|:-----|:----------------------|
| MCU | ESP32-S3, dual-core Xtensa LX7 up to 240 MHz |
| Memory | Octal PSRAM enabled; project profile currently selects 32 MB flash |
| Display | 2.06-inch 410 x 502 QSPI AMOLED; resolved BSP uses SH8601 |
| Touch | Resolved BSP uses an FT5x06-compatible I2C driver |
| Storage | microSD, native 1-bit SDMMC |
| Power | AXP2101 PMIC, I2C address `0x34` |
| Motion | QMI8658 6-axis IMU, I2C address `0x6B` |
| Audio | ES8311 output codec and ES7210 microphone ADC |
| BSP | `waveshare/esp32_s3_touch_amoled_2_06` version `2.0.0` |

## GPIO Allocation

The table records every GPIO assigned by the resolved official BSP header. GPIO
assignments not present in that header are not inferred from another board or a
newer hardware revision.

**Direction:** `In` = input, `Out` = output, `I/O` = bidirectional.  
**Source:** `[BSP]` = [resolved BSP pin header][bsp].

### Storage and Shared I2C

| GPIO | Signal/Function | Peripheral/Sensor | Bus | Direction | Shared/Conflict | Source |
|:-----|:----------------|:------------------|:----|:----------|:----------------|:-------|
| 1 | `SD_CMD` | microSD | SDMMC | I/O | Reserved while mounted | [BSP] |
| 2 | `SD_CLK` | microSD | SDMMC | Out | Reserved while mounted | [BSP] |
| 3 | `SD_D0` | microSD | SDMMC | I/O | 1-bit data line | [BSP] |
| 14 | `I2C_SCL` | All onboard I2C devices | I2C | I/O | Shared touch/PMU/IMU/audio bus | [BSP] |
| 15 | `I2C_SDA` | All onboard I2C devices | I2C | I/O | Shared; check address conflicts | [BSP] |

### Display and Touch

| GPIO | Signal/Function | Peripheral/Sensor | Bus | Direction | Shared/Conflict | Source |
|:-----|:----------------|:------------------|:----|:----------|:----------------|:-------|
| 4 | `LCD_DATA0` | SH8601 display | QSPI | I/O | Display bus | [BSP] |
| 5 | `LCD_DATA1` | SH8601 display | QSPI | I/O | Display bus | [BSP] |
| 6 | `LCD_DATA2` | SH8601 display | QSPI | I/O | Display bus | [BSP] |
| 7 | `LCD_DATA3` | SH8601 display | QSPI | I/O | Display bus | [BSP] |
| 8 | `LCD_RST` | SH8601 display | GPIO | Out | Display reset | [BSP] |
| 9 | `LCD_TOUCH_RST` | Touch controller | GPIO | Out | Touch reset | [BSP] |
| 11 | `LCD_PCLK` | SH8601 display | QSPI | Out | Display clock | [BSP] |
| 12 | `LCD_CS` | SH8601 display | QSPI | Out | Chip select | [BSP] |
| 38 | `LCD_TOUCH_INT` | Touch controller | GPIO IRQ | In | Touch interrupt | [BSP] |

### Audio

| GPIO | Signal/Function | Peripheral/Sensor | Bus | Direction | Shared/Conflict | Source |
|:-----|:----------------|:------------------|:----|:----------|:----------------|:-------|
| 16 | `I2S_MCLK` | ES8311 + ES7210 | I2S | Out | Shared master clock | [BSP] |
| 40 | `I2S_DOUT` | ES8311 codec | I2S | Out | Playback data | [BSP] |
| 41 | `I2S_SCLK` | ES8311 + ES7210 | I2S | Out | Shared bit clock | [BSP] |
| 42 | `I2S_DSIN` | ES7210 ADC | I2S | In | Capture data | [BSP] |
| 45 | `I2S_LCLK` | ES8311 + ES7210 | I2S | Out | Shared frame clock | [BSP] |
| 46 | `POWER_AMP_IO` | Audio amplifier | GPIO | Out | BSP owns active level | [BSP] |

## Sensors and Peripherals

### Display and Touch

| Device | Interface | Control | GPIO | Role |
|:-------|:----------|:--------|:-----|:-----|
| SH8601 display | QSPI, CS 12 | RST 8 | 4-8, 11, 12 | BSP display controller |
| FT5x06-compatible touch | I2C `0x38` | INT 38, RST 9 | 9, 14, 15, 38 | BSP touch controller |

### Power and Motion

| Device | Interface | Control | GPIO | Role |
|:-------|:----------|:--------|:-----|:-----|
| AXP2101 PMIC | I2C `0x34` | Register IRQ | 14, 15 | Power and telemetry |
| QMI8658 IMU | I2C `0x6B` | IRQ unused | 14, 15 | 6-axis motion |

The PMIC uses the local XPowersLib wrapper. The IMU uses the managed QMI8658
component; no IMU interrupt line is consumed by the current apps.

### Audio and Storage

| Device | Interface | Control | GPIO | Role |
|:-------|:----------|:--------|:-----|:-----|
| ES8311 codec | I2C `0x18`, I2S | N/A | 14-16, 40, 41, 45, 46 | Speaker output |
| ES7210 ADC | I2C `0x40`, I2S | N/A | 14-16, 41, 42, 45 | Microphone input |
| microSD | 1-bit SDMMC | No CD/WP | 1, 2, 3 | BSP storage |

## Project Compatibility

The source of truth is [supported_apps.txt](supported_apps.txt). All current apps
are selected as compatible with this project profile:

- `slide_player`: display, touch, SD, and PNG/JPEG decoding.
- `salary_cat`: display, SD, ES8311 audio, and AXP2101 power control.
- `acc_data`: display, SD, QMI8658, AXP2101, and current GPIO controls.
- `battery_monitor`: display and AXP2101 telemetry.

## Build Configuration and Limits

- The board sdkconfig currently selects 32 MB flash and a partition table with
  an 8 MB factory app plus 23 MB SPIFFS. Current upstream examples commonly
  select 16 MB flash. Verify the fitted flash before flashing this profile.
- The display is QSPI, not an RGB/DPI panel. Do not enable RGB-panel tearing
  avoidance in `esp_lvgl_port`.
- `CONFIG_BSP_DISPLAY_LVGL_BUF_HEIGHT=60` reduces internal DMA-buffer pressure.
- The resolved BSP ignores most fields passed to `bsp_display_start_with_config`.
  Project-specific buffer fixes currently present under `managed_components/`
  are generated-state changes and must be converted to a version-controlled
  override before relying on a clean checkout.
- `acc_data` currently drives GPIO18 as a haptic output, but GPIO18 is not
  declared by the resolved BSP header. Verify this connection against the exact
  board schematic before treating haptic behavior as validated.

## References

- [Resolved BSP source revision][bsp-root]
- [Resolved BSP pin header][bsp]
- [Current official product repository][repo]
- [ESP Component Registry BSP package][registry]

[bsp-root]: https://github.com/waveshareteam/Waveshare-ESP32-components/tree/9daffb7168b3c093a330da700988a22092294eef/bsp/esp32_s3_touch_amoled_2_06
[bsp]: https://github.com/waveshareteam/Waveshare-ESP32-components/blob/9daffb7168b3c093a330da700988a22092294eef/bsp/esp32_s3_touch_amoled_2_06/include/bsp/esp32_s3_touch_amoled_2_06.h
[repo]: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-2.06
[registry]: https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_2_06