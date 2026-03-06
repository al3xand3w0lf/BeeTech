#include <SPI.h>
#include "SdFat.h"

#define SD_FAT_TYPE 3
#define SD_CS_PIN A0

// SD Card-Objekt und Datei-Objekte
SdFat SD;
File root;
File entry;

// Buffer für die SD-Karte
uint8_t *myBuffer;
#define sdWriteSize 512

void setup() {
  // Initialisiere seriellen Port
  Serial.begin(115200);
  
  // Warte auf die Initialisierung des seriellen Ports
  delay(3000);
  
  Serial.println("SD-Karten Datei-Listing startet...");
  
  // Initialisiere SD-Karte
  init_sdCard();
  
  // Liste alle Dateien auf der SD-Karte auf
  listFiles();
}

void loop() {
  // Nichts zu tun im Loop
}

void init_sdCard() {
  // Initialisiere SD-Karte
  myBuffer = new uint8_t[sdWriteSize]; // Erzeuge Buffer für Daten während des Schreibens auf SD-Karte
  
  Serial.print("Initialisiere SD-Karte... "); 
  if (!SD.begin(SD_CS_PIN, SD_SCK_MHZ(4))) {
    Serial.println("Initialisierung fehlgeschlagen!");    
  }
  else {
    Serial.println("Initialisierung erfolgreich.");
  }
}

void listFiles() {
  // Öffne das Root-Verzeichnis
  root = SD.open("/");
  
  if (!root) {
    Serial.println("Fehler beim Öffnen des Root-Verzeichnisses!");
    return;
  }
  
  Serial.println("Dateien auf der SD-Karte:");
  Serial.println("------------------------");
  Serial.println("Name                             | Größe (Bytes)");
  Serial.println("----------------------------------|---------------");
  
  // Durchlaufe alle Dateien und Verzeichnisse
  while (true) {
    entry = root.openNextFile();
    
    // Keine weiteren Dateien
    if (!entry) {
      break;
    }
    
    // Ausgabe des Dateinamens (mit Padding für bessere Lesbarkeit)
    char fileName[256];
    entry.getName(fileName, 256);
    Serial.print(fileName);
    
    // Berechne Padding für schöne Ausgabe
    int nameLength = strlen(fileName);
    for (int i = nameLength; i < 33; i++) {
      Serial.print(" ");
    }
    
    Serial.print("| ");
    
    // Prüfe, ob es ein Verzeichnis ist
    if (entry.isDirectory()) {
      Serial.println("<DIR>");
    } else {
      // Ausgabe der Dateigröße
      Serial.println(entry.size());
    }
    
    // Schließe die Datei
    entry.close();
  }
  
  // Schließe das Root-Verzeichnis
  root.close();
  
  Serial.println("------------------------");
  Serial.println("Datei-Listing abgeschlossen.");
}