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
const char* WIFI_SSID     = "ETHIGPWIFI";
const char* WIFI_PASSWORD = "ETHIGPWIFI";

// MySQL Server
IPAddress server_ip(91, 204, 46, 146);
uint16_t  server_port = 3306;
char      db_user[]     = "k139859_n";
char      db_password[] = "22222";
char      db_name[]     = "k139859_n";
char      db_table[]    = "BeetechData01";

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
// Test MySQL Connection
// =============================================================================
void testMySQLConnection() {
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

    Serial.println("Attempting connection...");

    // Try connectNonBlocking first
    int result = conn.connectNonBlocking(server_ip, server_port, db_user, db_password);

    if (result != RESULT_FAIL) {
        Serial.println(">>> CONNECTION SUCCESSFUL! <<<");
        Serial.println();

        // Try a test INSERT
        testInsert();

        conn.close();
        Serial.println("Connection closed.");
    } else {
        Serial.println(">>> CONNECTION FAILED <<<");
        Serial.println();
        Serial.println("Mögliche Ursachen:");
        Serial.println("1. MySQL Server nicht erreichbar (Firewall?)");
        Serial.println("2. Falscher User/Password");
        Serial.println("3. MySQL 8+ mit caching_sha2_password");
        Serial.println();
        Serial.println("Lösung für MySQL 8+:");
        Serial.println("  ALTER USER 'k139859_n'@'%' IDENTIFIED WITH mysql_native_password BY '22222';");
        Serial.println("  FLUSH PRIVILEGES;");
    }
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

    Serial.println("SQL:");
    Serial.println(sqlBuffer);
    Serial.println();

    ESP32_MySQL_Query query_mem = ESP32_MySQL_Query(&conn);

    if (query_mem.execute(sqlBuffer)) {
        Serial.println(">>> INSERT SUCCESSFUL! <<<");
        Serial.println("Check your database for the test entry.");
    } else {
        Serial.println(">>> INSERT FAILED <<<");
    }
    Serial.println();
}

// =============================================================================
// Loop
// =============================================================================
void loop() {
    Serial.println();
    Serial.println("Press ENTER to retry connection test...");

    while (!Serial.available()) {
        delay(100);
    }
    while (Serial.available()) {
        Serial.read();
    }

    Serial.println();
    testMySQLConnection();
}
