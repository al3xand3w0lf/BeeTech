# Beetech V2 Changelog

## v1.0.0 — SdFat Library Migration (feature/sdfat-sdcard)

### SdCard.ino — Rewritten
- Replaced Arduino `SD.h` with **SdFat library** (Bill Greiman) to bypass ESP-IDF SDSPI driver CMD52 bug
- Uses `SdFs` (auto-detects FAT32 and exFAT) with `USER_SPI_BEGIN` for custom SPI pins
- File operations changed from `File` to `FsFile`, `SD.open()` to `file.open()`
- SD card properly unmounted (`sd.end()`) before deep sleep
- Added NVS (Non-Volatile Storage) config fallback: config cached to ESP32 flash on successful SD load
- NVS write protection: only writes when config values actually changed (~100k flash cycle limit)
- Retry logic: 3 attempts with SPI bus reset between retries

### BeetechV2_Scale_Beehive.ino
- Added `SdFs sd` global object and `#include "SdFat.h"`
- Added `#include <Preferences.h>` for NVS fallback
- Added firmware version defines (`FW_VERSION "1.0.0"`) and serial output on boot
- LCD + SPI re-initialization after SD card operations (SdFat changes SPI bus config)
- GPIO hold release for SD CS pin after deep sleep wake-up
- Auto-restart after 3s on config failure instead of hanging in `while(1)`
- Removed duplicate serial log messages for scale and temperature init

### ST7567_LCD.ino
- Firmware version displayed on startup splash screen ("V2 v1.0.0")

### SD Card Compatibility (tested)
| Card | Size | Result |
|------|------|--------|
| Kingston Industrial | 8GB | Works |
| Generic | 8GB | Works |
| Generic | 2GB | Works |
| Generic | 32GB | Works |
| Transcend | 16GB | Does not work |

---

## Previous Changes (hardware migration to ESP32-S3)

### BeetechV2_Scale.ino (Hauptdatei)
- Hardware-Beschreibung aktualisiert: ESP32-S3 Dev Board, ST7567 LCD
- Alle Pin-Definitionen auf ESP32-S3 Pin Mapping umgestellt
- SPI-Bus explizit initialisiert (geteilt zwischen SD-Karte und LCD)
- I2C mit neuen Pins: Wire.begin(8, 9)
- Display-Objekt von U8G2_SH1106_128X64_NONAME_F_HW_I2C oled → U8G2_ST7567_ENH_DG128064I_F_4W_HW_SPI lcd
- Alle oled_* Aufrufe → lcd_*
- Extra-Pins als #define angelegt: SD_CARD_DETECT (46), USER_BUTTON_PIN (0)

### SH1106_OLED.ino → ST7567_LCD.ino (umbenannt + umgeschrieben)
- Alle Funktionen von oled_* → lcd_* umbenannt
- I2C-spezifischen Init-Code entfernt (setI2CAddress)
- Backlight-Steuerung hinzugefügt (lcd_backlightOn/Off, active LOW)
- lcd_init() setzt jetzt Backlight-Pin und setContrast(200)
- lcd_off/on schaltet jetzt auch Backlight mit

### Nicht geändert
- HX711_Scale.ino — nutzt nur HX711_DOUT_PIN/HX711_CLK_PIN aus der Hauptdatei
- DS18B20_Temperature.ino — nutzt DS18B20_PIN aus der Hauptdatei
- Database.ino, Terminal.ino — keine Display/Pin-Abhängigkeiten
