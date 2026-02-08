
# Projektarbeit – Elektronische IoT-Waage mit Temperatur-Logging und OLED-Display

**Datum:** 2025-11-30  
**Erstellt von:** Alexander Wolf – ETH Zürich, IGP

---

## Projektbeschreibung

Es soll eine elektronische IoT-Waage gebaut werden, welche periodisch (alle 60 Sekunden) das Gewicht der Last in kg misst. Zusätzlich soll die Temperatur der Last gemessen werden. Diese Messwerte sollen auf einem OLED-Display dargestellt und periodisch via WLAN in eine InfluxDB-Datenbank gespeichert werden. Die Werte können dann via Grafana als Zeitreihe visualisiert werden.  
Alle Projektdetails, Schaltpläne, Code und Dokumentation sollen öffentlich zur freien Verfügung auf GitHub veröffentlicht werden.

---

## Grundsätzlicher Aufbau des Systems

### 1. Waage
- Die Waage wird aus **4 Wägezellen** aufgebaut, welche zwischen zwei **30 × 30 cm Aluminiumplatten** liegen.
- In jeder Ecke wird je eine Wägezelle angebracht.
- Für jede Wägezelle muss ein **Befestigungsrahmen in SolidWorks** entworfen und per **3D-Druck** gefertigt werden.
- Die 4 Wägezellen werden an einem **HX711 24-Bit ADC** in einer Wheatstone-Messbrücke angeschlossen.
- Der HX711 wird über ein **2-Wire-Bitbang-Protokoll** vom **ESP32-WROOM (Adafruit Feather V2)** ausgelesen.

**Spezifikation:**  
Messbereich: **0–100 kg**

---

### 2. Temperaturmessung
- Die Temperatur wird mit einem **DS18B20-Sensor** gemessen.
- Kommunikation via **One-Wire-Protokoll**.
- Ausgelesen durch den ESP32-WROOM.

---

### 3. OLED-Display
- Ein OLED-Display **SSD1306, 128×64 px** zeigt die aktuellen Messwerte an.
- Angesteuert via **I²C**.

---

### 4. Mikrocontroller & Datenübertragung
- ESP32 liest periodisch alle Sensorwerte aus.
- Anzeige auf dem OLED.
- Übertragung zur **InfluxDB** via WLAN.
- Stromversorgung über **USB 5V**.

---

## Ziele

- Sensoren in Betrieb nehmen  
- ESP32-Programmierung (Arduino)  
- I²C, One-Wire und SPI verstehen und anwenden  
- **Professionelle Schaltpläne in KiCad**  
- **3D-CAD in SolidWorks**  
- **Git & GitHub-Versionierung**  
- **Technische Dokumentation**  
- Messungen mit Oszilloskop & Logic Analyzer  
- WLAN & InfluxDB-Datenübertragung

---

## Aufgabenbeschreibung

### 1. Projektzeitraum
- **Start:** Dezember 2025  
- **Abgabe:** 31. Januar 2026

---

### 2. Recherche und Konzept
- Recherche zu HX711, DS18B20, ESP32, OLED SSD1306
- Verständnis des Zusammenspiels aller Komponenten
- Dokumentation:
  - Hauptdokumentation in Word
  - Recherchehandnotizen **handschriftlich**

---

### 3. Projektplanung
- Word-Dokument mit **Inhaltsverzeichnis**, das später den Aufbau der Dokumentation darstellt
- Erstellung einer **Projekt-Roadmap**, Zeitplanung & Meilensteine bis zur Abgabe

---

### 4. CAD-Design (SolidWorks)
- Erstellung der Befestigungsrahmen
- 3D-Druck der Teile
- Dokumentation mit Screenshots & technischen Zeichnungen

---

### 5. Schaltplan (KiCad)
- Zeichnen des vollständigen Schaltplans
- Beschriften aller Komponenten & Anschlüsse
- Integration in die Dokumentation

---

### 6. Stückliste (BOM)
- Erstellen einer Excel-Datei inklusive:
  - Bauteilbezeichnung
  - Anzahl
  - Händler
  - Bestellnummer
  - Link
  - Preis
- Einfügen in Dokumentation

---

### 7. Hardwareaufbau
- Aufbau auf Steckbrett oder Platine
- Professionelle Ausführung  
- Löten falls nötig

---

### 8. Softwareentwicklung

#### a) Versionskontrolle
- Zwingend Git verwenden
- GitHub-Repository erstellen
- Sinnvolle Commit-Messages

#### b) Entwicklungsschritte

##### Schritt 1: Basis-Funktionalität (ohne WLAN)
- Waage + Temperatursensor + OLED aufbauen
- Sensorwerte alle 15 Sekunden auslesen
- Anzeige auf OLED
- Nutzung von Arduino-Bibliotheken

##### Schritt 2: Protokollanalyse
- I²C-Kommunikation mit Oszilloskop aufnehmen
- Dekodieren mit Logic Analyzer
- Startsequenz dokumentieren (Screenshots)
- Signale erklären

##### Schritt 3: Kalibrierung
- Software muss Kalibrierfunktion haben (separater Kalibrier-Code erlaubt)
- Kalibrierung mit **mind. 3 Gewichten**
- Dokumentation von Prozess & Faktoren
- Überprüfung der Genauigkeit

##### Schritt 4: WLAN-Integration
- WLAN-Code integrieren
- Verbindung zur InfluxDB
- Messwerte alle 60 Sekunden senden

---

## Dokumentation (Mindestanforderung)

1. Einleitung & Projektübersicht  
2. Zielsetzung  
3. Systemarchitektur & Blockschaltbild  
4. Komponentenbeschreibung  
5. CAD-Design (SolidWorks)  
6. Schaltplan (KiCad)  
7. Stückliste (BOM)  
8. Softwareentwicklung & Code-Beschreibung  
9. Kalibrierung  
10. Messungen & Tests (Oszilloskop, Logic Analyzer)  
11. Inbetriebnahme & Resultate  
12. Probleme & Lösungen  
13. Fazit & Ausblick  

---

## GitHub-Veröffentlichung

- Vollständiges Projekt auf GitHub veröffentlichen:
  - Code
  - KiCad-Files
  - CAD-Dateien (STEP/STL)
  - Dokumentation (PDF)
  - BOM
- README.md aussagekräftig erstellen

---

## Bewertungskriterien

| Kriterium | Gewicht |
|----------|---------|
| Funktionalität | 30% |
| Dokumentation | 25% |
| Code-Qualität | 15% |
| Schaltplan/CAD | 15% |
| Eigenständigkeit | 10% |
| GitHub/Versionskontrolle | 5% |

---

## Verfügbare Ressourcen
- Arduino & Adafruit Bibliotheken  
- SolidWorks Lizenz  
- KiCad  
- Oszilloskop & Logic Analyzer  
- 3D-Drucker  
- InfluxDB Zugangsdaten  

---

**Viel Erfolg bei der Umsetzung!**
