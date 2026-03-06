// SPDX-FileCopyrightText: 2023 Liz Clark for Adafruit Industries
//
// SPDX-License-Identifier: MIT
/*
  SD card read/write

 This example shows how to read and write data to and from an SD card file
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13


ESP32S3 DEV Module 
https://de.aliexpress.com/item/1005009401501929.html?gatewayAdapt=glo2deu
D1 ESP32-S3 WiFi + Bluetooth 16 MB Flash für UNO D1 R3 Board Modul CH340 N16R8/N8R2 Für ESP-32 entwicklung Bord Drahtlose Modul



 */

#include <SPI.h>
//#include <SD.h>
#include "SdFat.h"
SdFat SD;

#define SD_FAT_TYPE 3

// SPI Pins laut Pin Mapping (neues Board)
#define SD_SCK_PIN  12
#define SD_MISO_PIN 13
#define SD_MOSI_PIN 11
#define SD_CS_PIN   10

FsFile myFile;
SPIClass mySPI(FSPI);
SdSpiConfig config(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(4), &mySPI);

// Ohne explizite Pin-Konfiguration (nur wenn Pins den Board-Defaults entsprechen):
//SdSpiConfig config(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(4), &SPI);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  mySPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  Serial.print("Initializing SD card...");

  while (!SD.begin(config)) {
    Serial.println("initialization failed! Retrying...");
    delay(1000);
  }
  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    myFile.println("hello sd card!");
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