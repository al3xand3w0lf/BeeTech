// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <CAN.h>

/*
// EVERY
#define CAN_CS 5
#define CAN_INTERRUPT 4
*/

// MKR 1400
#define CAN_CS 13
#define CAN_INTERRUPT 7


void setup() {
  Serial.begin(9600);
  while (!Serial);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("CAN Sender");
  
  // start the CAN bus at 500 kbps
  CAN.setPins(CAN_CS, CAN_INTERRUPT);
  CAN.setSPIFrequency(4E6); // necessary foor MKR, lower SPI frequenzy then standard by library
  
  // attachInterrupt (digitalPinToInterrupt (BUTTON1), ISR_button1_Pressed, FALLING);  // attach interrupt handler 
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while(1){
       digitalWrite(LED_BUILTIN, HIGH);
       delay(300);
       digitalWrite(LED_BUILTIN, LOW);
       delay(300);
      }
    }  
}

void loop() {
  // send packet: id is 11 bits, packet can contain up to 8 bytes of data
  Serial.print("Sending packet ... ");

  CAN.beginPacket(0x12);
  CAN.write('h');
  CAN.write('e');
  CAN.write('l');
  CAN.write('l');
  CAN.write('o');
  CAN.endPacket();

  Serial.println("done");

  delay(1000);

  // send extended packet: id is 29 bits, packet can contain up to 8 bytes of data
  Serial.print("Sending extended packet ... ");

  CAN.beginExtendedPacket(0xabcdef);
  CAN.write('w');
  CAN.write('o');
  CAN.write('r');
  CAN.write('l');
  CAN.write('d');
  CAN.endPacket();

  Serial.println("done");

  delay(1000);
}
