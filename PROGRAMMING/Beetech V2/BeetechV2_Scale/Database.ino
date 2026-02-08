/*
 * Database.ino - WiFi and MySQL Direct Connection Module
 *
 * Direct MySQL connection using ESP32_MySQL library
 * https://github.com/Syafiqlim/ESP32_MySQL
 */

#define WIFI_CONNECT_TIMEOUT_MS  30000

// Server IP address (parsed from config)
IPAddress db_server_ip;

// =============================================================================
// WiFi Connection
// =============================================================================
bool wifi_connect() {
    Serial.println("Connecting to WiFi...");
    Serial.print("SSID: ");
    Serial.println(CONFIG.wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(CONFIG.wifi_ssid, CONFIG.wifi_password);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("WiFi connection timeout!");
            return false;
        }
        delay(500);
        Serial.print(".");
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
    WiFi.disconnect();
    delay(1000);

    return wifi_connect();
}

// =============================================================================
// WiFi Status
// =============================================================================
bool wifi_isConnected() {
    return (WiFi.status() == WL_CONNECTED);
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

    // Parse server IP from string
    if (!db_server_ip.fromString(CONFIG.db_server)) {
        Serial.print("ERROR: Invalid DB server IP: ");
        Serial.println(CONFIG.db_server);
        return false;
    }

    Serial.print("Connecting to MySQL server: ");
    Serial.print(CONFIG.db_server);
    Serial.print(":");
    Serial.println(CONFIG.db_port);

    // Connect to MySQL using ESP32_MySQL library
    // Use connectNonBlocking for more stability
    if (conn.connectNonBlocking(db_server_ip, CONFIG.db_port,
                                 CONFIG.db_user, CONFIG.db_password) != RESULT_FAIL) {
        Serial.println("Database connection successful!");
        return true;
    } else {
        Serial.println("Database connection failed!");
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
    // Format: INSERT INTO db.table (location,hive,temp1,temp2,weight,sound) VALUES ('name',num,t1,t2,w,s)
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
    ESP32_MySQL_Query query_mem = ESP32_MySQL_Query(&conn);

    if (query_mem.execute(sqlBuffer)) {
        Serial.println("SQL executed successfully");
        return true;
    } else {
        Serial.println("SQL execution failed");
        return false;
    }
}

// =============================================================================
// Manual Database Test (for terminal command)
// =============================================================================
void db_test() {
    Serial.println("Testing database connection...");

    if (!wifi_isConnected()) {
        Serial.println("WiFi not connected!");
        return;
    }

    if (conn.connected()) {
        Serial.println("Database already connected");
    } else {
        if (db_connect()) {
            Serial.println("Database connection OK");
        } else {
            Serial.println("Database connection FAILED");
        }
    }
}
