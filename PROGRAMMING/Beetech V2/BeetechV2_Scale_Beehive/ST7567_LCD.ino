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
    lcd.clearBuffer();
    lcd.sendBuffer();

    Serial.println("LCD initialized OK");
}

// =============================================================================
// Helper: draw centered string
// =============================================================================
static void lcd_drawCentered(int y, const char* text) {
    int w = lcd.getStrWidth(text);
    lcd.drawStr((128 - w) / 2, y, text);
}

// =============================================================================
// Show Startup Screen (Splash)
// =============================================================================
void lcd_showStartup() {
    lcd.clearBuffer();

    // Big title
    lcd.setFont(u8g2_font_logisoso24_tf);
    lcd_drawCentered(32, "Beetech");

    // Version
    lcd.setFont(u8g2_font_helvB14_tf);
    char versionStr[20];
    snprintf(versionStr, sizeof(versionStr), "V2  v%s", FW_VERSION);
    lcd_drawCentered(54, versionStr);

    // Bottom line
    lcd.setFont(u8g2_font_6x12_tf);
    lcd_drawCentered(64, "Booting...");

    lcd.sendBuffer();
}

// =============================================================================
// Boot Step Display — shows icon + step name + status
// =============================================================================
void lcd_showBootStep(const char* step, const char* status) {
    lcd.clearBuffer();

    // Step name large centered
    lcd.setFont(u8g2_font_helvB14_tf);
    lcd_drawCentered(28, step);

    // Separator
    lcd.drawHLine(10, 36, 108);

    // Status below
    lcd.setFont(u8g2_font_helvB12_tf);
    lcd_drawCentered(56, status);

    lcd.sendBuffer();
}

// =============================================================================
// Show Sensor Data (main loop display)
// =============================================================================
void lcd_showData(float weight, float temp) {
    lcd.clearBuffer();

    // Station header (top line)
    lcd.setFont(u8g2_font_6x12_tf);
    char stationStr[60];
    snprintf(stationStr, sizeof(stationStr), "#%d %s", CONFIG.station_number, CONFIG.station_name);
    lcd_drawCentered(10, stationStr);
    lcd.drawHLine(0, 12, 128);

    // Weight — dominant, left-aligned
    lcd.setFont(u8g2_font_logisoso24_tf);
    char weightStr[16];
    if (weight < 0 || weight > 999) {
        snprintf(weightStr, sizeof(weightStr), "--.-");
    } else {
        dtostrf(weight, 5, 1, weightStr);
    }
    lcd.drawStr(0, 40, weightStr);

    // "kg" right of weight value
    int weightWidth = lcd.getStrWidth(weightStr);
    lcd.setFont(u8g2_font_helvB12_tf);
    lcd.drawStr(weightWidth + 4, 40, "kg");

    // Separator
    lcd.drawHLine(0, 46, 128);

    // Temperature bottom
    lcd.setFont(u8g2_font_helvB12_tf);
    char tempStr[16];
    if (temp < -100 || temp > 100) {
        snprintf(tempStr, sizeof(tempStr), "Temp: --.- C");
    } else {
        char tempVal[8];
        dtostrf(temp, 4, 1, tempVal);
        snprintf(tempStr, sizeof(tempStr), "Temp: %s C", tempVal);
    }
    lcd_drawCentered(62, tempStr);

    lcd.sendBuffer();
}

// =============================================================================
// Show Status Message (generic, large)
// =============================================================================
void lcd_showStatus(const char* status) {
    lcd_showBootStep("Beetech V2", status);
}

// =============================================================================
// Show Error Message (inverted header bar)
// =============================================================================
void lcd_showError(const char* error) {
    lcd.clearBuffer();

    // Inverted top bar
    lcd.drawBox(0, 0, 128, 22);
    lcd.setDrawColor(0);
    lcd.setFont(u8g2_font_helvB14_tf);
    lcd_drawCentered(17, "ERROR");
    lcd.setDrawColor(1);

    // Error message large
    lcd.setFont(u8g2_font_helvB12_tf);
    lcd_drawCentered(48, error);

    lcd.sendBuffer();
}

// =============================================================================
// Show Upload Status (overlay on current screen)
// =============================================================================
void lcd_showUploading() {
    // Small inverted badge top-right
    lcd.drawBox(104, 0, 24, 12);
    lcd.setDrawColor(0);
    lcd.setFont(u8g2_font_6x12_tf);
    lcd.drawStr(107, 11, "TX");
    lcd.setDrawColor(1);
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
