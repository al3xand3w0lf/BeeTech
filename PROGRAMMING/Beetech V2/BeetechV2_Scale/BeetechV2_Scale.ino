/*
 * BeetechV2_Scale - IoT Beehive Scale
 *
 * Hardware:
 * - Adafruit ESP32 Feather V2 (8MB Flash, 2MB PSRAM)
 * - Adafruit HX711 24-bit ADC (4 Load Cells in Wheatstone Bridge)
 * - DS18B20 Temperature Sensor
 * - SH1106 OLED Display (128x64, I2C)
 * - SD Card (SPI) for configuration
 *
 * Pin Configuration:
 * - A0 (GPIO26): HX711 Clock
 * - A1 (GPIO25): HX711 Data
 * - A3 (GPIO39): DS18B20 OneWire
 * - Standard VSPI: SD Card
 * - I2C (0x3C): SH1106 OLED
 */

#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>

#include <HX711.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>

// MySQL direct connection for ESP32
#define ESP32_MYSQL_DEBUG_PORT Serial
#define _ESP32_MYSQL_LOGLEVEL_ 1
#include <ESP32_MySQL.h>

// =============================================================================
// Pin Definitions
// =============================================================================
#define HX711_DOUT_PIN    25    // A1
#define HX711_CLK_PIN     26    // A0
#define DS18B20_PIN       39    // A3
#define SD_CS_PIN         4     // A5

// =============================================================================
// Hardware Objects
// =============================================================================
HX711 scale;

OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);
DeviceAddress tempDeviceAddress;

// SH1106 OLED 128x64 I2C (Adresse 0x3C = 0x78 >> 1)
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// MySQL connection objects
WiFiClient wifiClient;
ESP32_MySQL_Connection conn((Client *)&wifiClient);

// =============================================================================
// Configuration Structure (loaded from SD card)
// =============================================================================
typedef struct {
    // Station
    int station_number;
    char station_name[50];

    // Scale
    long calibration_factor;
    float scale_offset;

    // Timing
    int dataPoll_interval;      // seconds
    int upload_interval;        // seconds

    // WiFi
    char wifi_ssid[50];
    char wifi_password[50];

    // Database (MySQL direct connection like V1)
    char db_server[50];
    int  db_port;
    char db_user[50];
    char db_password[50];
    char db_name[50];
    char db_table[50];

    // Power
    int deep_sleep_enabled;
} config_t;

config_t CONFIG;

// =============================================================================
// Global State
// =============================================================================
typedef struct {
    float temperature;
    float weight;
    bool temp_connected;
    bool scale_connected;
} sensorData_t;

sensorData_t SENSOR_DATA;

// Timing
unsigned long lastPollMillis = 0;
unsigned long lastUploadMillis = 0;
int pollCount = 0;

// MySQL INSERT statement template
char INSERT_SQL[200];

// =============================================================================
// Forward Declarations (defined in other .ino files)
// =============================================================================
// SdCard.ino
bool sdCard_init();
bool sdCard_loadConfig();

// HX711_Scale.ino
bool scale_init();
float scale_read();

// DS18B20_Temperature.ino
bool temp_init();
float temp_read();

// SH1106_OLED.ino
void oled_init();
void oled_showStartup();
void oled_showData(float weight, float temp);
void oled_showStatus(const char* status);
void oled_showError(const char* error);

// Database.ino
bool wifi_connect();
bool db_connect();
bool db_uploadData(int stationNum, const char* stationName, float temp, float weight);

// Terminal.ino
void terminal_process();

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println();
    Serial.println("================================");
    Serial.println("  Beetech V2 - IoT Beehive Scale");
    Serial.println("================================");

    // Initialize I2C
    Wire.begin();

    // Initialize OLED
    oled_init();
    oled_showStartup();
    delay(1000);

    // Initialize SD Card and load config
    oled_showStatus("Loading config...");
    if (!sdCard_init()) {
        oled_showError("SD Card Error!");
        Serial.println("ERROR: SD Card initialization failed!");
        while(1) { delay(1000); }
    }

    if (!sdCard_loadConfig()) {
        oled_showError("Config Error!");
        Serial.println("ERROR: Failed to load config.txt!");
        while(1) { delay(1000); }
    }

    Serial.println("Config loaded successfully");
    Serial.print("Station: ");
    Serial.print(CONFIG.station_number);
    Serial.print(" - ");
    Serial.println(CONFIG.station_name);

    // Initialize Scale
    oled_showStatus("Init Scale...");
    SENSOR_DATA.scale_connected = scale_init();
    if (SENSOR_DATA.scale_connected) {
        Serial.println("Scale initialized OK");
    } else {
        Serial.println("WARNING: Scale initialization failed!");
    }

    // Initialize Temperature Sensor
    oled_showStatus("Init Temp...");
    SENSOR_DATA.temp_connected = temp_init();
    if (SENSOR_DATA.temp_connected) {
        Serial.println("Temperature sensor initialized OK");
    } else {
        Serial.println("WARNING: Temperature sensor not found!");
    }

    // Connect to WiFi
    oled_showStatus("Connecting WiFi...");
    if (!wifi_connect()) {
        oled_showError("WiFi Error!");
        Serial.println("ERROR: WiFi connection failed!");
        // Continue anyway, will retry later
    }

    // Connect to MySQL database
    oled_showStatus("Connecting DB...");
    if (!db_connect()) {
        oled_showError("DB Error!");
        Serial.println("WARNING: Database connection failed - will retry on upload");
    } else {
        Serial.println("Database connected!");
    }

    oled_showStatus("Ready");
    delay(1000);

    Serial.println("================================");
    Serial.println("  System Ready - Starting Loop");
    Serial.println("================================");

    // Initial reading
    lastPollMillis = millis();
    lastUploadMillis = millis();
}

// =============================================================================
// Main Loop
// =============================================================================
void loop() {
    unsigned long currentMillis = millis();

    // Process terminal commands
    terminal_process();

    // Poll sensors at configured interval
    if (currentMillis - lastPollMillis >= (CONFIG.dataPoll_interval * 1000UL)) {
        lastPollMillis = currentMillis;

        // Read sensors
        if (SENSOR_DATA.scale_connected) {
            SENSOR_DATA.weight = scale_read();
        }

        if (SENSOR_DATA.temp_connected) {
            SENSOR_DATA.temperature = temp_read();
        }

        // Update display
        oled_showData(SENSOR_DATA.weight, SENSOR_DATA.temperature);

        // Print to Serial
        Serial.print("Weight: ");
        Serial.print(SENSOR_DATA.weight, 2);
        Serial.print(" kg, Temp: ");
        Serial.print(SENSOR_DATA.temperature, 1);
        Serial.println(" C");

        pollCount++;
    }

    // Upload data at configured interval
    if (currentMillis - lastUploadMillis >= (CONFIG.upload_interval * 1000UL)) {
        lastUploadMillis = currentMillis;

        Serial.println("Uploading data to database...");

        if (db_uploadData(CONFIG.station_number, CONFIG.station_name,
                          SENSOR_DATA.temperature, SENSOR_DATA.weight)) {
            Serial.println("Upload successful");
        } else {
            Serial.println("Upload failed - will retry next interval");
        }

        pollCount = 0;
    }

    // Small delay to prevent watchdog issues
    delay(100);
}

// =============================================================================
// Utility Functions
// =============================================================================
void softReset() {
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
}
