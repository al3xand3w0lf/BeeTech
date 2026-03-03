/*

ESP32S3 DEV Module 
https://de.aliexpress.com/item/1005009401501929.html?gatewayAdapt=glo2deu
D1 ESP32-S3 WiFi + Bluetooth 16 MB Flash für UNO D1 R3 Board Modul CH340 N16R8/N8R2 Für ESP-32 entwicklung Bord Drahtlose Modul



*/


#include <SPI.h>
#include "SdFat.h"

#define SD_CS_PIN 10
#define SD_CD_PIN 46

#define SD_CARD_MISSING 0

SdFat SD;
FsFile myFile;
SdSpiConfig config(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(4), &SPI);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) { yield(); delay(10); }     // wait till serial port is opened

  pinMode(SD_CD_PIN, INPUT_PULLUP);
  delay(100);  
/*
  while(SD_CARD_MISSING == digitalRead(SD_CD_PIN)){
      Serial.println("No SD Card Found");
      delay(1000);  
  }
  delay(500);
*/
  Serial.print("Initializing SD card...");

  // Retry mechanism for SD card initialization
  while (!SD.begin(config)) {
    Serial.println("initialization failed! Retrying...");
    delay(1000); // Wait for a second before retrying
  }
  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    myFile.println("hello world!");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

void loop() {
  // nothing happens after setup
}
