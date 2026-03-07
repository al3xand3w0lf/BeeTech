/*
 * Database.ino - WiFi and MySQL Direct Connection Module
 *
 * Direct MySQL connection using ESP32_MySQL library
 * https://github.com/Syafiqlim/ESP32_MySQL
 *
 * WICHTIG: Falls "Error reading auth packets" erscheint:
 * MySQL 8+ verwendet standardmaessig caching_sha2_password.
 * Loesung auf MySQL Server:
 *   ALTER USER 'username'@'%' IDENTIFIED WITH mysql_native_password BY 'password';
 *   FLUSH PRIVILEGES;
 */

#define WIFI_CONNECT_TIMEOUT_MS  30000   // full timeout for reconnect
#define WIFI_STARTUP_TIMEOUT_MS  5000   // short timeout at startup

// Server IP address (parsed from config)
IPAddress db_server_ip;

// =============================================================================
// WiFi Connection
// =============================================================================
bool wifi_connect() {
    return wifi_connectWithTimeout(WIFI_CONNECT_TIMEOUT_MS);
}

bool wifi_connectStartup() {
    return wifi_connectWithTimeout(WIFI_STARTUP_TIMEOUT_MS);
}

bool wifi_connectWithTimeout(unsigned long timeoutMs) {
    Serial.println("Connecting to WiFi...");

    Serial.print("SSID: ");
    Serial.println(CONFIG.wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(CONFIG.wifi_ssid, CONFIG.wifi_password);

    unsigned long startTime = millis();

    int dots = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeoutMs) {
            Serial.println("\nWiFi connection timeout!");
            return false;
        }
        delay(500);
        Serial.print(".");

        // Update LCD with animated dots
        dots = (dots % 3) + 1;
        char buf[20];
        snprintf(buf, sizeof(buf), "Connecting%.*s", dots, "...");
        lcd_showBootStep("WiFi", buf);
    }

    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    return true;
}

// =============================================================================
// WiFi Reconnection
// =============================================================================
bool wifi_reconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.reconnect();

    int timeout = 15;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(1000);
        Serial.print(".");
        timeout--;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi reconnect FAILED!");
        // Try full reconnect
        WiFi.disconnect();
        delay(1000);
        return wifi_connect();
    }

    Serial.println("\nWiFi reconnected.");
    return true;
}

// =============================================================================
// WiFi Status
// =============================================================================
bool wifi_isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

// =============================================================================
// TCP Connectivity Test (before MySQL)
// =============================================================================
bool db_testTCPConnection() {
    Serial.println("--- TCP Connection Test ---");
    Serial.print("Connecting to ");
    Serial.print(db_server_ip);
    Serial.print(":");
    Serial.print(CONFIG.db_port);
    Serial.print(" ... ");

    WiFiClient testClient;
    unsigned long startTime = millis();
    bool connected = testClient.connect(db_server_ip, CONFIG.db_port);
    unsigned long elapsed = millis() - startTime;

    if (connected) {
        Serial.print("OK (");
        Serial.print(elapsed);
        Serial.println(" ms)");
        testClient.stop();
        return true;
    } else {
        Serial.print("FAILED after ");
        Serial.print(elapsed);
        Serial.println(" ms");
        Serial.println("  -> Server nicht erreichbar (Firewall/Port/IP pruefen)");
        return false;
    }
}

// =============================================================================
// MySQL Database Connection
// =============================================================================
bool db_connect() {
    // Ensure WiFi is connected first
    if (!wifi_isConnected()) {
        Serial.println("Cannot connect to DB: WiFi not connected");
        return false;
    }

    // Diagnostic info
    Serial.print("WiFi RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.print(" dBm | Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    // Parse server IP from string
    if (!db_server_ip.fromString(CONFIG.db_server)) {
        Serial.print("ERROR: Invalid DB server IP: ");
        Serial.println(CONFIG.db_server);
        return false;
    }

    // TCP test first
    if (!db_testTCPConnection()) {
        Serial.println("Skipping MySQL connection (TCP failed).");
        return false;
    }

    Serial.println();
    Serial.println("--- MySQL Connection ---");
    Serial.print("Server: ");
    Serial.print(CONFIG.db_server);
    Serial.print(":");
    Serial.println(CONFIG.db_port);
    Serial.print("User: ");
    Serial.println(CONFIG.db_user);
    Serial.print("Database: ");
    Serial.println(CONFIG.db_name);

    Serial.println("Attempting MySQL connection...");
    unsigned long startTime = millis();

    int result = conn.connectNonBlocking(db_server_ip, CONFIG.db_port,
                                          CONFIG.db_user, CONFIG.db_password);

    unsigned long elapsed = millis() - startTime;
    Serial.print("Connection attempt took ");
    Serial.print(elapsed);
    Serial.println(" ms");

    if (result != RESULT_FAIL) {
        Serial.println(">>> DATABASE CONNECTION SUCCESSFUL! <<<");
        return true;
    } else {
        Serial.println(">>> DATABASE CONNECTION FAILED <<<");
        Serial.println();
        Serial.println("Moegliche Ursachen:");
        Serial.println("1. Falscher User/Password");
        Serial.println("2. MySQL 8+ mit caching_sha2_password");
        Serial.println("3. User hat keine Berechtigung von dieser IP");
        Serial.println();
        Serial.println("Loesung fuer MySQL 8+:");
        Serial.println("  ALTER USER 'user'@'%' IDENTIFIED WITH mysql_native_password BY 'password';");
        Serial.println("  FLUSH PRIVILEGES;");
        return false;
    }
}

// =============================================================================
// Database Connection Status
// =============================================================================
bool db_isConnected() {
    return conn.connected();
}

// =============================================================================
// Upload Data to MySQL
// =============================================================================
bool db_uploadData(int stationNum, const char* stationName, float temp, float weight) {
    // Ensure WiFi is connected
    if (!wifi_reconnect()) {
        Serial.println("Cannot upload: WiFi not connected");
        return false;
    }

    // Check/restore database connection
    if (!conn.connected()) {
        Serial.println("Database disconnected, reconnecting...");
        conn.close();
        delay(500);

        if (!db_connect()) {
            Serial.println("Cannot upload: Database reconnection failed");
            return false;
        }
    }

    // Build SQL INSERT statement
    char sqlBuffer[250];
    snprintf(sqlBuffer, sizeof(sqlBuffer),
        "INSERT INTO %s.%s (location,hive,temp1,temp2,weight,sound) VALUES ('%s',%d,%.1f,0.0,%.1f,0)",
        CONFIG.db_name,
        CONFIG.db_table,
        stationName,
        stationNum,
        temp,
        weight
    );

    Serial.println("---");
    Serial.println("Executing SQL:");
    Serial.println(sqlBuffer);
    Serial.println("---");

    // Execute query using ESP32_MySQL_Query
    unsigned long startTime = millis();
    ESP32_MySQL_Query query_mem = ESP32_MySQL_Query(&conn);
    bool success = query_mem.execute(sqlBuffer);
    unsigned long elapsed = millis() - startTime;

    Serial.print("Query took ");
    Serial.print(elapsed);
    Serial.println(" ms");

    if (success) {
        Serial.println(">>> SQL executed successfully <<<");
        return true;
    } else {
        Serial.println(">>> SQL execution failed <<<");
        Serial.println("  -> Tabelle/Spalten pruefen oder INSERT-Recht fehlt");
        return false;
    }
}

// =============================================================================
// Manual Database Test (for terminal command)
// =============================================================================
void db_test() {
    Serial.println();
    Serial.println("=== Full Database Connection Test ===");

    if (!wifi_isConnected()) {
        Serial.println("WiFi not connected!");
        return;
    }

    Serial.print("WiFi RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.print(" dBm | Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println();

    // Parse server IP
    if (!db_server_ip.fromString(CONFIG.db_server)) {
        Serial.print("ERROR: Invalid DB server IP: ");
        Serial.println(CONFIG.db_server);
        return;
    }

    // TCP test
    if (!db_testTCPConnection()) {
        return;
    }
    Serial.println();

    // MySQL connection test
    if (conn.connected()) {
        conn.close();
        delay(500);
    }

    Serial.println("--- MySQL Connection Test ---");
    Serial.print("Server: ");
    Serial.print(CONFIG.db_server);
    Serial.print(":");
    Serial.println(CONFIG.db_port);
    Serial.print("User: ");
    Serial.println(CONFIG.db_user);

    unsigned long startTime = millis();
    int result = conn.connectNonBlocking(db_server_ip, CONFIG.db_port,
                                          CONFIG.db_user, CONFIG.db_password);
    unsigned long elapsed = millis() - startTime;

    Serial.print("Connection attempt took ");
    Serial.print(elapsed);
    Serial.println(" ms");

    if (result != RESULT_FAIL) {
        Serial.println(">>> CONNECTION SUCCESSFUL! <<<");
        Serial.println();

        // Test SELECT - check table structure
        Serial.println("--- Testing SELECT ---");
        char sqlBuffer[250];
        snprintf(sqlBuffer, sizeof(sqlBuffer),
            "SELECT COUNT(*) FROM %s.%s", CONFIG.db_name, CONFIG.db_table);
        Serial.print("SQL: ");
        Serial.println(sqlBuffer);

        ESP32_MySQL_Query query = ESP32_MySQL_Query(&conn);
        if (query.execute(sqlBuffer)) {
            row_values *row = query.get_next_row();
            if (row && row->values[0]) {
                Serial.print("Rows in table: ");
                Serial.println(row->values[0]);
            }
            Serial.println(">>> SELECT OK <<<");
        } else {
            Serial.println(">>> SELECT FAILED <<<");
            Serial.println("  -> Tabelle existiert nicht oder kein Leserecht");
        }

        conn.close();
        Serial.println("\nConnection closed.");
    } else {
        Serial.println(">>> CONNECTION FAILED <<<");
        Serial.println();
        Serial.println("Moegliche Ursachen:");
        Serial.println("1. Falscher User/Password");
        Serial.println("2. MySQL 8+ mit caching_sha2_password");
        Serial.println("3. User hat keine Berechtigung von dieser IP");
        Serial.println();
        Serial.println("Loesung fuer MySQL 8+:");
        Serial.println("  ALTER USER 'user'@'%' IDENTIFIED WITH mysql_native_password BY 'password';");
        Serial.println("  FLUSH PRIVILEGES;");
    }

    Serial.println();
}
