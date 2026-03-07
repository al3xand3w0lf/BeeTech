/*
 * Terminal.ino - Serial Terminal Interface
 *
 * Provides debug commands via Serial Monitor
 * Commands:
 *   help     - Show available commands
 *   status   - Show system status
 *   read     - Read current sensor values
 *   tare     - Tare the scale (set current weight as zero)
 *   cal XX   - Calibrate with known weight XX kg
 *   upload   - Force data upload
 *   reset    - Restart the system
 */

#define TERMINAL_BUFFER_SIZE 64

char terminalBuffer[TERMINAL_BUFFER_SIZE];
int terminalIndex = 0;

// =============================================================================
// Process Terminal Input (call from loop)
// =============================================================================
void terminal_process() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (terminalIndex > 0) {
                terminalBuffer[terminalIndex] = '\0';
                terminal_executeCommand(terminalBuffer);
                terminalIndex = 0;
            }
        } else {
            if (terminalIndex < TERMINAL_BUFFER_SIZE - 1) {
                terminalBuffer[terminalIndex++] = c;
            }
        }
    }
}

// =============================================================================
// Execute Terminal Command
// =============================================================================
void terminal_executeCommand(char* cmd) {
    Serial.print("> ");
    Serial.println(cmd);

    // Convert to lowercase for comparison
    char cmdLower[TERMINAL_BUFFER_SIZE];
    strncpy(cmdLower, cmd, TERMINAL_BUFFER_SIZE);
    for (int i = 0; cmdLower[i]; i++) {
        if (cmdLower[i] >= 'A' && cmdLower[i] <= 'Z') {
            cmdLower[i] = cmdLower[i] + 32;
        }
    }

    if (strcmp(cmdLower, "help") == 0) {
        terminal_showHelp();
    }
    else if (strcmp(cmdLower, "status") == 0) {
        terminal_showStatus();
    }
    else if (strcmp(cmdLower, "read") == 0) {
        terminal_readSensors();
    }
    else if (strcmp(cmdLower, "tare") == 0) {
        scale_tare();
        Serial.println("Scale tared");
        Serial.println("Use 'saveoffset' to persist this offset to SD card");
    }
    else if (strcmp(cmdLower, "saveoffset") == 0) {
        long offset = scale.get_offset();
        if (sdCard_saveOffset(offset)) {
            CONFIG.scale_offset = offset;
            Serial.println("Offset saved! You can now enable deep_sleep_enabled=1 in config.txt");
        } else {
            Serial.println("Failed to save offset to SD card");
        }
    }
    else if (strncmp(cmdLower, "cal ", 4) == 0) {
        float knownWeight = atof(cmd + 4);
        if (knownWeight > 0) {
            scale_calibrate(knownWeight);
        } else {
            Serial.println("Usage: cal <weight_in_kg>");
        }
    }
    else if (strcmp(cmdLower, "upload") == 0) {
        terminal_forceUpload();
    }
    else if (strcmp(cmdLower, "reset") == 0) {
        softReset();
    }
    else if (strcmp(cmdLower, "wifi") == 0) {
        terminal_showWiFi();
    }
    else if (strcmp(cmdLower, "db") == 0) {
        terminal_showDatabase();
    }
    else {
        Serial.print("Unknown command: ");
        Serial.println(cmd);
        Serial.println("Type 'help' for available commands");
    }
}

// =============================================================================
// Terminal Commands Implementation
// =============================================================================
void terminal_showHelp() {
    Serial.println();
    Serial.println("=== Beetech V2 Terminal ===");
    Serial.println("Available commands:");
    Serial.println("  help       - Show this help");
    Serial.println("  status     - Show system status");
    Serial.println("  read       - Read current sensor values");
    Serial.println("  tare       - Tare the scale");
    Serial.println("  saveoffset - Save tare offset to SD card");
    Serial.println("  cal XX     - Calibrate with known weight (kg)");
    Serial.println("  upload     - Force data upload");
    Serial.println("  wifi       - Show WiFi status");
    Serial.println("  db         - Show database status");
    Serial.println("  reset      - Restart system");
    Serial.println();
}

void terminal_showStatus() {
    Serial.println();
    Serial.println("=== System Status ===");

    Serial.print("Station: ");
    Serial.print(CONFIG.station_number);
    Serial.print(" - ");
    Serial.println(CONFIG.station_name);

    Serial.print("Scale connected: ");
    Serial.println(SENSOR_DATA.scale_connected ? "Yes" : "No");

    Serial.print("Temp sensor connected: ");
    Serial.println(SENSOR_DATA.temp_connected ? "Yes" : "No");

    Serial.print("WiFi connected: ");
    Serial.println(wifi_isConnected() ? "Yes" : "No");

    Serial.print("Database connected: ");
    Serial.println(db_isConnected() ? "Yes" : "No");

    Serial.print("Poll interval: ");
    Serial.print(CONFIG.dataPoll_interval);
    Serial.println(" sec");

    Serial.print("Upload interval: ");
    Serial.print(CONFIG.upload_interval);
    Serial.println(" sec");

    Serial.print("Scale offset: ");
    Serial.println(scale.get_offset());

    Serial.print("Stored offset: ");
    Serial.println(CONFIG.scale_offset);

    Serial.print("Deep sleep: ");
    Serial.println(CONFIG.deep_sleep_enabled ? "Enabled" : "Disabled");

    Serial.print("Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    Serial.println();
}

void terminal_readSensors() {
    Serial.println();
    Serial.println("=== Sensor Readings ===");

    if (SENSOR_DATA.scale_connected) {
        float weight = scale_read();
        Serial.print("Weight: ");
        Serial.print(weight, 2);
        Serial.println(" kg");
    } else {
        Serial.println("Weight: N/A (not connected)");
    }

    if (SENSOR_DATA.temp_connected) {
        float temp = temp_read();
        Serial.print("Temperature: ");
        Serial.print(temp, 1);
        Serial.println(" C");
    } else {
        Serial.println("Temperature: N/A (not connected)");
    }

    Serial.println();
}

void terminal_forceUpload() {
    Serial.println("Forcing data upload...");

    if (db_uploadData(CONFIG.station_number, CONFIG.station_name,
                      SENSOR_DATA.temperature, SENSOR_DATA.weight)) {
        Serial.println("Upload successful");
    } else {
        Serial.println("Upload failed");
    }
}

void terminal_showWiFi() {
    Serial.println();
    Serial.println("=== WiFi Status ===");

    Serial.print("SSID: ");
    Serial.println(CONFIG.wifi_ssid);

    Serial.print("Status: ");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("Disconnected");
    }

    Serial.println();
}

void terminal_showDatabase() {
    Serial.println();
    Serial.println("=== MySQL Database Status ===");

    Serial.print("Server: ");
    Serial.print(CONFIG.db_server);
    Serial.print(":");
    Serial.println(CONFIG.db_port);

    Serial.print("Database: ");
    Serial.println(CONFIG.db_name);

    Serial.print("Table: ");
    Serial.println(CONFIG.db_table);

    Serial.print("User: ");
    Serial.println(CONFIG.db_user);

    Serial.print("Connected: ");
    Serial.println(db_isConnected() ? "Yes" : "No");

    Serial.println();
}
