// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <CAN.h>

/*
// EVERY
#define CAN_CS 5
#define CAN_INTERRUPT 4
*/

// MKR 1400

#define CAN_CS 11
#define CAN_INTERRUPT 12





void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("CAN Receiver");

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
   Serial.println("Started!");   
}

void loop() {
  // try to parse packet
  int packetSize = CAN.parsePacket();

  if (packetSize) {
    // received a packet
    Serial.print("Received ");

    if (CAN.packetExtended()) {
      Serial.print("extended ");
    }

    if (CAN.packetRtr()) {
      // Remote transmission request, packet contains no data
      Serial.print("RTR ");
    }

    Serial.print("packet with id 0x");
    Serial.print(CAN.packetId(), HEX);

    if (CAN.packetRtr()) {
      Serial.print(" and requested length ");
      Serial.println(CAN.packetDlc());
    } else {
      Serial.print(" and length ");
      Serial.println(packetSize);

      // only print packet data for non-RTR packets
      while (CAN.available()) {
        //Serial.print((char)CAN.read());
        //Serial.print(CAN.read(), HEX);
        Serial.print(CAN.read(), DEC);
      }
      Serial.println();
    }

    Serial.println();
  }
}
