/*

ESP32S3 DEV Module 
https://de.aliexpress.com/item/1005009401501929.html?gatewayAdapt=glo2deu
D1 ESP32-S3 WiFi + Bluetooth 16 MB Flash für UNO D1 R3 Board Modul CH340 N16R8/N8R2 Für ESP-32 entwicklung Bord Drahtlose Modul


LCD
https://de.aliexpress.com/item/1005009572573845.html?gatewayAdapt=glo2deu
 1,37 Zoll 1,37 Zoll Matrix LCD-Modul SPI-Schnittstelle AiP31567 Treiber Gleiche wie ST7567
 

*/



#include <U8g2lib.h>
#include <SPI.h>

#define BL_PIN 14

// ST7567 128x64 SPI
U8G2_ST7567_ENH_DG128064I_F_4W_HW_SPI u8g2(
  U8G2_R0,
  /* cs=*/ 19,
  /* dc=*/ 3,
  /* reset=*/ 20
);

void setup() {

  // Backlight EIN
  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN, LOW);

  // SPI starten (SCK=12, MOSI=11)
  SPI.begin(12, -1, 11, 19);

  u8g2.begin();

  // Kontrast fest setzen
  u8g2.setContrast(200);

  // Anzeige
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_logisoso18_tr);
  u8g2.drawStr(20, 20, "Hello");
  u8g2.drawStr(10, 60, "Bee-Team");
  u8g2.sendBuffer();
}

void loop() {
  // nichts nötig
}