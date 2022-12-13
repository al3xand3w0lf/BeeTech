#include <SPI.h>
#include <SD.h>
#include "HX711.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <Wire.h>               // for later
#include "stdio.h"  
#include <CAN.h>


#define FIRMWARE_VER 1

#define MODE_MASTER  0
#define MODE_MASTER_DISABLED 1

#define ONE_WIRE_SENSOR_1 6  // Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_SENSOR_2 7  // Data wire is plugged into port 2 on the Arduino


#define HX_DOUT      16
#define HX_CLK       17

// EVERY CAN setup
#define CAN_CS        5
#define CAN_INTERRUPT 4

#define PIN_NEOPIX   21   // Pin connected
#define NUMPIXELS    1    // number of connected neopixels

Adafruit_NeoPixel neopix(NUMPIXELS, PIN_NEOPIX, NEO_GRB + NEO_KHZ800);

#define setNeoPixColor_RED    neopix.setPixelColor(0,neopix.Color(255,0,0));neopix.show();  
#define setNeoPixColor_GREEN  neopix.setPixelColor(0,neopix.Color(0,255,0));neopix.show();
#define setNeoPixColor_BLUE   neopix.setPixelColor(0,neopix.Color(0,0,255));neopix.show();


OneWire oneWire1(ONE_WIRE_SENSOR_1);   // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensor1(&oneWire1);  // Pass our oneWire reference to Dallas Temperature. 
DeviceAddress insideThermometer1;      // arrays to hold device address

OneWire oneWire2(ONE_WIRE_SENSOR_2);    // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensor2(&oneWire2);  // Pass our oneWire reference to Dallas Temperature. 
DeviceAddress insideThermometer2;      // arrays to hold device address


// --- Analog ------------
const int analogSoundPin = A0;    // select the input pin for the potentiometer


// --- Modes--- ----------
int operationMode_int = 0;         // logging disabled/enabled
int operationModeOld_int = 0;       
int serialNumber_int = 0;           
int calibration_factor_int = 0;   
int stationNumber = 1;    
char stationName[100] = ""; 
int sdcard_logging=0;    
int sdcard_log_intervall=60;   

const int addr_serialNumber = 0;    // 1  byte
const int addr_scaleVale    = 2;    // 2 bytes
const int addr_stationName  = 10;   // 100 bytes

#define SENSOR_DISCONNECTED   0
#define SENSOR_CONNECTED      1


#define MESSAGE_ID_TEMP1      0x100
#define MESSAGE_ID_TEMP2      0x200
#define MESSAGE_ID_WEIGHT     0x300
#define MESSAGE_ID_SOUND      0x400


int TEMP1_CON = 0;
int TEMP2_CON = 0;
int SCALE_CON = 0;
int AUDIO_CON = 0;



// beehive struct
typedef struct {
  float temp1_float;
  float temp2_float;
  float weight_float;
  int   sound_int;
} beehive_t;

beehive_t BEEHIVE;

float WEIGHT_OFFSET_float = 0.0;

// --- Misc ------------------
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval_1000ms = 1000;      
const long interval_2000ms = 2000;      
const long interval_3000ms = 3000; 
const long interval_5000ms = 5000;      
const long interval_10000ms = 10000;      

bool CAN_received_flag = false;  
uint8_t Buffer_CAN[100] = "";         // a String to hold incoming data
char    Buffer_Terminal[100] = "";    // a String to hold incoming data
bool TerminalComplete = false;     // whether the string is complete

String dataString;



void setup()
{
  int i = 0;
    // serial Port via USB
    Serial.begin(9600, SERIAL_8N1);
    /*
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
    */
    
    Serial.println("");
    Serial.println("-----------------");
    Serial.println("- System Start  -");
    Serial.println("-----------------");
    

    // --- CAN ---
    Serial.println("CAN Receiver Callback");
    CAN.setPins(CAN_CS, CAN_INTERRUPT);    
    CAN.setSPIFrequency(4E6); // necessary for MKR, lower SPI frequenzy then standard by library

    while (!CAN.begin(500E3)) {        
      delay(5000);    
      i++;
      Serial.println("Starting CAN failed!");
      CAN.begin(500E3);
      if(i > 20)
        break;
      }
     Serial.println("CAN Started!");    
    

    // --- NeoPix ---

    neopix.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    //neopix.setPixelColor(0, neopix.Color(0, 150, 0));  // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    setNeoPixColor_GREEN;
    neopix.setBrightness(10); // Set BRIGHTNESS to about 1/5 (max = 255)
    neopix.show();   // Send the updated pixel colors to the hardware.
    
    Serial.println("---");
    serialNumber_int = getEepromSerialNumber();
    
    Serial.println("---");
    sdCard_init();
    sdCard_readConfigFile();
    Serial.println("---");
    
    setNeoPixColor_BLUE;
    
    TEMP1_CON = init_DS18B20_Temp1();
    TEMP2_CON = init_DS18B20_Temp2();
    Serial.println("---");
    SCALE_CON = scale_initialize();
    
    
    Serial.println("-----------------");
    Serial.println("- logging start -");
    Serial.println("-----------------");
    Serial.println("date,time,temp1C,temp2C,weight,sound,"); 

    CAN.onReceive(onReceive);   // register the receive callback
}


void onReceive(int packetSize) { 
  int i = 0;

  // received a packet
  // Serial.print("Received ");

   // if (CAN.packetRtr()) {
    // Remote transmission request, packet contains no data
    //Serial.print("RTR ");
  // }

  // Serial.print("packet with id 0x");
  // Serial.print(CAN.packetId(), HEX);

  if ( CAN.packetId() == stationNumber){ 
      Serial.println("---");        
      Serial.print("Received request - it's me Hive Nr: ");
      Serial.println(stationNumber);
      if (CAN.packetRtr()) {
         // Serial.print(" and requested length ");
          //Serial.println(CAN.packetDlc());
          } 
      else {
          //Serial.print(" and length ");
          //Serial.println(packetSize);
      
          // only print packet data for non-RTR packets
          i = 0;
          while (CAN.available()) {           
              Buffer_CAN[i] = CAN.read();
              //Serial.print(Buffer_CAN[i]);
              //Serial.print(" - ");              
              i++;     
              }
          Buffer_CAN[i] = 0;    
          // Serial.println();
          CAN_received_flag = true;
          }
    
      Serial.println();
    }
    
}



void loop()
{
  static int k = 0;
  char inChar = 0;
  static int i = 0;

  
  // --- Terminal User data -----------------------------
  if (Serial.available()) {          
      inChar = (char)Serial.read();      
      Buffer_Terminal[k] = inChar;
      k++;
      if (inChar == 0x0D) { // if (inChar == '\n') {
          Buffer_Terminal[k] = 0;            
          k = 0;
          processTerminalData(); 
          }
      }


  // --- Network: Data from Master ----------------------  
  // Buffer_Softserial
  if (CAN_received_flag == true) {                
   
      getSensorData();   
      printSensorData();

      sendSensorData(  MESSAGE_ID_TEMP1  + stationNumber, BEEHIVE.temp1_float );
      sendSensorData(  MESSAGE_ID_TEMP2  + stationNumber, BEEHIVE.temp2_float );
      sendSensorData(  MESSAGE_ID_WEIGHT + stationNumber, BEEHIVE.weight_float  );
      sendSensorData(  MESSAGE_ID_SOUND  + stationNumber, BEEHIVE.sound_int ); 
      /*
      sendSensorData(  MESSAGE_ID_TEMP1  + stationNumber, 22.13 );
      sendSensorData(  MESSAGE_ID_TEMP2  + stationNumber, 23.13 );
      sendSensorData(  MESSAGE_ID_WEIGHT + stationNumber, 80.1  );
      sendSensorData(  MESSAGE_ID_SOUND  + stationNumber, 100.2 ); 
      */
      CAN_received_flag = false;                        
      }

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval_1000ms){     
        previousMillis = currentMillis;
                
        if(i == 0){
            setNeoPixColor_BLUE;
            i++;
            }
        else if(i == 1){  
            setNeoPixColor_GREEN;
            i = 0;
            }                 
        }
                     
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



void printSensorData(){
  Serial.print (stationNumber);    // to Terminal
  Serial.print (","); 
  Serial.print (BEEHIVE.temp1_float);    // to Terminal
  Serial.print (",");  
  Serial.print (BEEHIVE.temp2_float);    // to Terminal
  Serial.print (",");  
  Serial.print (BEEHIVE.weight_float);   // to Terminal
  Serial.print (",");  
  Serial.print (BEEHIVE.sound_int);      // to Terminal   
  Serial.print (",");  
  Serial.println ("");      // to Terminal   
}


void createDataString(){

  // string: station,behiveName,
 
  dataString = String(stationNumber);
  dataString += ",";  
  dataString += stationName;
  dataString += ","; 
  dataString += BEEHIVE.temp1_float;
  dataString += ",";  
  dataString += BEEHIVE.temp2_float;
  dataString += ",";  
  dataString += BEEHIVE.weight_float;
  dataString += ",";  
  dataString += BEEHIVE.sound_int;
  dataString += ",";  
}


void getSensorData(){
  float WEIGHT_SENSOR_float = 0.0;

  if( TEMP1_CON == SENSOR_CONNECTED){
      sensor1.requestTemperatures(); // Send the command to get temperatures
      BEEHIVE.temp1_float = getTemperature_sensor1(insideThermometer1); // Use a simple function to print out the data
      }
  else    
      BEEHIVE.temp1_float = 20.0;
  if( TEMP2_CON == SENSOR_CONNECTED){
      sensor2.requestTemperatures(); // Send the command to get temperatures
      BEEHIVE.temp2_float = getTemperature_sensor2(insideThermometer2); // Use a simple function to print out the data
      }
  else    
      BEEHIVE.temp2_float = 21.0;
  if( SCALE_CON == SENSOR_CONNECTED){
      WEIGHT_SENSOR_float = scale_readings();      
      BEEHIVE.weight_float = WEIGHT_OFFSET_float + WEIGHT_SENSOR_float;
      }
  else    
      BEEHIVE.weight_float = 90.0;      
  
  BEEHIVE.sound_int = getSound(analogSoundPin) ; // analogRead(analogSoundPin);      
  
}
