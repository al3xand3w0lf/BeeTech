// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <CAN.h>


// EVERY
#define CAN_CS 5
#define CAN_INTERRUPT 4

/*
// MKR 1400
#define CAN_CS 13
#define CAN_INTERRUPT 7 // funktioniert !!! PIN  D7 (INT5)
*/

const int BUTTON1 =  16; //20;  // interrupt ?
const int BUTTON2 =  17; //21;  // interrupt ?


// CAN Bus struct
typedef struct {
  int     messageId;
  uint8_t receiveBuffer[5];
  float   value_float;
} canMessage_t;

canMessage_t CAN_MESSAGE;



// Interrupt Service Routine (ISR)
void ISR_button1_Pressed ()
{ 
  Serial.println("Button 1!");

}  

// Interrupt Service Routine (ISR)
void ISR_button2_Pressed ()
{
  Serial.println("Button 2!");
} 


void setup() {
  Serial.begin(9600);
  while (!Serial);
  //delay(5000);

  Serial.println("CAN Receiver");
  /*
  CAN_MESSAGE.messageId = 0;
  CAN_MESSAGE.receiveBuffer[0] = 0;
  CAN_MESSAGE.value_float = 0;
  */

  // start the CAN bus at 500 kbps
  CAN.setPins(CAN_CS, CAN_INTERRUPT);
  CAN.setSPIFrequency(4E6); // necessary foor MKR, lower SPI frequenzy then standard by library
  //CAN.setClockFrequency(8E6);
  CAN.setClockFrequency(16E6);
  
  // attachInterrupt (digitalPinToInterrupt (BUTTON1), ISR_button1_Pressed, FALLING);  // attach interrupt handler 
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while(1){
       digitalWrite(LED_BUILTIN, HIGH);
       delay(150);
       digitalWrite(LED_BUILTIN, LOW);
       delay(150);
      }
    }
  Serial.println("CAN initialized"); 

  attachInterrupt (digitalPinToInterrupt (BUTTON1), ISR_button1_Pressed, FALLING);  // attach interrupt handler
  attachInterrupt (digitalPinToInterrupt (BUTTON2), ISR_button2_Pressed, FALLING);  // attach interrupt handler
      
  CAN.onReceive(onReceive);  
 
  Serial.println("System Started!");   
}

void loop() {
  
  Serial.println("loop..."); 
  
  delay(1000);
  
}



float S4ByteToFloat32Bit(unsigned char * ptr_input){
 int k = 0, i = 0;
 float value_float = 0.0;
 unsigned char *p1 = (unsigned char *)&value_float;

 p1 = (unsigned char *)&value_float;

 i = sizeof(value_float);

 for (k = 0; k < i; k++){
      *p1 = *ptr_input;
      p1++;
      ptr_input++;
      }

 return value_float;
}



void onReceive(int packetSize) {

  uint8_t i = 0;
  Serial.println();
  Serial.print("Received ");
  
  Serial.print("packet with id 0x");
  CAN_MESSAGE.messageId = CAN.packetId();
  Serial.print(CAN_MESSAGE.messageId, HEX);

  Serial.print(" and length ");
  Serial.println(packetSize);

  // only print packet data for non-RTR packets
  i = 0;
  while (CAN.available()) {

    CAN_MESSAGE.receiveBuffer[i] = CAN.read();
    Serial.print(CAN_MESSAGE.receiveBuffer[i], HEX);
    i++;
    Serial.print(" ");
  }
  Serial.println();
  

  CAN_MESSAGE.value_float = S4ByteToFloat32Bit(&CAN_MESSAGE.receiveBuffer[0]);
  Serial.print(" its float value: ");
  Serial.println(CAN_MESSAGE.value_float);
  Serial.println();
}
