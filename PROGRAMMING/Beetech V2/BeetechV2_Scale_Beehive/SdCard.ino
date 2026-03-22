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

    // SD spec requires 1ms+ power-up time, be generous after deep sleep
    delay(100);

    // Enable internal pull-up on MISO — ESP-IDF SD driver waits for MISO HIGH
    pinMode(SPI_MISO_PIN, INPUT_PULLUP);

    // Retry loop — full SPI reset + CMD0 before each attempt
    const int maxRetries = 5;
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        SD.end();
        SPI.end();
        delay(100);

        SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, -1);
        SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));

        // Send 80 dummy clock cycles with CS HIGH (SD spec requirement)
        digitalWrite(SD_CS_PIN, HIGH);
        for (int i = 0; i < 10; i++) {
            SPI.transfer(0xFF);
        }

        // Send CMD0 (GO_IDLE_STATE) to force card into idle/SPI mode
        // Critical after warm reset — card may be stuck with CRC ON from previous session
        digitalWrite(SD_CS_PIN, LOW);
        delayMicroseconds(100);
        SPI.transfer(0x40 | 0);   // CMD0
        SPI.transfer(0x00);
        SPI.transfer(0x00);
        SPI.transfer(0x00);
        SPI.transfer(0x00);
        SPI.transfer(0x95);        // Valid CRC for CMD0 (always 0x95)
        // Read R1 response
        for (int i = 0; i < 10; i++) {
            uint8_t r = SPI.transfer(0xFF);
            if (r != 0xFF) {
                Serial.print("CMD0 response: 0x");
                Serial.println(r, HEX);
                break;
            }
        }
        digitalWrite(SD_CS_PIN, HIGH);
        SPI.transfer(0xFF);  // 8 extra clocks

        SPI.endTransaction();
        SPI.end();
        delay(50);

        // Now try SD.begin() with the card in a clean idle state
        SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, -1);

        if (SD.begin(SD_CS_PIN, SPI, 4000000)) {
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
        delay(250 * attempt);
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

// =============================================================================
// NVS (Non-Volatile Storage) — Config Fallback
// =============================================================================
void nvs_saveConfig() {
    Preferences prefs;
    prefs.begin("beetech", true);  // read-only first to check

    // Skip write if NVS already has identical config (preserve flash life)
    bool needsUpdate = !prefs.getBool("valid", false)
        || prefs.getInt("station_num", -1) != CONFIG.station_number
        || prefs.getLong("cal_factor", 0) != CONFIG.calibration_factor
        || prefs.getLong("scale_offset", 0) != CONFIG.scale_offset
        || prefs.getInt("poll_interval", 0) != CONFIG.dataPoll_interval
        || prefs.getInt("upload_interval", 0) != CONFIG.upload_interval
        || prefs.getInt("db_port", 0) != CONFIG.db_port
        || prefs.getInt("deep_sleep", -1) != CONFIG.deep_sleep_enabled;

    // Check strings only if ints match (avoid unnecessary getString calls)
    if (!needsUpdate) {
        char buf[50];
        prefs.getString("wifi_ssid", buf, sizeof(buf));
        needsUpdate = strcmp(buf, CONFIG.wifi_ssid) != 0;
    }

    prefs.end();

    if (!needsUpdate) {
        Serial.println("NVS config unchanged, skipping write");
        return;
    }

    // Config changed — write to NVS
    prefs.begin("beetech", false);  // read-write

    prefs.putInt("station_num", CONFIG.station_number);
    prefs.putString("station_name", CONFIG.station_name);
    prefs.putLong("cal_factor", CONFIG.calibration_factor);
    prefs.putLong("scale_offset", CONFIG.scale_offset);
    prefs.putInt("poll_interval", CONFIG.dataPoll_interval);
    prefs.putInt("upload_interval", CONFIG.upload_interval);
    prefs.putString("wifi_ssid", CONFIG.wifi_ssid);
    prefs.putString("wifi_pass", CONFIG.wifi_password);
    prefs.putString("db_server", CONFIG.db_server);
    prefs.putInt("db_port", CONFIG.db_port);
    prefs.putString("db_user", CONFIG.db_user);
    prefs.putString("db_pass", CONFIG.db_password);
    prefs.putString("db_name", CONFIG.db_name);
    prefs.putString("db_table", CONFIG.db_table);
    prefs.putInt("deep_sleep", CONFIG.deep_sleep_enabled);
    prefs.putBool("valid", true);

    prefs.end();
    Serial.println("Config saved to NVS");
}

bool nvs_loadConfig() {
    Preferences prefs;
    prefs.begin("beetech", true);  // read-only

    if (!prefs.getBool("valid", false)) {
        prefs.end();
        Serial.println("NVS: No valid config stored");
        return false;
    }

    CONFIG.station_number = prefs.getInt("station_num", 0);
    prefs.getString("station_name", CONFIG.station_name, sizeof(CONFIG.station_name));
    CONFIG.calibration_factor = prefs.getLong("cal_factor", -24200);
    CONFIG.scale_offset = prefs.getLong("scale_offset", 0);
    CONFIG.dataPoll_interval = prefs.getInt("poll_interval", 60);
    CONFIG.upload_interval = prefs.getInt("upload_interval", 300);
    prefs.getString("wifi_ssid", CONFIG.wifi_ssid, sizeof(CONFIG.wifi_ssid));
    prefs.getString("wifi_pass", CONFIG.wifi_password, sizeof(CONFIG.wifi_password));
    prefs.getString("db_server", CONFIG.db_server, sizeof(CONFIG.db_server));
    CONFIG.db_port = prefs.getInt("db_port", 3306);
    prefs.getString("db_user", CONFIG.db_user, sizeof(CONFIG.db_user));
    prefs.getString("db_pass", CONFIG.db_password, sizeof(CONFIG.db_password));
    prefs.getString("db_name", CONFIG.db_name, sizeof(CONFIG.db_name));
    prefs.getString("db_table", CONFIG.db_table, sizeof(CONFIG.db_table));
    CONFIG.deep_sleep_enabled = prefs.getInt("deep_sleep", 0);

    prefs.end();
    Serial.println("Config loaded from NVS");
    return true;
}
