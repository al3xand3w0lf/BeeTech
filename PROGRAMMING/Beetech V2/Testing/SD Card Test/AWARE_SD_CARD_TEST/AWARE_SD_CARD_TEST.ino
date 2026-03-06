/*
  ESP32-WROOM-32E-N16
  SD card read/write
*/

#include <SPI.h>
#include "SdFat.h"

// SD-Karten-Konfiguration
#define SD_FAT_TYPE 3
#define SD_CS_PIN 5  // IO5 (Pin 29)

// SPI-Pins laut Schaltplan 
#define SPI_MOSI_PIN 19  // IO19 (Pin 31)
#define SPI_MISO_PIN 18  // IO18 (Pin 30)
#define SPI_SCK_PIN 2    // IO2 (Pin 24)

SdFat SD;
File myFile;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Warte auf serielle Verbindung
  }  

  Serial.print("Initialisiere SD-Karte...");
  
  // Konfiguriere die SPI-Pins vor der Initialisierung
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
  
  // Initialisiere die SD-Karte
  if (!SD.begin(SD_CS_PIN, SD_SCK_MHZ(4))) {
    Serial.println("Initialisierung fehlgeschlagen!");
    return;
  }
  Serial.println("Initialisierung erfolgreich.");

  // Öffne die Datei
  myFile = SD.open("test.txt", FILE_WRITE);

  // Wenn die Datei erfolgreich geöffnet wurde, schreibe hinein
  if (myFile) {
    Serial.print("Schreibe in test.txt...");
    myFile.println("Test 1, 2, 3.");
    myFile.println("Hallo SD-Karte!");
    // Schließe die Datei
    myFile.close();
    Serial.println("fertig.");
  } else {
    // Wenn die Datei nicht geöffnet werden konnte, gib einen Fehler aus
    Serial.println("Fehler beim Öffnen von test.txt");
  }

  // Öffne die Datei erneut zum Lesen
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    // Lese die Datei bis nichts mehr vorhanden ist
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // Schließe die Datei
    myFile.close();
  } else {
    // Wenn die Datei nicht geöffnet werden konnte, gib einen Fehler aus
    Serial.println("Fehler beim Öffnen von test.txt");
  }
}

void loop() {
  // Nach dem Setup passiert nichts
}