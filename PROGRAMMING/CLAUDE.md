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

| | V2 v0.1 (`BeetechV2_Scale - v0.1/`) | V2 current (`BeetechV2_Scale/`) |
|---|---|---|
| **MCU** | Adafruit ESP32 Feather V2 | ESP32-S3 Dev Board |
| **Display** | SH1106 OLED (I2C, 0x3C) | ST7567 LCD (SPI) |
| **HX711 Data** | GPIO 25 (A1) | GPIO 1 |
| **HX711 Clock** | GPIO 26 (A0) | GPIO 7 |
| **DS18B20** | GPIO 14 | GPIO 2 |
| **SD CS** | GPIO 4 (standard VSPI) | GPIO 10 (shared SPI) |
| **I2C** | Default | SDA=8, SCL=9 |
| **SPI** | Default VSPI | SCK=12, MOSI=11, MISO=13 |
| **LCD Pins** | N/A (I2C) | CS=19, D/C=3, RES=20, BL=14 |
| **User Button** | N/A | GPIO 0 |
| **Card Detect** | N/A | GPIO 46 |

The README.md in `Beetech V2/` still describes the v0.1 (Feather V2) hardware.

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
    float scale_offset;
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

### Timing Model
Non-blocking `millis()`-based intervals in `loop()`:
- `dataPoll_interval` — sensor read frequency (10–60s typical)
- `upload_interval` — database upload frequency (30+s typical)
- V1 uses `uploadAfterXdatapolls` multiplier instead of separate upload interval

### Configuration
SD card `/config.txt` with `key=value` format. Supports `#` comments and blank lines. Parsed line-by-line in `SdCard.ino`. Required fields validated on load.

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
`help`, `status`, `read`, `tare`, `cal XX`, `upload`, `wifi`, `db`, `reset`

## Testing

No automated test framework — testing is manual via Serial Monitor.

### V2 Test Sketches (`Beetech V2/Testing/`)
- **Database_Test** — MySQL connection debugging
- **Scale_Test** — Interactive HX711 calibration (`t`=tare, `cal X`=calibrate with X kg, `r`=read, `raw`=raw value, `+`/`-`=adjust factor)
- **Temperature_Test** — DS18B20 scanner (`r`=read, `s`=scan, `raw`=raw OneWire scan)

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

### SD Card Errors
- Format as FAT32
- `config.txt` must be in root directory
- Use UTF-8 encoding with Unix line endings

### WiFi
- ESP32 supports 2.4 GHz only (no 5 GHz)
