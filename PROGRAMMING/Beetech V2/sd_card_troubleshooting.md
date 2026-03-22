# SD Card Troubleshooting — ESP32-S3 Deep Sleep

## Problem

SD card initialization fails after ESP32-S3 wakes up from deep sleep. First boot (fresh card insert) works, but subsequent boots after a deep sleep cycle fail with `SD card initialization failed`.

Tested with 4 different SD cards — only a Kingston Industrial card worked reliably before the fix.

## Root Cause

After deep sleep, the ESP32-S3 reboots (setup() runs fresh), but the **SD card stays powered**. The card retains its previous SPI state with CRC checking enabled. The ESP32 tries to initialize it as a fresh power-on, but the card rejects commands due to CRC mismatches.

This is a **known ESP-IDF bug** documented in [GitHub issue #14000](https://github.com/espressif/esp-idf/issues/14000). Industrial-grade cards (like Kingston Industrial) have more robust state machines that tolerate re-initialization, while consumer cards fail.

## Attempts & Solutions

### 1. Simple retry loop (did not fix)
- Added 3 retries with `SD.end()` + `SPI.end()` between attempts
- Increasing delays between retries
- **Result:** All retries failed — same stale state on every attempt

### 2. SPI.begin with SS=-1 (partial fix)
- Changed `SPI.begin(SCK, MISO, MOSI, SD_CS_PIN)` to `SPI.begin(SCK, MISO, MOSI, -1)`
- Prevents hardware SS auto-control conflicting with SD library's software CS
- **Result:** Helped stability but did not fix the deep sleep issue

### 3. Dummy clock cycles before SD.begin (did not fix alone)
- Sent 80 dummy clock cycles (10 bytes of 0xFF) with CS HIGH at 400 kHz
- This is an SD spec requirement for entering SPI mode
- **Result:** Necessary but not sufficient for warm-reset recovery

### 4. SD.end() before deep sleep (necessary but not sufficient)
- Added `SD.end()` and `SPI.end()` in `enterDeepSleep()` before `gpio_hold_en()`
- Properly unmounts the SD card and releases the SPI bus before sleep
- **Result:** Required fix, but alone did not solve the problem for all cards

### 5. Manual CMD0 with CRC — THE FIX
- Send CMD0 (GO_IDLE_STATE) with hardcoded CRC 0x95 before `SD.begin()`
- CMD0 is the only SD command that always accepts a fixed CRC value
- Forces the card back to idle state regardless of its current SPI state
- **Must be sent before each retry attempt**, not just once at the beginning
- **Result:** Fixed the issue for all 4 tested SD cards

### 6. Internal pull-up on MISO (contributing fix)
- `pinMode(SPI_MISO_PIN, INPUT_PULLUP)` before SPI init
- ESP-IDF SD driver waits for MISO HIGH before sending transactions
- Without pull-up, MISO can float and stall the driver
- Industrial cards have stronger internal pull-ups, explaining why they worked
- **Result:** Improves reliability, especially for consumer cards

### 7. GPIO hold release for SD CS pin (contributing fix)
- Added `gpio_hold_dis((gpio_num_t)SD_CS_PIN)` after deep sleep wake-up
- The SD CS pin could be held from `gpio_deep_sleep_hold_en()` call
- **Result:** Ensures clean CS state on wake-up

## Final Working Init Sequence

```
1. Release GPIO holds (including SD CS)
2. 10ms delay for pin stabilization
3. SPI.begin(SCK, MISO, MOSI, -1)    // no hardware SS
4. For each retry attempt (up to 5):
   a. SD.end() + SPI.end()
   b. 100ms delay
   c. SPI.begin() at 400 kHz
   d. pinMode(MISO, INPUT_PULLUP)
   e. 80 dummy clocks with CS HIGH
   f. CMD0 (0x40, 0x00, 0x00, 0x00, 0x00, 0x95) with CS LOW
   g. Read R1 response (expect 0x01 = idle)
   h. CS HIGH + 8 extra clocks
   i. SPI.end()
   j. SPI.begin()
   k. SD.begin(CS_PIN, SPI, 4MHz)
5. If all retries fail: load config from NVS fallback
```

## NVS Fallback

As additional safety, the config is cached to ESP32 NVS (Non-Volatile Storage) on successful SD load. If the SD card fails on a subsequent boot, the system loads config from NVS and continues operating.

- NVS only writes when config values have actually changed (flash wear protection)
- ESP32 flash endurance: ~100,000 write cycles per sector

## Hardware Recommendations

- **10k ohm pull-up resistor from MISO (GPIO13) to 3.3V** — stronger than the internal ~45k ohm pull-up
- **Disconnect MISO from LCD** if connected — ST7567 is write-only and doesn't need MISO, but may pull the line low if not properly tri-stated
- **For future PCB revisions:** Add a P-channel MOSFET power switch on SD card VCC, controlled by a GPIO. This allows true power-cycling of the SD card, guaranteeing cold-boot state on every wake cycle.

## References

- [ESP-IDF Issue #14000: SD card SPI mode CMD52 CRC on warm reset](https://github.com/espressif/esp-idf/issues/14000)
- [Arduino-ESP32 Issue #2171: SD mount not working reliable](https://github.com/espressif/arduino-esp32/issues/2171)
- [ESP-IDF SD Pull-up Requirements](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html)
- [ESP32 Forum: SD card mount fail after deep sleep](https://www.esp32.com/viewtopic.php?t=11211)
