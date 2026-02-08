# Beetech V2 - IoT Bienenstock-Waage

ESP32-basiertes IoT-System zur Überwachung von Bienenstöcken mit direkter MySQL-Datenbankverbindung.

## Hardware

### Hauptkomponenten
| Komponente | Modell | Beschreibung |
|------------|--------|--------------|
| Mikrocontroller | Adafruit ESP32 Feather V2 | 8MB Flash, 2MB PSRAM, WiFi |
| ADC | HX711 24-bit | Wägezellen-Verstärker |
| Wägezellen | 4x 50kg | Wheatstone-Brücke Konfiguration |
| Temperatursensor | DS18B20 | OneWire Digital |
| Display | SH1106 OLED | 128x64 Pixel, I2C |
| Speicher | SD-Karte | Konfigurationsdatei |

### Pin-Belegung ESP32 Feather V2

```
+---------------------------+
|     ESP32 Feather V2      |
+---------------------------+
| GPIO25 (A1) --> HX711 DT  |
| GPIO26 (A0) --> HX711 SCK |
| GPIO39 (A3) --> DS18B20   |
| GPIO4  (A5) --> SD CS     |
| I2C SDA    --> OLED SDA   |
| I2C SCL    --> OLED SCL   |
| 3.3V       --> VCC        |
| GND        --> GND        |
+---------------------------+
```

### Verkabelung

#### HX711 Wägezellen-Verstärker
```
HX711          ESP32
------         -----
VCC    ----->  3.3V
GND    ----->  GND
DT     ----->  GPIO25 (A1)
SCK    ----->  GPIO26 (A0)
```

#### DS18B20 Temperatursensor
```
DS18B20        ESP32
-------        -----
VCC (rot)      ----->  3.3V
GND (schwarz)  ----->  GND
DATA (gelb)    ----->  GPIO39 (A3)

WICHTIG: 4.7kΩ Pullup-Widerstand zwischen DATA und VCC!
```

#### SH1106 OLED Display
```
OLED           ESP32
----           -----
VCC    ----->  3.3V
GND    ----->  GND
SDA    ----->  SDA (I2C)
SCL    ----->  SCL (I2C)

I2C Adresse: 0x3C
```

---

## Software

### Benötigte Arduino Libraries

| Library | Verwendung | Installation |
|---------|------------|--------------|
| ESP32_MySQL | MySQL Direktverbindung | Library Manager: "ESP32_MySQL" by Syafiqlim |
| HX711 | Wägezellen-ADC | Library Manager: "HX711 Arduino Library" |
| OneWire | OneWire Protokoll | Library Manager: "OneWire" |
| DallasTemperature | DS18B20 Sensor | Library Manager: "DallasTemperature" |
| U8g2 | OLED Display | Library Manager: "U8g2" |
| SD | SD-Karte | (eingebaut) |
| WiFi | WiFi Verbindung | (eingebaut für ESP32) |

### Arduino IDE Einstellungen

```
Board:        Adafruit ESP32 Feather V2
Upload Speed: 921600
Flash Mode:   QIO
Partition:    Default 4MB with spiffs
```

---

## Projektstruktur

```
Beetech V2/
├── BeetechV2_Scale/           # Hauptfirmware
│   ├── BeetechV2_Scale.ino    # Main, Setup, Loop
│   ├── Database.ino           # WiFi & MySQL
│   ├── DS18B20_Temperature.ino# Temperatursensor
│   ├── HX711_Scale.ino        # Waage/Wägezellen
│   ├── SdCard.ino             # SD-Karte & Config
│   ├── SH1106_OLED.ino        # Display
│   └── Terminal.ino           # Serial Befehle
│
├── Testing/                   # Test-Skripte
│   ├── Database_Test/         # MySQL Verbindungstest
│   ├── Scale_Test/            # Waagen-Kalibrierung
│   └── Temperature_Test/      # DS18B20 Test
│
├── config.txt                 # Beispiel-Konfiguration
└── README.md                  # Diese Dokumentation
```

---

## Konfiguration

### config.txt auf SD-Karte

Die Konfigurationsdatei muss im Root-Verzeichnis der SD-Karte liegen.

```ini
# ===========================================
# Station Identifikation
# ===========================================
station_number=0
station_name=Beehive_Lab

# ===========================================
# Waagen-Kalibrierung
# ===========================================
calibration_factor=-24200
scale_offset=0.0

# ===========================================
# Timing (in Sekunden)
# ===========================================
dataPoll_interval=10
upload_interval=30

# ===========================================
# WiFi Zugangsdaten
# ===========================================
wifi_ssid=MeinWiFi
wifi_password=MeinPasswort

# ===========================================
# MySQL Datenbank (Direktverbindung)
# ===========================================
db_server=91.204.46.146
db_port=3306
db_user=k139859_n
db_password=22222
db_name=k139859_n
db_table=BeetechData01

# ===========================================
# Energieverwaltung
# ===========================================
deep_sleep_enabled=0
```

### Konfigurationsparameter

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `station_number` | int | Eindeutige Stationsnummer |
| `station_name` | string | Name der Station (max 50 Zeichen) |
| `calibration_factor` | long | HX711 Kalibrierungsfaktor |
| `scale_offset` | float | Waagen-Offset |
| `dataPoll_interval` | int | Messintervall in Sekunden |
| `upload_interval` | int | Upload-Intervall in Sekunden |
| `wifi_ssid` | string | WiFi Netzwerkname |
| `wifi_password` | string | WiFi Passwort |
| `db_server` | string | MySQL Server IP-Adresse |
| `db_port` | int | MySQL Port (Standard: 3306) |
| `db_user` | string | MySQL Benutzername |
| `db_password` | string | MySQL Passwort |
| `db_name` | string | Datenbankname |
| `db_table` | string | Tabellenname |
| `deep_sleep_enabled` | int | 0=aus, 1=ein |

---

## Datenbank

### MySQL Tabellenstruktur

```sql
CREATE TABLE BeetechData01 (
    id INT AUTO_INCREMENT PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    location VARCHAR(100),
    hive INT,
    temp1 FLOAT,
    temp2 FLOAT,
    weight FLOAT,
    sound INT
);
```

### SQL INSERT Format

```sql
INSERT INTO k139859_n.BeetechData01
    (location, hive, temp1, temp2, weight, sound)
VALUES
    ('Beehive_Lab', 0, 25.5, 0.0, 45.3, 0);
```

---

## Terminal-Befehle

Über Serial Monitor (115200 baud) verfügbare Befehle:

| Befehl | Beschreibung |
|--------|--------------|
| `help` | Zeigt alle Befehle |
| `status` | Zeigt Systemstatus |
| `read` | Liest aktuelle Sensorwerte |
| `tare` | Tariert die Waage (Nullpunkt) |
| `cal XX` | Kalibriert mit bekanntem Gewicht XX kg |
| `upload` | Erzwingt Daten-Upload |
| `wifi` | Zeigt WiFi-Status |
| `db` | Zeigt Datenbank-Status |
| `reset` | Startet System neu |

---

## Test-Skripte

### 1. Database_Test

**Pfad:** `Testing/Database_Test/Database_Test.ino`

Testet die MySQL-Verbindung isoliert.

**Funktionen:**
- WiFi-Verbindungstest
- MySQL-Verbindungstest
- Test-INSERT ausführen
- Detaillierte Fehlermeldungen

**Verwendung:**
1. Sketch öffnen
2. WiFi und DB Credentials anpassen
3. Upload und Serial Monitor öffnen
4. ENTER drücken zum Wiederholen

### 2. Scale_Test

**Pfad:** `Testing/Scale_Test/Scale_Test.ino`

Interaktives Kalibrierungs-Tool für die Waage.

**Befehle:**
| Befehl | Beschreibung |
|--------|--------------|
| `r` | Gewicht lesen |
| `t` | Tarieren |
| `c XXX` | Kalibrierungsfaktor setzen |
| `+` | Kalibrierung +100 |
| `-` | Kalibrierung -100 |
| `raw` | Rohwert lesen |
| `cal X` | Mit X kg kalibrieren |
| `h` | Hilfe anzeigen |

**Kalibrierungsanleitung:**
1. Waage ohne Last starten
2. Befehl `t` zum Tarieren
3. Bekanntes Gewicht auflegen (z.B. 5 kg)
4. Befehl `cal 5` eingeben
5. Neuen Kalibrierungsfaktor notieren
6. In `config.txt` speichern

### 3. Temperature_Test

**Pfad:** `Testing/Temperature_Test/Temperature_Test.ino`

Testet und scannt DS18B20 Sensoren.

**Befehle:**
| Befehl | Beschreibung |
|--------|--------------|
| `r` | Temperaturen lesen |
| `s` | Sensoren neu scannen |
| `raw` | Raw OneWire Scan |
| `h` | Hilfe anzeigen |

**Troubleshooting wenn kein Sensor gefunden:**
1. Verkabelung prüfen
2. 4.7kΩ Pullup-Widerstand vorhanden?
3. Korrekter GPIO-Pin (39)?
4. Anderen Sensor versuchen

---

## Fehlerbehebung

### MySQL: "Error reading auth packets"

**Ursache:** MySQL 8+ verwendet standardmäßig `caching_sha2_password` statt `mysql_native_password`.

**Lösung auf MySQL Server:**
```sql
ALTER USER 'k139859_n'@'%'
    IDENTIFIED WITH mysql_native_password BY '22222';
FLUSH PRIVILEGES;
```

### MySQL: Verbindung fehlgeschlagen

1. **Firewall prüfen:** Port 3306 muss offen sein
2. **Remote-Zugriff:** MySQL muss Remote-Verbindungen erlauben
   ```sql
   -- In MySQL config (my.cnf):
   bind-address = 0.0.0.0
   ```
3. **User-Berechtigungen:**
   ```sql
   GRANT ALL ON k139859_n.* TO 'k139859_n'@'%';
   FLUSH PRIVILEGES;
   ```

### HX711: Keine Messwerte

1. Verkabelung prüfen (DT → GPIO25, SCK → GPIO26)
2. VCC an 3.3V (nicht 5V!)
3. Wägezellen korrekt angeschlossen?
4. Scale_Test Sketch verwenden

### DS18B20: Sensor nicht gefunden

1. **4.7kΩ Pullup-Widerstand** zwischen DATA und VCC
2. Verkabelung prüfen
3. GPIO39 korrekt?
4. Temperature_Test Sketch verwenden

### SD-Karte: Fehler beim Laden

1. SD-Karte als FAT32 formatieren
2. `config.txt` im Root-Verzeichnis
3. Datei mit Texteditor erstellen (UTF-8, Unix-Zeilenenden)

### WiFi: Verbindung fehlgeschlagen

1. SSID und Passwort prüfen
2. 2.4 GHz Netzwerk? (ESP32 unterstützt kein 5 GHz)
3. Signalstärke prüfen

---

## Vergleich V1 vs V2

| Feature | Beetech V1 | Beetech V2 |
|---------|-----------|-----------|
| MCU | Arduino MKR GSM 1400 | ESP32 Feather V2 |
| Konnektivität | GSM/GPRS | WiFi |
| Netzwerk | CAN-Bus (Master/Slave) | Standalone |
| Display | ILI9341 TFT | SH1106 OLED |
| MySQL Library | MySQL_Connector_Arduino | ESP32_MySQL |
| Konfiguration | SD-Karte | SD-Karte |

---

## Changelog

### Version 2.0 (2024)
- Neue Hardware-Plattform: ESP32 Feather V2
- WiFi statt GSM
- Direkte MySQL-Verbindung mit ESP32_MySQL Library
- SH1106 OLED Display
- Vereinfachte Einzelstation (kein CAN-Bus)
- Interaktive Test-Skripte

---

## Lizenz

Dieses Projekt ist Teil des BeeTech IoT Bienenstock-Monitoring-Systems.

## Kontakt

Bei Fragen oder Problemen: GitHub Issues verwenden.
