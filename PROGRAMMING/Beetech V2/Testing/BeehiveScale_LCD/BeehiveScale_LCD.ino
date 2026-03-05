/*
 * BeehiveScale_LCD.ino - HX711 Load Cell Test with ST7567 LCD
 *
 * Combines Scale_Test with ST7567 LCD display
 * Use Serial Monitor to interact, weight shown on LCD
 *
 * Hardware:
 * - ESP32S3 DEV
 * - HX711 24-bit ADC
 * - 4x Load Cells in Wheatstone Bridge
 * - ST7567 128x64 LCD (SPI)
 *
 * Wiring HX711:
 * - HX711 VCC  -> 3.3V
 * - HX711 GND  -> GND
 * - HX711 DT   -> GPIO 1 (A1)
 * - HX711 SCK  -> GPIO 7 (A0)
 *
 * Wiring LCD:
 * - SCK  -> GPIO 12
 * - MOSI -> GPIO 11
 * - CS   -> GPIO 19
 * - DC   -> GPIO 3
 * - RES  -> GPIO 20
 * - BL   -> GPIO 14 (LOW = on)
 */

#include <HX711.h>
#include <U8g2lib.h>
#include <SPI.h>

// =============================================================================
// Pin Configuration - HX711
// =============================================================================
#define HX711_DOUT_PIN    1
#define HX711_CLK_PIN     7

// =============================================================================
// Pin Configuration - ST7567 LCD (SPI)
// =============================================================================
#define BL_PIN 14

U8G2_ST7567_ENH_DG128064I_F_4W_HW_SPI u8g2(
  U8G2_R0,
  /* cs=*/ 19,
  /* dc=*/ 3,
  /* reset=*/ 20
);

// =============================================================================
// Calibration - ADJUST THIS VALUE
// =============================================================================
long CALIBRATION_FACTOR = -17200;  // Adjust for your load cells

// =============================================================================
// Objects
// =============================================================================
HX711 scale;
bool continuousMode = true;  // Toggle with 'p' command

// =============================================================================
// LCD functions
// =============================================================================
void lcdInit() {
  // Backlight on
  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, LOW);

  // SPI starten (SCK=12, MOSI=11)
  SPI.begin(12, -1, 11, 19);

  u8g2.begin();
  u8g2.setContrast(200);
}

void displayWeight(float weightKg, long raw) {
  u8g2.clearBuffer();

  // Weight large
  u8g2.setFont(u8g2_font_logisoso32_tf);
  u8g2.setCursor(0, 42);
  if (!isnan(weightKg)) u8g2.print(weightKg, 1);
  else                  u8g2.print("--.-");

  // "kg" label
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setCursor(110, 42);
  u8g2.print("kg");

  // Raw + Cal info bottom line
  u8g2.setCursor(0, 62);
  u8g2.print("R:");
  u8g2.print(raw);
  u8g2.print(" C:");
  u8g2.print(CALIBRATION_FACTOR);

  u8g2.sendBuffer();
}

void displayMessage(const char* line1, const char* line2) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 20, line1);
  if (line2) u8g2.drawStr(0, 40, line2);
  u8g2.sendBuffer();
}

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("  Beetech V2 - Scale + LCD Test");
    Serial.println("================================");
    Serial.println();

    // LCD init
    lcdInit();
    displayMessage("Beetech V2", "Scale + LCD Test");
    delay(1000);

    Serial.println("Initializing HX711...");
    Serial.print("DOUT Pin: GPIO");
    Serial.println(HX711_DOUT_PIN);
    Serial.print("CLK Pin:  GPIO");
    Serial.println(HX711_CLK_PIN);
    Serial.println();

    scale.begin(HX711_DOUT_PIN, HX711_CLK_PIN);

    // Check if HX711 is connected
    if (!scale.wait_ready_timeout(1000)) {
        Serial.println(">>> HX711 NOT FOUND! <<<");
        Serial.println();
        Serial.println("Check wiring:");
        Serial.println("- VCC to 3.3V");
        Serial.println("- GND to GND");
        Serial.println("- DT to GPIO1");
        Serial.println("- SCK to GPIO7");
        displayMessage("HX711 NOT FOUND!", "Check wiring");
        while(1) delay(1000);
    }

    Serial.println("HX711 found!");
    Serial.println();

    // Set calibration
    scale.set_scale(CALIBRATION_FACTOR);
    Serial.print("Calibration factor: ");
    Serial.println(CALIBRATION_FACTOR);

    // Tare the scale
    Serial.println("Taring scale (remove all weight)...");
    displayMessage("Taring...", "Remove all weight");
    delay(2000);
    scale.tare();
    Serial.println("Tare complete!");
    Serial.println();

    printHelp();
}

// =============================================================================
// Print Help
// =============================================================================
void printHelp() {
    Serial.println("--- Commands ---");
    Serial.println("r     - Read weight");
    Serial.println("t     - Tare (zero) the scale");
    Serial.println("c XXX - Set calibration factor (e.g., c -24200)");
    Serial.println("+     - Increase calibration factor by 100");
    Serial.println("-     - Decrease calibration factor by 100");
    Serial.println("raw   - Read raw value (no calibration)");
    Serial.println("cal X - Calibrate with known weight X kg");
    Serial.println("p     - Pause/resume continuous output");
    Serial.println("h     - Show this help");
    Serial.println();
}

// =============================================================================
// Loop
// =============================================================================
void loop() {
    // Continuous weight display
    if (continuousMode && scale.is_ready()) {
        float weight = scale.get_units(5);  // Average of 5 readings
        long raw = scale.read();
        Serial.print("Weight: ");
        Serial.print(weight, 2);
        Serial.print(" kg  |  Raw: ");
        Serial.print(raw);
        Serial.print("  |  Cal: ");
        Serial.print(CALIBRATION_FACTOR);
        Serial.println();

        // Update LCD
        displayWeight(weight, raw);
    }

    // Process serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        processCommand(cmd);
    }

    delay(500);
}

// =============================================================================
// Process Commands
// =============================================================================
void processCommand(String cmd) {
    Serial.println();

    if (cmd == "r") {
        // Read weight
        float weight = scale.get_units(10);
        long raw = scale.read();
        Serial.print(">>> Weight: ");
        Serial.print(weight, 3);
        Serial.println(" kg <<<");
        displayWeight(weight, raw);
    }
    else if (cmd == "t") {
        // Tare
        Serial.println("Taring... (remove all weight!)");
        displayMessage("Taring...", "Remove all weight");
        delay(1000);
        scale.tare();
        Serial.println(">>> Tare complete <<<");
        displayMessage("Tare complete!", NULL);
        delay(500);
    }
    else if (cmd.startsWith("c ")) {
        // Set calibration factor
        long newCal = cmd.substring(2).toInt();
        if (newCal != 0) {
            CALIBRATION_FACTOR = newCal;
            scale.set_scale(CALIBRATION_FACTOR);
            Serial.print(">>> Calibration set to: ");
            Serial.print(CALIBRATION_FACTOR);
            Serial.println(" <<<");
        }
    }
    else if (cmd == "+") {
        // Increase calibration
        CALIBRATION_FACTOR += 100;
        scale.set_scale(CALIBRATION_FACTOR);
        Serial.print("Cal: ");
        Serial.println(CALIBRATION_FACTOR);
    }
    else if (cmd == "-") {
        // Decrease calibration
        CALIBRATION_FACTOR -= 100;
        scale.set_scale(CALIBRATION_FACTOR);
        Serial.print("Cal: ");
        Serial.println(CALIBRATION_FACTOR);
    }
    else if (cmd == "raw") {
        // Raw reading
        long raw = scale.read_average(10);
        Serial.print(">>> Raw value: ");
        Serial.print(raw);
        Serial.println(" <<<");
    }
    else if (cmd.startsWith("cal ")) {
        // Calibrate with known weight
        float knownWeight = cmd.substring(4).toFloat();
        if (knownWeight > 0) {
            Serial.print("Calibrating with ");
            Serial.print(knownWeight);
            Serial.println(" kg");
            Serial.println("Place the weight on the scale and wait...");
            displayMessage("Calibrating...", "Place weight now");
            delay(3000);

            long raw = scale.read_average(20);
            long offset = scale.get_offset();
            CALIBRATION_FACTOR = (raw - offset) / knownWeight;
            scale.set_scale(CALIBRATION_FACTOR);

            Serial.print(">>> New calibration factor: ");
            Serial.print(CALIBRATION_FACTOR);
            Serial.println(" <<<");
            Serial.println("Save this value to config.txt!");
        } else {
            Serial.println("Usage: cal <weight_in_kg>");
        }
    }
    else if (cmd == "p") {
        continuousMode = !continuousMode;
        Serial.print(">>> Continuous output: ");
        Serial.println(continuousMode ? "ON" : "OFF");
    }
    else if (cmd == "h") {
        printHelp();
    }

    Serial.println();
}
