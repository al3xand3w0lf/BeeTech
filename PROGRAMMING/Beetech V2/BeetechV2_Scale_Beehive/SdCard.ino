/*
 * SdCard.ino - SD Card and Configuration Module
 *
 * Handles SD card initialization and config.txt parsing
 */

// =============================================================================
// SD Card Initialization
// =============================================================================
bool sdCard_init() {
    Serial.println("Initializing SD card...");

    // Ensure LCD is deselected so SD has exclusive SPI access
    pinMode(LCD_CS_PIN, OUTPUT);
    digitalWrite(LCD_CS_PIN, HIGH);

    // Ensure SD CS starts HIGH (deselected)
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(10);

    // Retry loop — after deep sleep the SD card often needs multiple attempts
    const int maxRetries = 3;
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        // End any previous SD/SPI session (stale state after deep sleep)
        SD.end();
        SPI.end();
        delay(100);

        // Re-init SPI fresh before each SD attempt
        SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, -1);
        delay(10);

        if (SD.begin(SD_CS_PIN, SPI, 4000000)) {  // 4 MHz — conservative clock
            Serial.print("SD card initialized (attempt ");
            Serial.print(attempt);
            Serial.println(")");
            return true;
        }

        Serial.print("SD init attempt ");
        Serial.print(attempt);
        Serial.print("/");
        Serial.print(maxRetries);
        Serial.println(" failed, retrying...");
        delay(200);
    }

    Serial.println("SD card initialization failed after all retries!");
    return false;
}

// =============================================================================
// Configuration Loading
// =============================================================================
bool sdCard_loadConfig() {
    File configFile = SD.open("/config.txt");

    if (!configFile) {
        Serial.println("Failed to open config.txt");
        return false;
    }

    // Set defaults
    CONFIG.station_number = 0;
    strcpy(CONFIG.station_name, "Unknown");
    CONFIG.calibration_factor = -24200;
    CONFIG.scale_offset = 0;
    CONFIG.dataPoll_interval = 60;
    CONFIG.upload_interval = 300;
    strcpy(CONFIG.wifi_ssid, "");
    strcpy(CONFIG.wifi_password, "");
    // Database defaults
    strcpy(CONFIG.db_server, "");
    CONFIG.db_port = 3306;
    strcpy(CONFIG.db_user, "");
    strcpy(CONFIG.db_password, "");
    strcpy(CONFIG.db_name, "");
    strcpy(CONFIG.db_table, "");
    CONFIG.deep_sleep_enabled = 0;

    char line[128];
    int lineIndex = 0;

    while (configFile.available()) {
        char c = configFile.read();

        if (c == '\n' || c == '\r') {
            if (lineIndex > 0) {
                line[lineIndex] = '\0';
                parseLine(line);
                lineIndex = 0;
            }
        } else {
            if (lineIndex < 127) {
                line[lineIndex++] = c;
            }
        }
    }

    // Parse last line if no newline at end
    if (lineIndex > 0) {
        line[lineIndex] = '\0';
        parseLine(line);
    }

    configFile.close();

    // Validate required fields
    if (strlen(CONFIG.wifi_ssid) == 0) {
        Serial.println("ERROR: wifi_ssid not configured!");
        return false;
    }

    if (strlen(CONFIG.db_server) == 0) {
        Serial.println("ERROR: db_server not configured!");
        return false;
    }

    if (strlen(CONFIG.db_user) == 0) {
        Serial.println("ERROR: db_user not configured!");
        return false;
    }

    if (strlen(CONFIG.db_name) == 0) {
        Serial.println("ERROR: db_name not configured!");
        return false;
    }

    if (strlen(CONFIG.db_table) == 0) {
        Serial.println("ERROR: db_table not configured!");
        return false;
    }

    return true;
}

// =============================================================================
// Line Parser
// =============================================================================
void parseLine(char* line) {
    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == '#') {
        return;
    }

    // Find '=' separator
    char* separator = strchr(line, '=');
    if (separator == NULL) {
        return;
    }

    // Split into key and value
    *separator = '\0';
    char* key = line;
    char* value = separator + 1;

    // Trim whitespace from key
    while (*key == ' ' || *key == '\t') key++;
    char* keyEnd = key + strlen(key) - 1;
    while (keyEnd > key && (*keyEnd == ' ' || *keyEnd == '\t')) {
        *keyEnd = '\0';
        keyEnd--;
    }

    // Trim whitespace from value
    while (*value == ' ' || *value == '\t') value++;
    char* valueEnd = value + strlen(value) - 1;
    while (valueEnd > value && (*valueEnd == ' ' || *valueEnd == '\t')) {
        *valueEnd = '\0';
        valueEnd--;
    }

    // Parse key-value pairs
    if (strcmp(key, "station_number") == 0) {
        CONFIG.station_number = atoi(value);
    }
    else if (strcmp(key, "station_name") == 0) {
        strncpy(CONFIG.station_name, value, sizeof(CONFIG.station_name) - 1);
    }
    else if (strcmp(key, "calibration_factor") == 0) {
        CONFIG.calibration_factor = atol(value);
    }
    else if (strcmp(key, "scale_offset") == 0) {
        CONFIG.scale_offset = atol(value);
    }
    else if (strcmp(key, "dataPoll_interval") == 0) {
        CONFIG.dataPoll_interval = atoi(value);
    }
    else if (strcmp(key, "upload_interval") == 0) {
        CONFIG.upload_interval = atoi(value);
    }
    else if (strcmp(key, "wifi_ssid") == 0) {
        strncpy(CONFIG.wifi_ssid, value, sizeof(CONFIG.wifi_ssid) - 1);
    }
    else if (strcmp(key, "wifi_password") == 0) {
        strncpy(CONFIG.wifi_password, value, sizeof(CONFIG.wifi_password) - 1);
    }
    // Database configuration
    else if (strcmp(key, "db_server") == 0) {
        strncpy(CONFIG.db_server, value, sizeof(CONFIG.db_server) - 1);
    }
    else if (strcmp(key, "db_port") == 0) {
        CONFIG.db_port = atoi(value);
    }
    else if (strcmp(key, "db_user") == 0) {
        strncpy(CONFIG.db_user, value, sizeof(CONFIG.db_user) - 1);
    }
    else if (strcmp(key, "db_password") == 0) {
        strncpy(CONFIG.db_password, value, sizeof(CONFIG.db_password) - 1);
    }
    else if (strcmp(key, "db_name") == 0) {
        strncpy(CONFIG.db_name, value, sizeof(CONFIG.db_name) - 1);
    }
    else if (strcmp(key, "db_table") == 0) {
        strncpy(CONFIG.db_table, value, sizeof(CONFIG.db_table) - 1);
    }
    else if (strcmp(key, "deep_sleep_enabled") == 0) {
        CONFIG.deep_sleep_enabled = atoi(value);
    }

    Serial.print("Config: ");
    Serial.print(key);
    Serial.print(" = ");
    Serial.println(value);
}

// =============================================================================
// Tare Offset File (/tare_offset.txt)
// =============================================================================
bool sdCard_loadOffset() {
    File f = SD.open("/tare_offset.txt");
    if (!f) {
        Serial.println("No tare_offset.txt found - using config/tare");
        return false;
    }

    char buf[32];
    int len = 0;
    while (f.available() && len < 31) {
        char c = f.read();
        if (c == '\n' || c == '\r') break;
        buf[len++] = c;
    }
    buf[len] = '\0';
    f.close();

    if (len > 0) {
        CONFIG.scale_offset = atol(buf);
        Serial.print("Loaded tare offset from file: ");
        Serial.println(CONFIG.scale_offset);
        return true;
    }

    return false;
}

bool sdCard_saveOffset(long offset) {
    // Remove existing file first
    if (SD.exists("/tare_offset.txt")) {
        SD.remove("/tare_offset.txt");
    }

    File f = SD.open("/tare_offset.txt", FILE_WRITE);
    if (!f) {
        Serial.println("ERROR: Cannot write tare_offset.txt");
        return false;
    }

    f.println(offset);
    f.close();

    Serial.print("Tare offset saved to SD: ");
    Serial.println(offset);
    return true;
}
