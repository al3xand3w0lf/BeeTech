// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <CAN.h>


// EVERY
#define CAN_CS 5
#define CAN_INTERRUPT 4


// MKR 1400

/*
#define CAN_CS 11
#define CAN_INTERRUPT 7
*/

void setup() {
  Serial.begin(9600);
  while (!Serial);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("CAN Sender");
  
  // start the CAN bus at 500 kbps
  CAN.setPins(CAN_CS, CAN_INTERRUPT);
  //CAN.setSPIFrequency(4E6);
  
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
  delay(5000);

  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  //messageID = 0x0101;
  sendSensorData(  0x0101, 22.13 );
  sendSensorData(  0x0201, 23.13 );
  sendSensorData(  0x0301, 80.1 );
  sendSensorData(  0x0401, 100.2 );
  
}




void float32BitTo4Byte(float input_float, unsigned char *ptr_output){
 int i = 0, k = 0;
 unsigned char  *p1 = (unsigned char *)&input_float;

 i = sizeof(input_float);

 for (k = 0; k < i; k++){
     *ptr_output++ = *p1++;
      }
}


void sendSensorData(int messageID, float value_float ){
  
  uint8_t buffer_float[4];
  
  Serial.print("Sending packet ... ");

  float32BitTo4Byte( value_float, &buffer_float[0]);
  
  Serial.print(buffer_float[3],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);    // to Master  
  Serial.print(" ");
  Serial.println(buffer_float[0],HEX);  // to Master  
 /* */    
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();

  Serial.println("done");
}


void sendSensorData1(){
  int messageID = 0;
  float temporary_float = 22.13; // 3:41 2:B1 1:0A 0:3D
  uint8_t buffer_float[4];
  

  Serial.println("Sending packet ... ");
  Serial.print("Temp 1: ");

  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  messageID = 0x0101;

  float32BitTo4Byte( temporary_float, &buffer_float[0]);
  
  Serial.print(buffer_float[3],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);    // to Master  
  Serial.print(" ");
  Serial.println(buffer_float[0],HEX);  // to Master  
      
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();


  Serial.print("Temp 2: ");
  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  messageID = 0x0201;
  temporary_float = 23.13;  // 41 B9 0A 3D
  
  float32BitTo4Byte( temporary_float, &buffer_float[0]);

  Serial.print(buffer_float[3],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);    // to Master  
  Serial.print(" ");
  Serial.println(buffer_float[0],HEX);  // to Master  
  
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();


  Serial.print("Weigt: ");
  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  messageID = 0x0301;
  temporary_float = 80.1;  // 42 A0 33 33
    
  float32BitTo4Byte( temporary_float, &buffer_float[0]);

  Serial.print(buffer_float[3],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);    // to Master  
  Serial.print(" ");
  Serial.println(buffer_float[0],HEX);  // to Master  
  
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();

  Serial.print("Sound: ");
  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  messageID = 0x0401;
  temporary_float = 100.2;  // 42 C8 66 66
     
  float32BitTo4Byte( temporary_float, &buffer_float[0]);

  Serial.print(buffer_float[3],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);    // to Master  
  Serial.print(" ");
  Serial.println(buffer_float[0],HEX);  // to Master  
  
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();
  
  Serial.println(" done");
  
}

void sendSensorData2(){
  int messageID = 0;
  float temporary_float = 22.13; // 41 B1 0A 3D
  uint8_t buffer_float[4];
  uint8_t * buffer_ptr = (uint8_t*) &temporary_float;

  Serial.println("Sending packet ... ");
  Serial.print("Temp 1: ");

  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  messageID = 0x0101;

  // send packet: id is 11 bits, packet can contain up to 8 bytes of data
  buffer_float[3] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[2] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[1] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[0] = *buffer_ptr;
  //buffer_ptr--;buffer_ptr--;buffer_ptr--;

  Serial.print(buffer_float[0],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);    // to Master  
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);    // to Master  
  Serial.print(" ");
  Serial.println(buffer_float[3],HEX);    // to Master  
      
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();

  Serial.print("Temp 2: ");
  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  messageID = 0x0201;
  temporary_float = 23.13;  // 41 B9 0A 3D
    
  buffer_float[3] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[2] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[1] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[0] = *buffer_ptr;
  //buffer_ptr--;buffer_ptr--;buffer_ptr--;

  Serial.print(buffer_float[0],HEX);
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);
  Serial.print(" ");
  Serial.println(buffer_float[3],HEX);
  
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();


  Serial.print("Weigt: ");
  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  messageID = 0x0301;
  temporary_float = 80.1;  // 42 A0 33 33
    
  buffer_float[3] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[2] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[1] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[0] = *buffer_ptr;
 // buffer_ptr--;buffer_ptr--;buffer_ptr--;

  Serial.print(buffer_float[0],HEX);
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);
  Serial.print(" ");
  Serial.println(buffer_float[3],HEX);
  
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();

  Serial.print("Sound: ");
  // mesage temp 1 ID = 0b 1001 0000 0001 =  0x0101
  messageID = 0x0401;
  temporary_float = 100.2;  // 42 C8 66 66
     
  buffer_float[3] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[2] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[1] = *buffer_ptr;
  buffer_ptr++;
  buffer_float[0] = *buffer_ptr;
  buffer_ptr--;buffer_ptr--;buffer_ptr--;

  Serial.print(buffer_float[0],HEX);
  Serial.print(" ");
  Serial.print(buffer_float[1],HEX);
  Serial.print(" ");
  Serial.print(buffer_float[2],HEX);
  Serial.print(" ");
  Serial.print(buffer_float[3],HEX);
  
  CAN.beginPacket(messageID);
  CAN.write(buffer_float[0]);
  CAN.write(buffer_float[1]);
  CAN.write(buffer_float[2]);
  CAN.write(buffer_float[3]);
  CAN.endPacket();
  
  Serial.println(" done");


}
