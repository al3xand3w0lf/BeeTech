/*
 * Scale_Test.ino - HX711 Load Cell Test for Beetech V2
 *
 * Tests the HX711 load cell amplifier and scale calibration
 * Use Serial Monitor to interact
 *
 * Hardware:
 * - Adafruit ESP32 Feather V2
 * - HX711 24-bit ADC
 * - 4x Load Cells in Wheatstone Bridge
 *
 * Wiring:
 * - HX711 VCC  -> 3.3V
 * - HX711 GND  -> GND
 * - HX711 DT   -> GPIO25 (A1)
 * - HX711 SCK  -> GPIO26 (A0)
 */

#include <HX711.h>

// =============================================================================
// Pin Configuration
// =============================================================================
#define HX711_DOUT_PIN    26    // A1 - Data
#define HX711_CLK_PIN     25    // A0 - Clock

// =============================================================================
// Calibration - ADJUST THIS VALUE
// =============================================================================
long CALIBRATION_FACTOR = -17200;  // Adjust for your load cells

// =============================================================================
// Objects
// =============================================================================
HX711 scale;

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("  Beetech V2 - Scale Test");
    Serial.println("================================");
    Serial.println();

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
        Serial.println("- DT to GPIO25");
        Serial.println("- SCK to GPIO26");
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
    Serial.println("h     - Show this help");
    Serial.println();
}

// =============================================================================
// Loop
// =============================================================================
void loop() {
    // Continuous weight display
    if (scale.is_ready()) {
        float weight = scale.get_units(5);  // Average of 5 readings
        Serial.print("Weight: ");
        Serial.print(weight, 2);
        Serial.print(" kg  |  Cal: ");
        Serial.print(CALIBRATION_FACTOR);
        Serial.println();
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
        Serial.print(">>> Weight: ");
        Serial.print(weight, 3);
        Serial.println(" kg <<<");
    }
    else if (cmd == "t") {
        // Tare
        Serial.println("Taring... (remove all weight!)");
        delay(1000);
        scale.tare();
        Serial.println(">>> Tare complete <<<");
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
    else if (cmd == "h") {
        printHelp();
    }

    Serial.println();
}
