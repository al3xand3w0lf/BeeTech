/*
 * ST7567_LCD.ino - LCD Display Module
 *
 * Handles ST7567 128x64 SPI LCD (AiP31567 driver)
 */

// =============================================================================
// LCD Initialization
// =============================================================================
void lcd_init() {
    Serial.println("Initializing ST7567 LCD...");

    // Backlight on (active LOW)
    pinMode(LCD_BL_PIN, OUTPUT);
    digitalWrite(LCD_BL_PIN, LOW);

    lcd.begin();
    lcd.setContrast(200);
    lcd.setFont(u8g2_font_6x12_tf);
    lcd.clearBuffer();
    lcd.sendBuffer();

    Serial.println("LCD initialized OK");
}

// =============================================================================
// Show Startup Screen
// =============================================================================
void lcd_showStartup() {
    lcd.clearBuffer();

    lcd.setFont(u8g2_font_helvB12_tf);
    lcd.drawStr(20, 25, "Beetech");
    lcd.drawStr(40, 45, "V2");

    lcd.setFont(u8g2_font_6x12_tf);
    lcd.drawStr(25, 60, "Starting...");

    lcd.sendBuffer();
}

// =============================================================================
// Show Sensor Data
// =============================================================================
void lcd_showData(float weight, float temp) {
    lcd.clearBuffer();

    // Title bar
    lcd.setFont(u8g2_font_6x12_tf);
    lcd.drawStr(0, 10, "Beetech V2");

    // Draw separator line
    lcd.drawHLine(0, 14, 128);

    // Weight - large font
    lcd.setFont(u8g2_font_logisoso24_tf);

    char weightStr[16];
    if (weight < 0 || weight > 999) {
        snprintf(weightStr, sizeof(weightStr), "--.-");
    } else {
        dtostrf(weight, 5, 1, weightStr);
    }
    lcd.drawStr(0, 45, weightStr);

    // kg unit
    lcd.setFont(u8g2_font_6x12_tf);
    lcd.drawStr(85, 45, "kg");

    // Temperature
    char tempStr[16];
    if (temp < -100 || temp > 100) {
        snprintf(tempStr, sizeof(tempStr), "Temp: --.- C");
    } else {
        char tempVal[8];
        dtostrf(temp, 4, 1, tempVal);
        snprintf(tempStr, sizeof(tempStr), "Temp: %s C", tempVal);
    }
    lcd.drawStr(0, 60, tempStr);

    lcd.sendBuffer();
}

// =============================================================================
// Show Status Message
// =============================================================================
void lcd_showStatus(const char* status) {
    lcd.clearBuffer();

    lcd.setFont(u8g2_font_helvB12_tf);
    lcd.drawStr(20, 25, "Beetech");
    lcd.drawStr(40, 45, "V2");

    lcd.setFont(u8g2_font_6x12_tf);

    // Center the status text
    int width = lcd.getStrWidth(status);
    int x = (128 - width) / 2;
    lcd.drawStr(x, 60, status);

    lcd.sendBuffer();
}

// =============================================================================
// Show Error Message
// =============================================================================
void lcd_showError(const char* error) {
    lcd.clearBuffer();

    lcd.setFont(u8g2_font_helvB12_tf);
    lcd.drawStr(30, 25, "ERROR");

    lcd.setFont(u8g2_font_6x12_tf);

    // Center the error text
    int width = lcd.getStrWidth(error);
    int x = (128 - width) / 2;
    lcd.drawStr(x, 45, error);

    lcd.sendBuffer();
}

// =============================================================================
// Show Upload Status
// =============================================================================
void lcd_showUploading() {
    lcd.setFont(u8g2_font_6x12_tf);
    lcd.drawStr(100, 10, "TX");
    lcd.sendBuffer();
}

// =============================================================================
// Backlight Control
// =============================================================================
void lcd_backlightOn() {
    digitalWrite(LCD_BL_PIN, LOW);   // Active LOW
}

void lcd_backlightOff() {
    digitalWrite(LCD_BL_PIN, HIGH);
}

// =============================================================================
// Power Management
// =============================================================================
void lcd_off() {
    lcd.setPowerSave(1);
    lcd_backlightOff();
}

void lcd_on() {
    lcd.setPowerSave(0);
    lcd_backlightOn();
}
