/*
 * SH1106_OLED.ino - OLED Display Module
 *
 * Handles SH1106 128x64 I2C OLED display
 */

// =============================================================================
// OLED Initialization
// =============================================================================
void oled_init() {
    Serial.println("Initializing OLED display...");

    oled.setI2CAddress(0x3C * 2);  // U8g2 erwartet 8-bit Adresse (0x3C << 1 = 0x78)
    oled.begin();
    oled.setFont(u8g2_font_6x12_tf);
    oled.clearBuffer();
    oled.sendBuffer();

    Serial.println("OLED initialized OK");
}

// =============================================================================
// Show Startup Screen
// =============================================================================
void oled_showStartup() {
    oled.clearBuffer();

    oled.setFont(u8g2_font_helvB12_tf);
    oled.drawStr(20, 25, "Beetech");
    oled.drawStr(40, 45, "V2");

    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(25, 60, "Starting...");

    oled.sendBuffer();
}

// =============================================================================
// Show Sensor Data
// =============================================================================
void oled_showData(float weight, float temp) {
    oled.clearBuffer();

    // Title bar
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 10, "Beetech V2");

    // Draw separator line
    oled.drawHLine(0, 14, 128);

    // Weight - large font
    oled.setFont(u8g2_font_logisoso24_tf);

    char weightStr[16];
    if (weight < 0 || weight > 999) {
        snprintf(weightStr, sizeof(weightStr), "--.-");
    } else {
        dtostrf(weight, 5, 1, weightStr);
    }
    oled.drawStr(0, 45, weightStr);

    // kg unit
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(85, 45, "kg");

    // Temperature
    char tempStr[16];
    if (temp < -100 || temp > 100) {
        snprintf(tempStr, sizeof(tempStr), "Temp: --.- C");
    } else {
        char tempVal[8];
        dtostrf(temp, 4, 1, tempVal);
        snprintf(tempStr, sizeof(tempStr), "Temp: %s C", tempVal);
    }
    oled.drawStr(0, 60, tempStr);

    oled.sendBuffer();
}

// =============================================================================
// Show Status Message
// =============================================================================
void oled_showStatus(const char* status) {
    oled.clearBuffer();

    oled.setFont(u8g2_font_helvB12_tf);
    oled.drawStr(20, 25, "Beetech");
    oled.drawStr(40, 45, "V2");

    oled.setFont(u8g2_font_6x12_tf);

    // Center the status text
    int width = oled.getStrWidth(status);
    int x = (128 - width) / 2;
    oled.drawStr(x, 60, status);

    oled.sendBuffer();
}

// =============================================================================
// Show Error Message
// =============================================================================
void oled_showError(const char* error) {
    oled.clearBuffer();

    oled.setFont(u8g2_font_helvB12_tf);
    oled.drawStr(30, 25, "ERROR");

    oled.setFont(u8g2_font_6x12_tf);

    // Center the error text
    int width = oled.getStrWidth(error);
    int x = (128 - width) / 2;
    oled.drawStr(x, 45, error);

    oled.sendBuffer();
}

// =============================================================================
// Show Upload Status
// =============================================================================
void oled_showUploading() {
    // Save current content and show uploading indicator
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(100, 10, "TX");
    oled.sendBuffer();
}

// =============================================================================
// Power Management
// =============================================================================
void oled_off() {
    oled.setPowerSave(1);
}

void oled_on() {
    oled.setPowerSave(0);
}
