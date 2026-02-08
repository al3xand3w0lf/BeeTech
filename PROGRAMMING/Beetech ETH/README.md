# IoT-Waage

Eine IoT-fähige Waage mit ESP32, HX711-Verstärker und DS18B20-Temperatursensor.  
Die Waage misst in einem festen Intervall das Gewicht und die Temperatur einer Last, zeigt die Werte auf einem OLED an und sendet sie zusätzlich an eine InfluxDB, wo sie mit Grafana visualisiert werden können. 

---

## ✨ Features

- Periodische Gewichtsmessung in kg (z. B. alle 60 s)  
- Temperaturmessung mit DS18B20  
- Anzeige der aktuellen Messwerte auf einem OLED-Display  
- Versand der Messdaten via WLAN an eine InfluxDB  
- Visualisierung der Messwerte als Zeitreihen in Grafana  
- Projekt inkl. Schaltplan, Code und Dokumentation ist öffentlich auf GitHub vorgesehen  

> **Hinweis:** Die Waage ist fix kalibriert, es gibt keine Tara-Funktion und das Projekt ist für den internen Gebrauch gedacht.
---

## 🧱 Hardware

Verwendete Hauptkomponenten: 

- ESP32 (z. B. Adafruit Feather ESP32)  
- HX711 – Verstärker für Wägezelle(n)  
- Wägezelle(n)  
- DS18B20 Temperatursensor  
- OLED-Display (I²C)  
- Stromversorgung (z. B. USB, je nach Board)  
- Mechanischer Aufbau (Teller, Gehäuse etc.)  

Details zum mechanischen Aufbau, CAD-Modellen und Stückliste findest du in der ausführlichen Projektdokumentation.

---

## 🧰 Software & Tools

- **Firmware:** Arduino-Ökosystem (Arduino IDE)  
- **Datenbank:** InfluxDB  
- **Visualisierung:** Grafana  
- **Dokumentation:** Markdown (`.md`)  
- **Versionsverwaltung:** Git & GitHub  

Die genaue Projektstruktur, Bibliotheken und Abhängigkeiten werden in der Projektdokumentation und im Code beschrieben.

---

## 🏗️ Systemübersicht

Grobe Datenfluss-Übersicht:
1. Wägezelle → HX711 → ESP32  
2. DS18B20 → ESP32  
3. ESP32 bereitet die Messwerte auf  
4. Anzeige der aktuellen Werte auf dem OLED-Display  
5. Periodischer Versand der Werte via WLAN an InfluxDB  
6. Darstellung der Daten in Grafana als Zeitreihen-Dashboard  

Details zur IoT-Architektur (Topics, Messintervalle, Datenstruktur etc.) werden in der Dokumentation beschrieben.

---

## 🚀 Getting Started (Kurz)

### Voraussetzungen

- Installierte Arduino IDE oder ein anderes ESP32-kompatibles Tooling  
- InfluxDB-Instanz (lokal oder im Netzwerk)  
- Grafana-Installation mit Zugriff auf die InfluxDB  
- Klon dieses Repositories

### Grundlegende Schritte



