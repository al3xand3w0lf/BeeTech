# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BeeTech is an IoT beehive monitoring system with multiple hardware variants. The system collects weight, temperature, and sound data from beehives and uploads to a database for visualization.

## Build System

This is an Arduino-based firmware project. There is no Makefile or PlatformIO configuration - use Arduino IDE for compilation and upload.

### Target Boards
- **Beetech V1 (GSM/CAN)**: Arduino MKR GSM 1400
- **Beetech V1 (Slave nodes)**: Arduino Nano Every
- **Beetech V2 (WiFi)**: Adafruit ESP32 Feather V2
- **Beetech ETH**: Adafruit Feather ESP32

### Key Libraries Required

#### All Variants
- HX711 (load cell amplifier)
- DallasTemperature + OneWire (DS18B20 temperature sensors)

#### Beetech V1 (MKR GSM 1400)
- CAN (CAN bus communication)
- MKRGSM + MySQL_Connection (GSM upload)
- Adafruit_NeoPixel (status LED)
- Adafruit_ILI9341 (TFT display)

#### Beetech V2 (ESP32 Feather V2)
- ESP32_MySQL by Syafiqlim (direct MySQL connection)
- U8g2lib (SH1106 OLED display)
- SD (configuration from SD card)

#### Beetech ETH
- U8g2lib (OLED display)
- ArduinoJson + HTTPClient (WiFi/HTTP upload)
- Adafruit_EEPROM_I2C (configuration storage)

## Architecture

### Project Variants

```
Beetech V1/
├── BeeHive_MKR1400_Board/     # Master station with GSM upload
│   ├── BeeHive_GSM_CAN_Network/    # CAN bus network mode
│   └── BeeHive_GSM_Standalone_noCAN/
├── BeeHive_NanoEvery_Board/   # CAN slave nodes
└── Testing/                   # Component test sketches

Beetech V2/
├── BeetechV2_Scale/           # ESP32 WiFi with direct MySQL
│   ├── BeetechV2_Scale.ino    # Main sketch
│   ├── Database.ino           # WiFi & MySQL (ESP32_MySQL)
│   ├── DS18B20_Temperature.ino
│   ├── HX711_Scale.ino
│   ├── SdCard.ino             # Config from SD card
│   ├── SH1106_OLED.ino
│   └── Terminal.ino           # Serial commands
├── Testing/
│   ├── Database_Test/         # MySQL connection test
│   ├── Scale_Test/            # HX711 calibration
│   └── Temperature_Test/      # DS18B20 test
├── config.txt                 # Example configuration
└── README.md                  # V2 documentation

Beetech ETH/
└── Codes/BeehiveScale/        # ESP32 WiFi variant (HTTP)
```

### Multi-File Sketch Pattern
Each firmware variant is split into functional modules that Arduino IDE combines:
- `*_main.ino` or `*.ino` - setup()/loop(), system initialization
- `HX711_Scale.ino` - Load cell interface
- `DS18B20_Temperature.ino` - Temperature sensors
- `Database.ino` - Data upload (MySQL or HTTP)
- `Terminal.ino` - Serial command interface
- `SdCard.ino` - SD card and config loading
- `*_OLED.ino` - Display functions

### Data Structures

#### V1 (CAN Network)
```cpp
typedef struct {
  float temp1_float;      // Inside temperature
  float temp2_float;      // Scale temperature
  float weight_float;     // Weight in kg
  int   sound_int;        // Sound level
} beehive_t;
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

- **CAN Bus (V1)**: Master-slave network, 500kbps. Message IDs encode hive number + data type
- **GSM/GPRS (V1)**: MySQL database upload via MKRGSM + MySQL_Connection library
- **WiFi/MySQL (V2)**: Direct MySQL connection via ESP32_MySQL library
- **WiFi/HTTP (ETH)**: JSON POST to Flask server → InfluxDB → Grafana

### Configuration

#### V1 - config.txt on SD card
```
station_number=0
station_name=Lab_Beehive0
calibration_factor=-24200
dataPoll_intervall=60
uploadAfterXdatapolls=5
number_beehives=9
```

#### V2 - config.txt on SD card
```
station_number=0
station_name=Beehive_Lab
calibration_factor=-24200
scale_offset=0.0
dataPoll_interval=10
upload_interval=30
wifi_ssid=MyWiFi
wifi_password=MyPassword
db_server=91.204.46.146
db_port=3306
db_user=dbuser
db_password=dbpass
db_name=dbname
db_table=BeetechData01
deep_sleep_enabled=0
```

## Pin Configuration

### Beetech V2 (ESP32 Feather V2)
| Function | GPIO | Arduino Pin |
|----------|------|-------------|
| HX711 Data | 25 | A1 |
| HX711 Clock | 26 | A0 |
| DS18B20 | 39 | A3 |
| SD Card CS | 4 | A5 |
| I2C SDA | 23 | SDA |
| I2C SCL | 22 | SCL |

## Testing

### V2 Test Sketches
Located in `Beetech V2/Testing/`:

- **Database_Test**: MySQL connection debugging with detailed error messages
- **Scale_Test**: Interactive HX711 calibration tool
- **Temperature_Test**: DS18B20 sensor scanner

### V1 Test Sketches
Located in `Beetech V1/Testing/` and `Beetech ETH/Codes/`:
- `ScaleTest_*` - Load cell calibration
- `DS18B20_test` - Temperature sensor verification
- `I2C_Scanner` - Device discovery
- `CanBus` - CAN communication testing

There is no automated test framework - testing is manual via Serial Monitor.

## Common Issues

### MySQL 8+ Authentication Error
```
[SQL] Can't connect. Error reading auth packets
```
**Solution**: MySQL 8+ uses `caching_sha2_password` by default. Change to `mysql_native_password`:
```sql
ALTER USER 'username'@'%' IDENTIFIED WITH mysql_native_password BY 'password';
FLUSH PRIVILEGES;
```

### DS18B20 Not Found
- Ensure 4.7kΩ pullup resistor between DATA and VCC
- Check wiring connections
- Use Temperature_Test sketch for debugging

### HX711 Not Responding
- Check wiring (DT → GPIO25, SCK → GPIO26)
- Ensure 3.3V power (not 5V)
- Use Scale_Test sketch for debugging
