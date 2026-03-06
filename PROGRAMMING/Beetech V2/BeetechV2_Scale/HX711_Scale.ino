/*
 * HX711_Scale.ino - Load Cell / Scale Module
 *
 * Handles HX711 ADC for 4 load cells in Wheatstone bridge configuration
 */

// =============================================================================
// Scale Initialization
// =============================================================================
bool scale_init() {
    Serial.println("Initializing HX711 scale...");

    scale.begin(HX711_DOUT_PIN, HX711_CLK_PIN);

    // Wait for HX711 to be ready
    unsigned long timeout = millis() + 2000;
    while (!scale.is_ready()) {
        if (millis() > timeout) {
            Serial.println("HX711 timeout - not responding");
            return false;
        }
        delay(10);
    }

    // Set calibration factor from config
    scale.set_scale(CONFIG.calibration_factor);

    // Apply offset if configured
    if (CONFIG.scale_offset != 0.0) {
        scale.set_offset((long)(CONFIG.scale_offset * CONFIG.calibration_factor));
    } else {
        // Tare the scale (set current weight as zero)
        Serial.println("Taring scale...");
        scale.tare(10);  // Average of 10 readings
    }

    Serial.print("Scale calibration factor: ");
    Serial.println(CONFIG.calibration_factor);
    Serial.print("Scale offset: ");
    Serial.println(scale.get_offset());

    return true;
}

// =============================================================================
// Read Weight
// =============================================================================
float scale_read() {
    if (!scale.is_ready()) {
        Serial.println("Scale not ready");
        return 0.0;
    }

    // Get average of multiple readings for stability
    float weight = scale.get_units(5);  // Average of 5 readings

    // Ensure non-negative (small fluctuations around zero)
    if (weight < 0 && weight > -0.5) {
        weight = 0.0;
    }

    return weight;
}

// =============================================================================
// Scale Calibration (for Terminal command)
// =============================================================================
void scale_tare() {
    Serial.println("Taring scale...");
    scale.tare(10);
    Serial.print("New offset: ");
    Serial.println(scale.get_offset());
}

void scale_calibrate(float knownWeight) {
    Serial.print("Calibrating with known weight: ");
    Serial.print(knownWeight);
    Serial.println(" kg");

    // Read raw value
    scale.set_scale();  // Reset scale factor
    float rawValue = scale.get_units(10);

    // Calculate new calibration factor
    long newFactor = (long)(rawValue / knownWeight);

    Serial.print("Raw value: ");
    Serial.println(rawValue);
    Serial.print("New calibration factor: ");
    Serial.println(newFactor);

    scale.set_scale(newFactor);
}

// =============================================================================
// Power Management
// =============================================================================
void scale_powerDown() {
    scale.power_down();
}

void scale_powerUp() {
    scale.power_up();
    delay(50);  // Allow HX711 to stabilize
}
