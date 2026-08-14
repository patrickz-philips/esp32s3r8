# Waveshare ESP32-S3 Touch AMOLED 2.06

## Profile

This profile uses a version-controlled override of
`waveshare/esp32_s3_touch_amoled_2_06` version `2.0.0`. The override keeps the
vendor BSP API but drives the physical CO5300 QSPI panel and synchronizes LVGL
flushes to the panel TE output on GPIO13. Touch remains on the vendor's
FT5x06-compatible driver for the physical FT3168 controller.

| Item | Project Configuration |
|:-----|:----------------------|
| MCU | ESP32-S3, dual-core Xtensa LX7 up to 240 MHz |
| Memory | Octal PSRAM enabled; project profile currently selects 32 MB flash |
| Display | CO5300, 2.06-inch 410 x 502 RGB565 QSPI AMOLED, TE synchronized |
| Touch | FT3168 through the FT5x06-compatible I2C driver |
| Storage | microSD, native 1-bit SDMMC |
| Power | AXP2101 PMIC, I2C address `0x34` |
| Motion | QMI8658 6-axis IMU, I2C address `0x6B` |
| Audio | ES8311 output codec and ES7210 microphone ADC |
| BSP | Waveshare `2.0.0`, project override revision 1 |

## GPIO Allocation

The table records the vendor BSP assignments plus the product signals verified
for this board integration.

**Direction:** `In` = input, `Out` = output, `I/O` = bidirectional.  
**Source:** `[BSP]` = [project BSP override][override]; `[HW]` = [official
product repository][repo]; `[DEV]` = verified behavior in this project.

### System, Storage, and Interrupts

| GPIO | Signal/Function | Peripheral/Sensor | Bus | Direction | Shared/Conflict | Source |
|:-----|:----------------|:------------------|:----|:----------|:----------------|:-------|
| 0 | `BOOT` | Boot button | GPIO/strap | In | Boot strapping pin | [HW] |
| 1 | `SD_CMD` | microSD | SDMMC | I/O | Reserved while mounted | [BSP] |
| 2 | `SD_CLK` | microSD | SDMMC | Out | Reserved while mounted | [BSP] |
| 3 | `SD_D0` | microSD | SDMMC | I/O | 1-bit data line | [BSP] |
| 10 | `PMU_SYS_OUT` | AXP2101 | GPIO | In | Current apps poll PMU over I2C | [HW] |
| 14 | `I2C_SCL` | All onboard I2C devices | I2C | I/O | Shared touch/PMU/IMU/audio bus | [BSP] |
| 15 | `I2C_SDA` | All onboard I2C devices | I2C | I/O | Shared; check address conflicts | [BSP] |
| 17 | `SD_DAT3` / `SD_CS` | microSD | SDMMC/SPI | I/O | Unused by current 1-bit SDMMC mount | [HW] |
| 18 | `HAPTIC` | Haptic actuator | GPIO | Out | App-controlled feedback | [DEV] |
| 19 | `USB_D-` | ESP32-S3 USB | USB | I/O | Native USB | [HW] |
| 20 | `USB_D+` | ESP32-S3 USB | USB | I/O | Native USB | [HW] |
| 21 | `IMU_INT1` | QMI8658C | GPIO IRQ | In | Current app polls over I2C | [HW] |
| 39 | `RTC_INT` | PCF85063 | GPIO IRQ | In | RTC interrupt not yet consumed | [HW] |
| 43 | `UART0_TX` | ESP32-S3 UART0 | UART | Out | Console/programming | [HW] |
| 44 | `UART0_RX` | ESP32-S3 UART0 | UART | In | Console/programming | [HW] |

### Display and Touch

| GPIO | Signal/Function | Peripheral/Sensor | Bus | Direction | Shared/Conflict | Source |
|:-----|:----------------|:------------------|:----|:----------|:----------------|:-------|
| 4 | `LCD_DATA0` | CO5300 display | QSPI | I/O | Display bus | [BSP] |
| 5 | `LCD_DATA1` | CO5300 display | QSPI | I/O | Display bus | [BSP] |
| 6 | `LCD_DATA2` | CO5300 display | QSPI | I/O | Display bus | [BSP] |
| 7 | `LCD_DATA3` | CO5300 display | QSPI | I/O | Display bus | [BSP] |
| 8 | `LCD_RST` | CO5300 display | GPIO | Out | Display reset | [BSP] |
| 9 | `LCD_TOUCH_RST` | Touch controller | GPIO | Out | Touch reset | [BSP] |
| 11 | `LCD_PCLK` | CO5300 display | QSPI | Out | Display clock | [BSP] |
| 12 | `LCD_CS` | CO5300 display | QSPI | Out | Chip select | [BSP] |
| 13 | `LCD_TE` | CO5300 display | GPIO IRQ | In | Adapter TE synchronization | [HW][BSP] |
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
| CO5300 display | QSPI, CS 12 | RST 8, TE 13 | 4-8, 11-13 | BSP display controller |
| FT5x06-compatible touch | I2C `0x38` | INT 38, RST 9 | 9, 14, 15, 38 | BSP touch controller |

### Power and Motion

| Device | Interface | Control | GPIO | Role |
|:-------|:----------|:--------|:-----|:-----|
| AXP2101 PMIC | I2C `0x34` | SYS_OUT 10 | 10, 14, 15 | Power and telemetry |
| QMI8658C IMU | I2C `0x6B` | INT1 21 | 14, 15, 21 | 6-axis motion |
| PCF85063 RTC | I2C | INT 39 | 14, 15, 39 | Real-time clock |

The PMIC uses the local XPowersLib wrapper. Current PMU and IMU services poll
over I2C; the newly defined GPIOs make their interrupt lines available without
changing those services.

### Audio and Storage

| Device | Interface | Control | GPIO | Role |
|:-------|:----------|:--------|:-----|:-----|
| ES8311 codec | I2C `0x18`, I2S | N/A | 14-16, 40, 41, 45, 46 | Speaker output |
| ES7210 ADC | I2C `0x40`, I2S | N/A | 14-16, 41, 42, 45 | Microphone input |
| microSD | 1-bit SDMMC | DAT3/CS 17 unused | 1, 2, 3, 17 | BSP storage |

## Project Compatibility

The source of truth is [supported_apps.txt](supported_apps.txt). All current apps
are selected as compatible with this project profile:

- `slide_player`: display, touch, SD, and PNG/JPEG decoding.
- `salary_cat`: display, SD, ES8311 audio, and AXP2101 power control.
- `acc_data`: display, SD, QMI8658, AXP2101, GPIO0 BOOT input, and GPIO18 haptic feedback.
- `battery_monitor`: display and AXP2101 telemetry.

## Build Configuration and Limits

- The board sdkconfig currently selects 32 MB flash and a partition table with
  an 8 MB factory app plus 23 MB SPIFFS. Current upstream examples commonly
  select 16 MB flash. Verify the fitted flash before flashing this profile.
- The display is QSPI, not RGB/DPI. The override uses the CO5300 driver and
  `ESP_LV_ADAPTER_TEAR_AVOID_MODE_TE_SYNC` with GPIO13.
- `CONFIG_BSP_DISPLAY_LVGL_BUF_HEIGHT=60` selects a DMA-capable internal draw
  stripe. Dirty-area stripes from one LVGL refresh share one TE wait and do not
  use RGB frame buffer switching. Salary-cat uses a 144-line internal draw
  buffer so its rounded 242 x 242 GIF area is fully rendered before TE and sent
  in one color transaction.
- The BSP source is tracked under `boards/amoled_206/bsp_override`; generated
  files under `managed_components/` are not patched.
- The acc-data app pins the BSP-managed LVGL task to core 0 on this profile,
  preserving the display scheduling used by the original 2.06 integration.
- GPIO18 haptic feedback is enabled from device-verified project behavior. It is
  not declared by the resolved BSP header and must be revalidated on a different
  board revision before reuse.

## References

- [Resolved BSP source revision][bsp-root]
- [Resolved BSP pin header][bsp]
- [Project BSP override][override]
- [Current official product repository][repo]
- [ESP Component Registry BSP package][registry]
- [ESP LCD CO5300 component][co5300]
- [ESP LVGL adapter component][adapter]

[bsp-root]: https://github.com/waveshareteam/Waveshare-ESP32-components/tree/9daffb7168b3c093a330da700988a22092294eef/bsp/esp32_s3_touch_amoled_2_06
[bsp]: https://github.com/waveshareteam/Waveshare-ESP32-components/blob/9daffb7168b3c093a330da700988a22092294eef/bsp/esp32_s3_touch_amoled_2_06/include/bsp/esp32_s3_touch_amoled_2_06.h
[override]: bsp_override/waveshare__esp32_s3_touch_amoled_2_06
[repo]: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-2.06
[registry]: https://components.espressif.com/components/waveshare/esp32_s3_touch_amoled_2_06
[co5300]: https://components.espressif.com/components/espressif/esp_lcd_co5300/versions/2.1.0
[adapter]: https://components.espressif.com/components/espressif/esp_lvgl_adapter/versions/0.6.3