/*
 * Temperature_Test.ino - DS18B20 Sensor Test for Beetech V2
 *
 * Tests DS18B20 OneWire temperature sensors
 * Scans for all connected sensors and displays readings
 *
 * Hardware:
 * - Adafruit ESP32 Feather V2
 * - DS18B20 Temperature Sensor(s)
 *
 * Wiring:
 * - DS18B20 VCC (red)    -> 3.3V
 * - DS18B20 GND (black)  -> GND
 * - DS18B20 DATA (yellow) -> GPIO39 (A3)
 * - 4.7k Ohm Pullup Resistor between DATA and VCC
 *
 * WICHTIG: Der 4.7k Pullup-Widerstand ist ERFORDERLICH!
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// =============================================================================
// Pin Configuration
// =============================================================================
#define DS18B20_PIN    2    // D2 - OneWire Data

// =============================================================================
// Objects
// =============================================================================
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

// Store found device addresses
#define MAX_SENSORS 8
DeviceAddress sensorAddresses[MAX_SENSORS];
int sensorCount = 0;

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("  Beetech V2 - Temperature Test");
    Serial.println("================================");
    Serial.println();

    Serial.print("OneWire Pin: GPIO");
    Serial.println(DS18B20_PIN);
    Serial.println();

    // Initialize sensors
    sensors.begin();

    // Scan for sensors
    scanSensors();

    if (sensorCount == 0) {
        Serial.println(">>> NO SENSORS FOUND! <<<");
        Serial.println();
        Serial.println("Troubleshooting:");
        Serial.println("1. Check wiring (VCC, GND, DATA)");
        Serial.println("2. 4.7k pullup resistor between DATA and VCC?");
        Serial.println("3. Correct GPIO pin? (GPIO39 = A3)");
        Serial.println("4. Try different sensor");
        Serial.println();
        Serial.println("Wiring:");
        Serial.println("  DS18B20 VCC (red)    -> 3.3V");
        Serial.println("  DS18B20 GND (black)  -> GND");
        Serial.println("  DS18B20 DATA (yellow) -> GPIO39");
        Serial.println("  4.7k Resistor: DATA <-> VCC");
    }

    Serial.println();
    printHelp();
}

// =============================================================================
// Scan for Sensors
// =============================================================================
void scanSensors() {
    Serial.println("--- Scanning for DS18B20 sensors ---");

    sensorCount = sensors.getDeviceCount();
    Serial.print("Found ");
    Serial.print(sensorCount);
    Serial.println(" sensor(s)");
    Serial.println();

    // Get addresses of all found sensors
    for (int i = 0; i < sensorCount && i < MAX_SENSORS; i++) {
        if (sensors.getAddress(sensorAddresses[i], i)) {
            Serial.print("Sensor ");
            Serial.print(i);
            Serial.print(": ");
            printAddress(sensorAddresses[i]);
            Serial.println();

            // Set resolution (9-12 bits, higher = more precise but slower)
            sensors.setResolution(sensorAddresses[i], 12);
        }
    }
}

// =============================================================================
// Print Device Address
// =============================================================================
void printAddress(DeviceAddress deviceAddress) {
    for (uint8_t i = 0; i < 8; i++) {
        if (deviceAddress[i] < 16) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
        if (i < 7) Serial.print(":");
    }
}

// =============================================================================
// Print Help
// =============================================================================
void printHelp() {
    Serial.println("--- Commands ---");
    Serial.println("r     - Read all temperatures");
    Serial.println("s     - Scan for sensors");
    Serial.println("raw   - Show raw OneWire scan");
    Serial.println("h     - Show this help");
    Serial.println();
}

// =============================================================================
// Loop
// =============================================================================
void loop() {
    // Continuous temperature display if sensors found
    if (sensorCount > 0) {
        sensors.requestTemperatures();

        for (int i = 0; i < sensorCount && i < MAX_SENSORS; i++) {
            float tempC = sensors.getTempC(sensorAddresses[i]);

            Serial.print("Sensor ");
            Serial.print(i);
            Serial.print(": ");

            if (tempC == DEVICE_DISCONNECTED_C) {
                Serial.println("DISCONNECTED!");
            } else {
                Serial.print(tempC, 2);
                Serial.println(" C");
            }
        }
        Serial.println();
    }

    // Process serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        processCommand(cmd);
    }

    delay(2000);
}

// =============================================================================
// Process Commands
// =============================================================================
void processCommand(String cmd) {
    Serial.println();

    if (cmd == "r") {
        // Read temperatures
        if (sensorCount == 0) {
            Serial.println("No sensors found! Use 's' to scan.");
        } else {
            sensors.requestTemperatures();
            delay(750);  // Wait for conversion (12-bit = 750ms)

            Serial.println("--- Temperature Readings ---");
            for (int i = 0; i < sensorCount && i < MAX_SENSORS; i++) {
                float tempC = sensors.getTempC(sensorAddresses[i]);
                float tempF = sensors.getTempF(sensorAddresses[i]);

                Serial.print("Sensor ");
                Serial.print(i);
                Serial.print(" [");
                printAddress(sensorAddresses[i]);
                Serial.print("]: ");

                if (tempC == DEVICE_DISCONNECTED_C) {
                    Serial.println("DISCONNECTED!");
                } else {
                    Serial.print(tempC, 2);
                    Serial.print(" C  (");
                    Serial.print(tempF, 2);
                    Serial.println(" F)");
                }
            }
        }
    }
    else if (cmd == "s") {
        // Rescan for sensors
        sensors.begin();
        scanSensors();
    }
    else if (cmd == "raw") {
        // Raw OneWire scan
        Serial.println("--- Raw OneWire Scan ---");
        byte addr[8];
        int count = 0;

        oneWire.reset_search();

        while (oneWire.search(addr)) {
            Serial.print("Device ");
            Serial.print(count);
            Serial.print(": ");

            for (int i = 0; i < 8; i++) {
                if (addr[i] < 16) Serial.print("0");
                Serial.print(addr[i], HEX);
                if (i < 7) Serial.print(":");
            }

            // Check CRC
            if (OneWire::crc8(addr, 7) != addr[7]) {
                Serial.print(" (CRC ERROR!)");
            } else {
                // Identify device type
                Serial.print(" - ");
                switch (addr[0]) {
                    case 0x10:
                        Serial.print("DS18S20");
                        break;
                    case 0x28:
                        Serial.print("DS18B20");
                        break;
                    case 0x22:
                        Serial.print("DS1822");
                        break;
                    default:
                        Serial.print("Unknown");
                }
            }

            Serial.println();
            count++;
        }

        if (count == 0) {
            Serial.println("No devices found on OneWire bus!");
        }

        oneWire.reset_search();
    }
    else if (cmd == "h") {
        printHelp();
    }

    Serial.println();
}
