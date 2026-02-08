#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <U8g2lib.h>
#include <Pushbutton.h>
#include "Adafruit_EEPROM_I2C.h"
#include "esp_sleep.h"

#include "HX711.h"

// =====================
// Serial
// =====================
#define SERIAL_BAUD 115200

// =====================
// WiFi
// =====================
const char* ssid     = "eth-iot";
const char* password = "LigPyqB.p_Ah";

// =====================
// Server / Flask
// =====================
#define GALACTICARCHIVE_HTTP_ADRESS "http://10.236.16.251:5000/sensor/"
static const char SENSOR_ENDPOINT[] = "beehive";

// =====================
// Influx naming (sent in JSON to Flask)
// =====================
#define INFLUXDB_DBNAME       "flask_data"
#define INFLUXDB_MEASUREMENT  "beehives"

// =====================
// Tags
// =====================
static const char LOCATION_TAG[] = "garden";

// =====================
// DeepSleep
// =====================
static const uint64_t SLEEP_US = 60ULL * 1000000ULL;   // 1 minute
static const unsigned long WIFI_TIMEOUT_MS = 30000;    // 30s
static const int SAMPLES = 5;                          // 5 samples / 5 sec
static const int MIN_VALID_SAMPLES = 3;                // min 3/5
static const float TEMPERATURE_CONST = 0.0f;           // always 0 for now

// =====================
// I2C (EEPROM + OLED)
// =====================
constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

#define EEPROM_ADDR 0x50
#define ADRESS_NODE_NUMBER 0x00

// EEPROM addresses for saved tare offset (hybrid approach)
#define ADDR_TARE_MAGIC0     0x10
#define ADDR_TARE_MAGIC1     0x11
#define ADDR_TARE_OFFSET     0x12   // 4 bytes: int32_t

static const uint8_t TARE_MAGIC0 = 0xA5;
static const uint8_t TARE_MAGIC1 = 0x5A;

Adafruit_EEPROM_I2C i2ceeprom;

// U8g2 OLED (SSD1306 128x64 I2C)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// =====================
// HX711
// =====================
const int LOADCELL_DOUT_PIN = A0;
const int LOADCELL_SCK_PIN  = A1;

HX711 scale;

// Your calibration factor
#define CALIBRATION_FACTOR 16200

// =====================
// Button = BOOT button on ESP32 (GPIO0), tare only
// =====================
#define BUTTON_PIN 0
Pushbutton button(BUTTON_PIN);

// =====================
// State
// =====================
uint8_t hive_id = 0;
bool eepromOk = false;

// -------------------------------------------------
// EEPROM byte helpers
// -------------------------------------------------
static void eepromWriteBytes(uint16_t addr, const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    i2ceeprom.write(addr + i, data[i]);
    delay(5); // safe for I2C EEPROM write cycle
  }
}

static void eepromReadBytes(uint16_t addr, uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    data[i] = i2ceeprom.read(addr + i);
  }
}

static bool tareOffsetExists() {
  if (!eepromOk) return false;
  return (i2ceeprom.read(ADDR_TARE_MAGIC0) == TARE_MAGIC0) &&
         (i2ceeprom.read(ADDR_TARE_MAGIC1) == TARE_MAGIC1);
}

static bool loadTareOffset(int32_t &off) {
  if (!tareOffsetExists()) return false;
  eepromReadBytes(ADDR_TARE_OFFSET, (uint8_t*)&off, sizeof(off));
  return true;
}

static void saveTareOffset(int32_t off) {
  if (!eepromOk) return;
  i2ceeprom.write(ADDR_TARE_MAGIC0, TARE_MAGIC0); delay(5);
  i2ceeprom.write(ADDR_TARE_MAGIC1, TARE_MAGIC1); delay(5);
  eepromWriteBytes(ADDR_TARE_OFFSET, (uint8_t*)&off, sizeof(off));
}

// -------------------------------------------------
// OLED
// -------------------------------------------------
static void oledOn() {
  u8g2.begin();
  u8g2.setPowerSave(0); // ON
  delay(30);
}

static void oledOff() {
  u8g2.setPowerSave(1); // OFF
}

static void displayWeight(float weightKg) {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_logisoso32_tf);
  u8g2.setCursor(0, 58);

  if (!isnan(weightKg)) u8g2.print(weightKg, 1);
  else                  u8g2.print("--.-");

  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setCursor(100, 60);
  u8g2.print("kg");

  u8g2.sendBuffer();
}

// -------------------------------------------------
// WiFi
// -------------------------------------------------
static bool waitForWiFiConnected(unsigned long timeoutMs) {
  Serial.println("connecting...");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    // allow tare by button while waiting
    if (button.getSingleDebouncedPress()) {
      // stronger tare for stability
      scale.tare(15);
      int32_t off = (int32_t)scale.get_offset();
      saveTareOffset(off);
    }
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected");
    return true;
  }
  return false;
}

// -------------------------------------------------
// HX711 sampling
// -------------------------------------------------
static bool readWeightSample(float &outKg) {
  // Attempt 1
  if (scale.wait_ready_timeout(200)) {
    outKg = scale.get_units();
    return true;
  }
  // Retry once
  delay(100);
  if (scale.wait_ready_timeout(200)) {
    outKg = scale.get_units();
    return true;
  }
  return false;
}

// -------------------------------------------------
// HTTP POST
// -------------------------------------------------
static bool send_Beehive_DataToFlask(uint8_t hiveIdU8, const char* location, float weightKg, float temperatureC) {
  if (WiFi.status() != WL_CONNECTED) return false;

  char url[200];
  snprintf(url, sizeof(url), "%s%s", GALACTICARCHIVE_HTTP_ADRESS, SENSOR_ENDPOINT);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["databank"]    = INFLUXDB_DBNAME;
  doc["measurement"] = INFLUXDB_MEASUREMENT;
  doc["hive_id"]     = String(hiveIdU8);
  doc["location"]    = location;
  doc["weight"]      = weightKg;
  doc["temperature"] = temperatureC; // required by server

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  http.end();

  return (code >= 200 && code < 300);
}

// -------------------------------------------------
// Sleep
// -------------------------------------------------
static void goDeepSleep() {
  // power down peripherals as much as possible
  oledOff();
  scale.power_down();

  Serial.println("DeepSleep");
  Serial.flush();

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);

  esp_sleep_enable_timer_wakeup(SLEEP_US);
  esp_deep_sleep_start();
}

// -------------------------------------------------
// Main one-shot
// -------------------------------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(50);

  Serial.println("Wake Up.");

  // BOOT button as input (pressed = LOW)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // I2C init
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  // EEPROM init
  eepromOk = i2ceeprom.begin(EEPROM_ADDR, &Wire);
  if (eepromOk) {
    hive_id = i2ceeprom.read(ADRESS_NODE_NUMBER);
  } else {
    hive_id = 0;
  }

  // OLED on only during wake
  oledOn();
  displayWeight(NAN);

  // HX711 init
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.power_up();
  delay(50);

  // HYBRID TARE:
  // - If offset exists: load it (no auto tare)
  // - Else: do ONE auto tare now, save offset
  if (eepromOk) {
    int32_t off;
    if (loadTareOffset(off)) {
      scale.set_offset((long)off);
    } else {
      // First-time tare (only once ever)
      // (Assumes scale is empty on first start)
      scale.tare(15);
      off = (int32_t)scale.get_offset();
      saveTareOffset(off);
    }
  } else {
    // No EEPROM -> no persistence, so we do NOT auto-tare here to avoid surprises.
    // User can still tare by BOOT button while awake.
  }

  // WiFi connect (max 30s)
  if (!waitForWiFiConnected(WIFI_TIMEOUT_MS)) {
    goDeepSleep();
  }

  // 5 measurements in ~5 seconds
  int valid = 0;
  float sum = 0.0f;

  for (int i = 0; i < SAMPLES; i++) {
    // tare only by button
    if (button.getSingleDebouncedPress()) {
      scale.tare(15);
      int32_t off = (int32_t)scale.get_offset();
      saveTareOffset(off);
    }

    float w = NAN;
    bool ok = readWeightSample(w);

    if (ok && !isnan(w)) {
      sum += w;
      valid++;
      displayWeight(w);
    }

    delay(1000);
  }

  // Need at least 3 valid samples
  if (valid < MIN_VALID_SAMPLES) {
    goDeepSleep();
  }

  float avgWeight = sum / (float)valid;

  // print mean BEFORE sending
  Serial.print("Mean weight: ");
  Serial.print(avgWeight, 3);
  Serial.println(" kg");

  // send
  if (send_Beehive_DataToFlask(hive_id, LOCATION_TAG, avgWeight, TEMPERATURE_CONST)) {
    Serial.println("Data send");
  }

  goDeepSleep();
}

void loop() {
  // not used (one-shot + deep sleep)
}
