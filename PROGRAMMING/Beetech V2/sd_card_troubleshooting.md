# SD Card Troubleshooting — ESP32-S3 Deep Sleep

## Problem

SD card initialization fails after ESP32-S3 wakes up from deep sleep. First boot (fresh card insert) works, but subsequent boots after a deep sleep cycle fail with `SD card initialization failed`.

Tested with 5 different SD cards of various sizes and brands.

## Root Cause

The Arduino `SD.h` library on ESP32 Core 3.x uses the **ESP-IDF SDSPI driver** internally. This driver sends CMD52 (IO reset) with CRC during initialization. After a deep sleep warm reset, the SD card retains its previous SPI state with CRC checking enabled, causing the CMD52 to fail with a CRC mismatch.

This is a **known ESP-IDF bug** documented in [GitHub issue #14000](https://github.com/espressif/esp-idf/issues/14000). Even with ESP32 Arduino Core 3.3.7 (which includes the fix), the problem persisted for most consumer SD cards.

## Solution: SdFat Library (feature/sdfat-sdcard branch)

Replaced Arduino `SD.h` with **SdFat by Bill Greiman** (v2.3.1). SdFat bypasses the ESP-IDF SDSPI driver entirely and does its own SPI-level card initialization without CMD52.

**Result:** 4 out of 5 SD cards work reliably after deep sleep (previously only 1-2 with SD.h).

### SD Card Compatibility Test Results

| Card | Size | SD.h (main) | SdFat (feature branch) |
|------|------|-------------|----------------------|
| Kingston Industrial | 8GB | Works | Works |
| Generic #1 | 8GB | Fails after sleep | Works |
| Generic #2 | 2GB | Fails after sleep | Works |
| Generic #3 | 32GB | Fails after sleep | Works |
| Transcend | 16GB | Fails after sleep | Does not work |

### Key Changes (SdFat migration)

| Component | SD.h (main) | SdFat (feature branch) |
|-----------|------------|----------------------|
| Library | `#include <SD.h>` | `#include "SdFat.h"` |
| SD object | `SD` (global) | `SdFs sd` (FAT32 + exFAT) |
| File type | `File` | `FsFile` |
| Init | `SD.begin(CS, SPI, speed)` | `sd.begin(SdSpiConfig(...))` |
| Open read | `SD.open("/file.txt")` | `file.open("/file.txt", O_RDONLY)` |
| Open write | `SD.open(path, FILE_WRITE)` | `file.open(path, O_WRONLY \| O_CREAT \| O_TRUNC)` |
| SPI config | ESP-IDF managed | `USER_SPI_BEGIN` (user-managed custom pins) |

### SPI Bus Sharing (LCD + SD Card)

LCD (ST7567) and SD card share the SPI bus (SCK=GPIO12, MOSI=GPIO11). MISO (GPIO13) is only connected to the SD card.

**Important:** SdFat changes the SPI bus configuration during `sd.begin()`. After SD card operations, the SPI bus and LCD must be re-initialized:

```cpp
sd.end();
SPI.end();
SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, -1);
lcd.begin();
lcd.setContrast(200);
```

This is done once after boot, not during normal operation.

## Attempts That Did NOT Solve the Problem (with SD.h)

### 1. Simple retry loop
- Added retries with `SD.end()` + `SPI.end()` between attempts
- **Result:** All retries failed — same stale state on every attempt

### 2. SPI.begin with SS=-1
- Changed `SPI.begin(SCK, MISO, MOSI, SD_CS_PIN)` to `SPI.begin(SCK, MISO, MOSI, -1)`
- Prevents hardware SS auto-control conflicting with SD library's software CS
- **Result:** Helped stability but did not fix the deep sleep issue

### 3. Dummy clock cycles before SD.begin
- Sent 80-160 dummy clock cycles with CS HIGH at 400 kHz
- **Result:** Necessary but not sufficient

### 4. SD.end() before deep sleep
- Added `SD.end()` and `SPI.end()` in `enterDeepSleep()`
- **Result:** Required fix, but alone did not solve it

### 5. Manual CMD0 with CRC 0x95
- Send CMD0 (GO_IDLE_STATE) with hardcoded CRC before `SD.begin()`
- Cards responding with 0x01 (idle) worked; cards responding 0x00 (active) did not
- **Result:** Fixed 2 additional cards, but 2 still failed

### 6. Internal pull-up on MISO
- `pinMode(SPI_MISO_PIN, INPUT_PULLUP)` before SPI init
- **Result:** Improves reliability for consumer cards

### 7. GPIO hold release for SD CS pin
- `gpio_hold_dis((gpio_num_t)SD_CS_PIN)` after deep sleep wake-up
- **Result:** Ensures clean CS state on wake-up

### 8. CMD12 (STOP_TRANSMISSION) before CMD0
- Attempted to abort a stuck multi-block transfer
- **Result:** Did not help — no transfer was in progress

### 9. GPIO bus reset (all SPI pins LOW)
- Set SCK, MOSI, CS as GPIO OUTPUT LOW for 100ms to simulate bus power-cycle
- **Result:** Did not help

### 10. Reduced SPI clock speed (4 MHz → 2 MHz)
- **Result:** Did not help

## NVS Fallback (both branches)

Config is cached to ESP32 NVS (Non-Volatile Storage) on successful SD load. If the SD card fails on a subsequent boot, the system loads config from NVS and continues operating.

- NVS only writes when config values have actually changed (flash wear protection)
- ESP32 flash endurance: ~100,000 write cycles per sector

## Hardware Recommendations

- **10k ohm pull-up resistor from MISO (GPIO13) to 3.3V** — stronger than internal ~45k ohm
- **For future PCB revisions:** Add a P-channel MOSFET power switch on SD card VCC for true power-cycling

## References

- [ESP-IDF Issue #14000: SD card SPI mode CMD52 CRC on warm reset](https://github.com/espressif/esp-idf/issues/14000)
- [Arduino-ESP32 Issue #2171: SD mount not working reliable](https://github.com/espressif/arduino-esp32/issues/2171)
- [ESP-IDF SD Pull-up Requirements](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html)
- [ESP32 Forum: SD card mount fail after deep sleep](https://www.esp32.com/viewtopic.php?t=11211)
- [SdFat GitHub — greiman/SdFat](https://github.com/greiman/SdFat)
