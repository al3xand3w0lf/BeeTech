/*
 * DS18B20_Temperature.ino - Temperature Sensor Module
 *
 * Handles DS18B20 OneWire temperature sensor
 */

// =============================================================================
// Temperature Sensor Initialization
// =============================================================================
bool temp_init() {
    Serial.println("Initializing DS18B20 temperature sensor...");

    tempSensor.begin();

    // Check for connected devices
    int deviceCount = tempSensor.getDeviceCount();
    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" temperature sensor(s)");

    if (deviceCount == 0) {
        Serial.println("No DS18B20 sensor found!");
        return false;
    }

    // Get address of first sensor
    if (!tempSensor.getAddress(tempDeviceAddress, 0)) {
        Serial.println("Unable to get sensor address");
        return false;
    }

    // Print sensor address
    Serial.print("Sensor address: ");
    for (uint8_t i = 0; i < 8; i++) {
        if (tempDeviceAddress[i] < 16) Serial.print("0");
        Serial.print(tempDeviceAddress[i], HEX);
    }
    Serial.println();

    // Set resolution (9-12 bits, 12 = highest precision)
    tempSensor.setResolution(tempDeviceAddress, 12);

    // Set to non-blocking mode
    tempSensor.setWaitForConversion(false);

    Serial.println("Temperature sensor initialized OK");
    return true;
}

// =============================================================================
// Read Temperature
// =============================================================================
float temp_read() {
    // Request temperature conversion
    tempSensor.requestTemperatures();

    // Wait for conversion (750ms for 12-bit resolution)
    delay(750);

    // Read temperature
    float tempC = tempSensor.getTempC(tempDeviceAddress);

    // Check for read error
    if (tempC == DEVICE_DISCONNECTED_C) {
        Serial.println("Temperature sensor disconnected!");
        return -127.0;
    }

    return tempC;
}

// =============================================================================
// Check Sensor Connection
// =============================================================================
bool temp_isConnected() {
    return tempSensor.isConnected(tempDeviceAddress);
}
