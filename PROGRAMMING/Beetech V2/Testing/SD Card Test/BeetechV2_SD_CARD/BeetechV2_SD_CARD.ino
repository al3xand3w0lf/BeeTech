/*
 * BeetechV2_Scale - IoT Beehive Scale
 *
 * Hardware ESP32S3 Dev Module
 * - ESP32-S3 Dev Board (D1 R3, 16MB Flash)
 * - HX711 24-bit ADC (4 Load Cells in Wheatstone Bridge)
 * - DS18B20 Temperature Sensor
 * - ST7567 LCD Display (128x64, SPI) - AiP31567 driver
 * - SD Card (SPI) for configuration
 *
 * Pin Configuration (ESP32-S3):
 * - GPIO 1:  HX711 Data
 * - GPIO 7:  HX711 Clock
 * - GPIO 2:  DS18B20 OneWire
 * - SPI (SCK=12, MOSI=11, MISO=13): SD Card (CS=10) + LCD (CS=19)
 * - GPIO 3:  LCD D/C
 * - GPIO 20: LCD Reset
 * - GPIO 14: LCD Backlight
 * - I2C (SDA=8, SCL=9)
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
#define _ESP32_MYSQL_LOGLEVEL_ 2
#include <ESP32_MySQL.h>

// =============================================================================
// Pin Definitions (ESP32-S3)
// =============================================================================
// Scale
#define HX711_DOUT_PIN    1
#define HX711_CLK_PIN     7

// Temperature
#define DS18B20_PIN       2

// SPI Bus (shared by SD Card and LCD)
#define SPI_SCK_PIN       12
#define SPI_MISO_PIN      13
#define SPI_MOSI_PIN      11

// SD Card
#define SD_CS_PIN         10
#define SD_CARD_DETECT    46

// LCD (ST7567)
#define LCD_CS_PIN        19
#define LCD_DC_PIN        3
#define LCD_RES_PIN       20
#define LCD_BL_PIN        14

// I2C
#define I2C_SDA_PIN       8
#define I2C_SCL_PIN       9

// User Button
#define USER_BUTTON_PIN   0

// =============================================================================
// Hardware Objects
// =============================================================================
HX711 scale;

OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);
DeviceAddress tempDeviceAddress;

// ST7567 LCD 128x64 SPI (AiP31567 driver)
U8G2_ST7567_ENH_DG128064I_F_4W_HW_SPI lcd(U8G2_R0, LCD_CS_PIN, LCD_DC_PIN, LCD_RES_PIN);

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

// ST7567_LCD.ino
void lcd_init();
void lcd_showStartup();
void lcd_showData(float weight, float temp);
void lcd_showStatus(const char* status);
void lcd_showError(const char* error);

// Database.ino
bool wifi_connect();
bool db_connect();
bool db_testTCPConnection();
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

    // Initialize SPI bus (shared by SD Card and LCD)
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);

    /*
    // Initialize I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Initialize LCD
    lcd_init();
    lcd_showStartup();
    delay(1000);

    // Initialize SD Card and load config
    lcd_showStatus("Loading config...");
    */

    if (!sdCard_init()) {
        lcd_showError("SD Card Error!");
        Serial.println("ERROR: SD Card initialization failed!");
        while(1) { delay(1000); }
    }

    if (!sdCard_loadConfig()) {
        lcd_showError("Config Error!");
        Serial.println("ERROR: Failed to load config.txt!");
        while(1) { delay(1000); }
    }

    Serial.println("Config loaded successfully");
    Serial.print("Station: ");
    Serial.print(CONFIG.station_number);
    Serial.print(" - ");
    Serial.println(CONFIG.station_name);

    while(1){

    }

    // Initialize Scale
    lcd_showStatus("Init Scale...");
    SENSOR_DATA.scale_connected = scale_init();
    if (SENSOR_DATA.scale_connected) {
        Serial.println("Scale initialized OK");
    } else {
        Serial.println("WARNING: Scale initialization failed!");
    }

    // Initialize Temperature Sensor
    lcd_showStatus("Init Temp...");
    SENSOR_DATA.temp_connected = temp_init();
    if (SENSOR_DATA.temp_connected) {
        Serial.println("Temperature sensor initialized OK");
    } else {
        Serial.println("WARNING: Temperature sensor not found!");
    }

    // Connect to WiFi
    lcd_showStatus("Connecting WiFi...");
    if (!wifi_connect()) {
        lcd_showError("WiFi Error!");
        Serial.println("ERROR: WiFi connection failed!");
        // Continue anyway, will retry later
    }

    // Connect to MySQL database
    lcd_showStatus("Connecting DB...");
    if (!db_connect()) {
        lcd_showError("DB Error!");
        Serial.println("WARNING: Database connection failed - will retry on upload");
    } else {
        Serial.println("Database connected!");
    }

    lcd_showStatus("Ready");
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
        lcd_showData(SENSOR_DATA.weight, SENSOR_DATA.temperature);

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
