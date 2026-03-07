# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BeeTech is an IoT beehive monitoring system with multiple hardware variants. The system collects weight, temperature, and sound data from beehives and uploads to a database for visualization.

## Build System

Arduino-based firmware project — use Arduino IDE for compilation and upload. No Makefile or PlatformIO.

### Target Boards
- **Beetech V1 (Master)**: Arduino MKR GSM 1400
- **Beetech V1 (Slave)**: Arduino Nano Every
- **Beetech V2 (current)**: ESP32-S3 Dev Board (replaced Adafruit ESP32 Feather V2)
- **Beetech ETH**: Adafruit Feather ESP32

### Arduino IDE Settings (V2 current)
```
Board:        ESP32-S3 Dev Board
Upload Speed: 921600
Flash Mode:   QIO
```

### Key Libraries

| Library | Used By | Purpose |
|---------|---------|---------|
| HX711 | All | Load cell amplifier |
| DallasTemperature + OneWire | All | DS18B20 temperature sensors |
| ESP32_MySQL (by Syafiqlim) | V2 | Direct MySQL connection |
| U8g2 | V2, ETH | Display driver (ST7567 LCD / SH1106 OLED) |
| SD | V2 | Configuration from SD card |
| CAN | V1 | CAN bus communication |
| MKRGSM + MySQL_Connection | V1 | GSM data upload |
| Adafruit_ILI9341 | V1 | TFT display |
| ArduinoJson + HTTPClient | ETH | WiFi/HTTP upload to InfluxDB |

## Architecture

### Multi-File Sketch Pattern
Arduino IDE concatenates all `.ino` files in a sketch folder. Each variant splits code into functional modules:
- Main `.ino` — `setup()`/`loop()`, globals, forward declarations for module functions
- `HX711_Scale.ino` — Load cell interface
- `DS18B20_Temperature.ino` — Temperature sensors
- `Database.ino` — Data upload (MySQL or HTTP)
- `Terminal.ino` — Serial command interface (115200 baud)
- `SdCard.ino` — SD card config loading
- Display `.ino` — `ST7567_LCD.ino` (V2 current), `SH1106_OLED.ino` (V2 v0.1), `LCD_ST7567.ino` (ETH), `ILI9341_tft.ino` (V1)

### Hardware Versions (V2)

**IMPORTANT**: Two V2 hardware versions exist in the repo with completely different pinouts:

The current V2 hardware uses an ESP32-S3 Dev Board. The main production sketch is `BeetechV2_Scale_Beehive/`. The previous `BeetechV2_Scale/` sketch is now in `Testing/` for basic scale testing without deep sleep.

| Pin | Function |
|---|---|
| GPIO 1 | HX711 Data |
| GPIO 7 | HX711 Clock |
| GPIO 2 | DS18B20 OneWire |
| GPIO 10 | SD Card CS |
| GPIO 12/11/13 | SPI SCK/MOSI/MISO |
| GPIO 19 | LCD CS |
| GPIO 3 | LCD D/C |
| GPIO 20 | LCD Reset |
| GPIO 14 | LCD Backlight (active LOW) |
| GPIO 8/9 | I2C SDA/SCL |
| GPIO 0 | User Button |
| GPIO 46 | SD Card Detect |

### Data Structures

#### V1 (CAN Network)
```cpp
typedef struct {
  float temp1_float;      // Inside temperature
  float temp2_float;      // Scale temperature
  float weight_float;     // Weight in kg
  int   sound_int;        // Sound level
} beehive_t;              // Array of up to 20 hives
```

#### V2 (Standalone)
```cpp
typedef struct {
    float temperature;
    float weight;
    bool temp_connected;
    bool scale_connected;
} sensorData_t;

typedef struct {
    int station_number;
    char station_name[50];
    long calibration_factor;
    long scale_offset;           // raw HX711 tare offset
    int dataPoll_interval;
    int upload_interval;
    char wifi_ssid[50];
    char wifi_password[50];
    char db_server[50];
    int db_port;
    char db_user[50];
    char db_password[50];
    char db_name[50];
    char db_table[50];
    int deep_sleep_enabled;
} config_t;
```

### Communication Protocols

- **CAN Bus (V1)**: Master-slave, 500kbps. Message IDs: 0x100=TEMP1, 0x200=TEMP2, 0x300=WEIGHT, 0x400=SOUND (offset by hive number)
- **GSM/MySQL (V1)**: Direct MySQL via MKRGSM + MySQL_Connection
- **WiFi/MySQL (V2)**: Direct MySQL via ESP32_MySQL library
- **WiFi/HTTP (ETH)**: JSON POST → Flask server → InfluxDB → Grafana

### Operating Modes (V2)

Controlled by `deep_sleep_enabled` in `config.txt`:

- **Setup mode** (`deep_sleep_enabled=0`): Normal loop with terminal, LCD, periodic sensor reads and uploads. Use for tare/calibration.
- **Deep Sleep mode** (`deep_sleep_enabled=1`): Boot → init → measure → upload → deep sleep for `upload_interval` seconds. LCD stays on during sleep (GPIO hold). `dataPoll_interval` is ignored.

### Tare Offset Persistence (V2)

The tare offset is stored as a raw HX711 `long` value in `/tare_offset.txt` on the SD card (separate from `config.txt`). On boot, `sdCard_loadOffset()` loads it and overrides `CONFIG.scale_offset`. If no file exists and no offset is configured, the scale auto-tares with current load.

Workflow: `tare` → `saveoffset` in terminal, or manually create `/tare_offset.txt` with the known offset value.

### Timing Model
Non-blocking `millis()`-based intervals in `loop()` (setup mode only):
- `dataPoll_interval` — sensor read frequency (10–60s typical)
- `upload_interval` — database upload frequency / deep sleep duration (30+s typical)
- V1 uses `uploadAfterXdatapolls` multiplier instead of separate upload interval

### Configuration
SD card `/config.txt` with `key=value` format. Supports `#` comments and blank lines. Parsed line-by-line in `SdCard.ino`. Required fields validated on load. Tare offset is stored separately in `/tare_offset.txt`.

### MySQL Database Schema (V2)
```sql
CREATE TABLE BeetechData01 (
    id INT AUTO_INCREMENT PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    location VARCHAR(100),
    hive INT,
    temp1 FLOAT,
    temp2 FLOAT,
    weight FLOAT,
    sound INT
);
```

### Terminal Commands (V2, 115200 baud)
`help`, `status`, `read`, `tare`, `saveoffset`, `cal XX`, `upload`, `wifi`, `db`, `reset`

See `Beetech V2/terminal_cli.md` for full CLI reference.

## Testing

No automated test framework — testing is manual via Serial Monitor.

### V2 Test Sketches (`Beetech V2/Testing/`)
- **Database_Test** — MySQL connection debugging
- **BeetechV2_Scale** — Basic IoT scale (no deep sleep, continuous loop mode, for testing)
- **Scale_Test** — Interactive HX711 calibration (`t`=tare, `cal X`=calibrate with X kg, `r`=read, `raw`=raw value, `+`/`-`=adjust factor)
- **BeehiveScale_LCD** — Scale_Test combined with ST7567 LCD display (weight + raw/cal shown on LCD)
- **Temperature_Test** — DS18B20 scanner (`r`=read, `s`=scan, `raw`=raw OneWire scan)
- **LCD_Test/LCD_ST7567** — Standalone ST7567 LCD display test

### V1 Test Sketches (`Beetech V1/Testing/`)
- `ScaleTest_*` — Load cell calibration
- `CanBus/` — CAN communication testing
- `DS18B20_test` — Temperature sensor verification
- `I2C_Scanner` — Device discovery
- `SoundTest/` — Sound sensor testing

## Common Issues

### MySQL 8+ Authentication Error
`[SQL] Can't connect. Error reading auth packets` — MySQL 8+ defaults to `caching_sha2_password`. Fix:
```sql
ALTER USER 'username'@'%' IDENTIFIED WITH mysql_native_password BY 'password';
FLUSH PRIVILEGES;
```

### DS18B20 Not Found
- 4.7kΩ pullup resistor between DATA and VCC required
- Check correct GPIO for your hardware version (GPIO 2 on current V2, GPIO 14 on V2 v0.1)

### HX711 Not Responding
- Check correct GPIO for your hardware version (GPIO 1/7 on current V2, GPIO 25/26 on V2 v0.1)
- Ensure 3.3V power (not 5V)
- **Calibration factor must be negative** (e.g., `-17200`). A positive factor produces inverted (negative) weight readings. Do NOT negate `get_units()` — use a negative factor instead.

### SD Card Errors
- Format as FAT32
- `config.txt` must be in root directory
- Use UTF-8 encoding with Unix line endings

### WiFi
- ESP32 supports 2.4 GHz only (no 5 GHz)
- V2 startup uses short WiFi timeout (5s) so boot is not blocked. Full reconnect (30s) happens on upload. WiFi auto-reconnect is enabled.
