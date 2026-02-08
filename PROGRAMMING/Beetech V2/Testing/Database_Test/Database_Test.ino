/*
 * Database_Test.ino - MySQL Connection Test for Beetech V2
 *
 * Tests direct MySQL connection using ESP32_MySQL library
 * Use Serial Monitor to see results
 *
 * WICHTIG: Falls "Error reading auth packets" erscheint:
 * MySQL 8+ verwendet standardmäßig caching_sha2_password.
 * Lösung auf MySQL Server:
 *   ALTER USER 'k139859_n'@'%' IDENTIFIED WITH mysql_native_password BY '22222';
 *   FLUSH PRIVILEGES;
 */

#include <WiFi.h>

// ESP32_MySQL Library
#define ESP32_MYSQL_DEBUG_PORT Serial
#define _ESP32_MYSQL_LOGLEVEL_ 2  // Higher debug level
#include <ESP32_MySQL.h>

// =============================================================================
// Configuration - EDIT THESE VALUES
// =============================================================================
const char* WIFI_SSID     = "FibreBox_X6-2FF847";
const char* WIFI_PASSWORD = "WallaWalla40!";

/*
const char* WIFI_SSID     = "ETHIGPWIFI";
const char* WIFI_PASSWORD = "ETHIGPWIFI";
*/
// MySQL Server
IPAddress server_ip(91, 204, 46, 146);
uint16_t  server_port = 3306;
char      db_user[]     = "k139859_n";
char      db_password[] = "22222";
char      db_name[]     = "k139859_n";
char      db_table[]    = "BeetechData03";

// =============================================================================
// Objects
// =============================================================================
WiFiClient wifiClient;
ESP32_MySQL_Connection conn((Client *)&wifiClient);

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("  Beetech V2 - Database Test");
    Serial.println("================================");
    Serial.println();

    // Connect WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(1000);
        Serial.print(".");
        timeout--;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi connection FAILED!");
        Serial.println("Check SSID and password");
        while(1) delay(1000);
    }

    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println();

    // Test MySQL connection
    testMySQLConnection();
}

// =============================================================================
// TCP Connectivity Test (before MySQL)
// =============================================================================
bool testTCPConnection() {
    Serial.println("--- TCP Connection Test ---");
    Serial.print("Connecting to ");
    Serial.print(server_ip);
    Serial.print(":");
    Serial.print(server_port);
    Serial.print(" ... ");

    WiFiClient testClient;
    unsigned long startTime = millis();
    bool connected = testClient.connect(server_ip, server_port);
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
        Serial.println("  -> Server nicht erreichbar (Firewall/Port/IP prüfen)");
        return false;
    }
}

// =============================================================================
// Test MySQL Connection
// =============================================================================
void testMySQLConnection() {
    // WiFi still connected?
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("!!! WiFi disconnected - reconnecting...");
        WiFi.reconnect();
        int timeout = 15;
        while (WiFi.status() != WL_CONNECTED && timeout > 0) {
            delay(1000);
            Serial.print(".");
            timeout--;
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\nWiFi reconnect FAILED!");
            return;
        }
        Serial.println("\nWiFi reconnected.");
    }

    Serial.print("WiFi RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.print(" dBm | Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println();

    // TCP test first
    if (!testTCPConnection()) {
        Serial.println("Skipping MySQL test (TCP failed).");
        return;
    }
    Serial.println();

    Serial.println("--- MySQL Connection Test ---");
    Serial.print("Server: ");
    Serial.print(server_ip);
    Serial.print(":");
    Serial.println(server_port);
    Serial.print("User: ");
    Serial.println(db_user);
    Serial.print("Database: ");
    Serial.println(db_name);
    Serial.println();

    Serial.println("Attempting MySQL connection...");
    unsigned long startTime = millis();

    int result = conn.connectNonBlocking(server_ip, server_port, db_user, db_password);

    unsigned long elapsed = millis() - startTime;
    Serial.print("Connection attempt took ");
    Serial.print(elapsed);
    Serial.println(" ms");

    if (result != RESULT_FAIL) {
        Serial.println(">>> CONNECTION SUCCESSFUL! <<<");
        Serial.println();

        // Test SELECT first (read)
        testSelect();

        // Test INSERT (write)
        testInsert();

        conn.close();
        Serial.println("Connection closed.");
    } else {
        Serial.println(">>> CONNECTION FAILED <<<");
        Serial.println();
        Serial.println("Mögliche Ursachen:");
        Serial.println("1. Falscher User/Password");
        Serial.println("2. MySQL 8+ mit caching_sha2_password");
        Serial.println("3. User hat keine Berechtigung von dieser IP");
        Serial.println();
        Serial.println("Lösung für MySQL 8+:");
        Serial.println("  ALTER USER 'k139859_n'@'%' IDENTIFIED WITH mysql_native_password BY '22222';");
        Serial.println("  FLUSH PRIVILEGES;");
    }
}

// =============================================================================
// Test SELECT - verify table exists and check structure
// =============================================================================
void testSelect() {
    Serial.println("--- Testing SELECT ---");

    char sqlBuffer[250];

    // Check table columns
    snprintf(sqlBuffer, sizeof(sqlBuffer),
        "DESCRIBE %s.%s", db_name, db_table);

    Serial.print("SQL: ");
    Serial.println(sqlBuffer);

    ESP32_MySQL_Query query = ESP32_MySQL_Query(&conn);

    if (query.execute(sqlBuffer)) {
        Serial.println("Table structure:");
        column_names *cols = query.get_columns();
        if (cols) {
            for (int i = 0; i < cols->num_fields; i++) {
                Serial.print("  ");
                Serial.print(cols->fields[i]->name);
                if (i < cols->num_fields - 1) Serial.print(" | ");
            }
            Serial.println();
            Serial.println("  ---");

            row_values *row = NULL;
            while ((row = query.get_next_row()) != NULL) {
                Serial.print("  ");
                for (int i = 0; i < cols->num_fields; i++) {
                    Serial.print(row->values[i] ? row->values[i] : "NULL");
                    if (i < cols->num_fields - 1) Serial.print(" | ");
                }
                Serial.println();
            }
        }
        Serial.println(">>> SELECT OK <<<");
    } else {
        Serial.println(">>> SELECT FAILED <<<");
        Serial.println("  -> Tabelle existiert nicht oder kein Leserecht");
    }
    Serial.println();

    // Count existing rows
    snprintf(sqlBuffer, sizeof(sqlBuffer),
        "SELECT COUNT(*) FROM %s.%s", db_name, db_table);

    Serial.print("SQL: ");
    Serial.println(sqlBuffer);

    ESP32_MySQL_Query query2 = ESP32_MySQL_Query(&conn);
    if (query2.execute(sqlBuffer)) {
        row_values *row = query2.get_next_row();
        if (row && row->values[0]) {
            Serial.print("Rows in table: ");
            Serial.println(row->values[0]);
        }
    } else {
        Serial.println("  -> COUNT query failed");
    }
    Serial.println();
}

// =============================================================================
// Test INSERT
// =============================================================================
void testInsert() {
    Serial.println("--- Testing INSERT ---");

    char sqlBuffer[250];
    snprintf(sqlBuffer, sizeof(sqlBuffer),
        "INSERT INTO %s.%s (location,hive,temp1,temp2,weight,sound) VALUES ('TEST_ESP32',99,25.5,20.0,10.5,100)",
        db_name, db_table);

    Serial.print("SQL: ");
    Serial.println(sqlBuffer);

    unsigned long startTime = millis();
    ESP32_MySQL_Query query_mem = ESP32_MySQL_Query(&conn);
    bool success = query_mem.execute(sqlBuffer);
    unsigned long elapsed = millis() - startTime;

    Serial.print("Query took ");
    Serial.print(elapsed);
    Serial.println(" ms");

    if (success) {
        Serial.println(">>> INSERT SUCCESSFUL! <<<");

        // Verify: read back the inserted row
        snprintf(sqlBuffer, sizeof(sqlBuffer),
            "SELECT location,hive,temp1,weight FROM %s.%s WHERE location='TEST_ESP32' ORDER BY id DESC LIMIT 1",
            db_name, db_table);

        ESP32_MySQL_Query verifyQuery = ESP32_MySQL_Query(&conn);
        if (verifyQuery.execute(sqlBuffer)) {
            row_values *row = verifyQuery.get_next_row();
            if (row) {
                Serial.print("Verify readback: location=");
                Serial.print(row->values[0] ? row->values[0] : "NULL");
                Serial.print(", hive=");
                Serial.print(row->values[1] ? row->values[1] : "NULL");
                Serial.print(", temp1=");
                Serial.print(row->values[2] ? row->values[2] : "NULL");
                Serial.print(", weight=");
                Serial.println(row->values[3] ? row->values[3] : "NULL");
            }
        }
    } else {
        Serial.println(">>> INSERT FAILED <<<");
        Serial.println("Mögliche Ursachen:");
        Serial.println("  -> Tabelle/Spalten stimmen nicht überein");
        Serial.println("  -> Kein INSERT-Recht für diesen User");
        Serial.println("  -> Datentyp-Fehler");
    }
    Serial.println();
}

// =============================================================================
// Cleanup test data
// =============================================================================
void cleanupTestData() {
    Serial.println("--- Cleanup Test Data ---");

    int result = conn.connectNonBlocking(server_ip, server_port, db_user, db_password);
    if (result == RESULT_FAIL) {
        Serial.println("Connection failed - cannot cleanup.");
        return;
    }

    char sqlBuffer[200];
    snprintf(sqlBuffer, sizeof(sqlBuffer),
        "DELETE FROM %s.%s WHERE location='TEST_ESP32'",
        db_name, db_table);

    Serial.print("SQL: ");
    Serial.println(sqlBuffer);

    ESP32_MySQL_Query query = ESP32_MySQL_Query(&conn);
    if (query.execute(sqlBuffer)) {
        Serial.println(">>> Test data deleted <<<");
    } else {
        Serial.println(">>> DELETE FAILED <<<");
    }

    conn.close();
    Serial.println();
}

// =============================================================================
// Loop - Interactive Menu
// =============================================================================
void loop() {
    Serial.println();
    Serial.println("========== Menu ==========");
    Serial.println("  1 - Full connection test (TCP + MySQL + INSERT)");
    Serial.println("  2 - TCP only test");
    Serial.println("  3 - WiFi status");
    Serial.println("  4 - Cleanup test data (DELETE WHERE location='TEST_ESP32')");
    Serial.println("==========================");
    Serial.println("Send 1-4 (or ENTER for full test):");

    while (!Serial.available()) {
        delay(100);
    }

    String input = Serial.readStringUntil('\n');
    input.trim();
    Serial.println();

    if (input == "2") {
        testTCPConnection();
    } else if (input == "3") {
        Serial.print("WiFi Status: ");
        Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "DISCONNECTED");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("Free heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
    } else if (input == "4") {
        cleanupTestData();
    } else {
        testMySQLConnection();
    }
}
