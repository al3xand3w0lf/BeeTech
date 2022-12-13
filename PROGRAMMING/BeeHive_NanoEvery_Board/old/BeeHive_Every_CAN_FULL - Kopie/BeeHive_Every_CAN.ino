#include <SPI.h>
#include <SD.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "HX711.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <Wire.h>               // for later
#include "stdio.h"
#include "RTClib.h"
#include <CAN.h>


#define FIRMWARE_VER 1

#define MODE_MASTER  0
#define MODE_MASTER_DISABLED 1

#define ONE_WIRE_SENSOR_1 6  // Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_SENSOR_2 7  // Data wire is plugged into port 2 on the Arduino

#define TFT_RST      8
#define TFT_DC       9
#define TFT_CS       10
#define TFT_EN       15

#define HX_DOUT      16
#define HX_CLK       17

// EVERY CAN setup
#define CAN_CS 5
#define CAN_INTERRUPT 4


#define PIN_NEOPIX   21   // Pin connected
#define NUMPIXELS    1    // number of connected neopixels

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);  // using hardware SPI

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

// --- Time -------------

RTC_Millis rtc;  // software lib

// internal time struct
typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec;
} timeStruct_t;

// --- Buttons ----------

const int SWITCH  =  20;  // interrupt 28   
const int BUTTON1 =  18;  // interrupt 2
const int BUTTON2 =  19;  // interrupt 3

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

int TEMP1_CON = 0;
int TEMP2_CON = 0;
int SCALE_CON = 0;
int AUDIO_CON = 0;

/*
// CAN Bus struct
typedef struct {
  int     messageId;
  uint8_t receiveBuffer[10];
  float   value_float;
  
} canMessage_t;

canMessage_t CANMESSAGE
*/

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
const long interval_5000ms = 5000;      
const long interval_10000ms = 10000;      

bool CAN_received_flag = false;  
uint8_t Buffer_CAN[100] = "";         // a String to hold incoming data
char    Buffer_Terminal[100] = "";    // a String to hold incoming data
bool TerminalComplete = false;     // whether the string is complete

String dataString;
char logFileName_char[20];

// Interrupt Service Routine (ISR)
void ISR_switch ()
{ 
     
  if (digitalRead (SWITCH) == HIGH){   // System enabled
      operationModeOld_int = operationMode_int;
      operationMode_int = MODE_MASTER_DISABLED;
      Serial.println("Pin High");
      Serial.println("System Disabled"); 
      Serial.print("System mode: "); 
      Serial.println(operationMode_int);       
  }
  else{   // System disabled
      operationMode_int = operationModeOld_int;
      Serial.println("Pin Low");
      Serial.println("System Enabled"); 
      Serial.print("System mode: "); 
      Serial.println(operationMode_int); 
  } 
} 


// Interrupt Service Routine (ISR)
void ISR_button1_Pressed ()
{ 

}  

// Interrupt Service Routine (ISR)
void ISR_button2_Pressed ()
{
  if (digitalRead (TFT_EN) == LOW){
    digitalWrite(TFT_EN, HIGH);   // turn the LED on (HIGH is the voltage level)
  }
  else{
    digitalWrite(TFT_EN, LOW);   // turn the LED on (HIGH is the voltage level)
  }
} 

void setup()
{
  int i = 0;
    // serial Port via USB
    Serial.begin(9600, SERIAL_8N1);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
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
    

    // --- Digital ---
    pinMode(TFT_EN, OUTPUT);
    pinMode(BUTTON1, INPUT);
    pinMode(BUTTON2, INPUT); 
    pinMode(SWITCH , INPUT);
    attachInterrupt (digitalPinToInterrupt (BUTTON1), ISR_button1_Pressed, FALLING);  // attach interrupt handler
    attachInterrupt (digitalPinToInterrupt (BUTTON2), ISR_button2_Pressed, FALLING);  // attach interrupt handler
    attachInterrupt (digitalPinToInterrupt (SWITCH ), ISR_switch         , CHANGE);  // attach interrupt handler

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
    
    digitalWrite(TFT_EN, LOW);
    //digitalWrite(TFT_EN, HIGH);
    init_tft();
    
    tft.fillScreen(ILI9341_BLACK);

    testLines(ILI9341_CYAN);

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0,0);
    tft.setTextColor(ILI9341_GREEN);   
    tft.println(stationName);
    
    tft.print("Number ");
    tft.println(stationNumber);
    
    tft.print("Serial ");
    tft.println(serialNumber_int);

    tft.print("scale ");
    tft.println(calibration_factor_int);
    
    if( operationMode_int == MODE_MASTER)
        tft.println("Master mode");

    tft.print("FW ver:  ");
    tft.println(FIRMWARE_VER);
    
    delay(4000);
    
    rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
    
    setNeoPixColor_BLUE;
    
    TEMP1_CON = init_DS18B20_Temp1();
    TEMP2_CON = init_DS18B20_Temp2();
    Serial.println("---");
    SCALE_CON = scale_initialize();
    
    // ---------------------------------
    // LOG SCREEN
    // ---------------------------------
    
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0,0);
    tft.setTextColor(ILI9341_GREEN);   
    tft.println(stationName);
    tft.print("Bee Hive ");
    tft.println(stationNumber);
    tft.println("-----------");
    tft.setCursor(0,80);
    
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
      Serial.print("Received request - it's me ");
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
                             
      Serial.print("Received data: ");    
      Serial.print("year: ");
      Serial.print(Buffer_CAN[2]);
      Serial.print(" month: ");
      Serial.print(Buffer_CAN[1]);
      Serial.print(" day: ");
      Serial.print(Buffer_CAN[0]);
      Serial.print(" hour: ");
      Serial.print(Buffer_CAN[3]);
      Serial.print(" min: ");
      Serial.print(Buffer_CAN[4]);                  
      Serial.print(" sec: ");
      Serial.println(Buffer_CAN[5]);
            
      rtc.adjust(DateTime((uint16_t)Buffer_CAN[2] + 2000,  // year
                          (uint8_t) Buffer_CAN[1],     // month
                          (uint8_t) Buffer_CAN[0],     // day                      
                          (uint8_t) Buffer_CAN[3],     // hour
                          (uint8_t) Buffer_CAN[4],     // min 
                          (uint8_t) Buffer_CAN[5]));   // sec
      
      // sendSensorData();
      sendSensorData(  0x0101, 22.13 );
      sendSensorData(  0x0201, 23.13 );
      sendSensorData(  0x0301, 80.1 );
      sendSensorData(  0x0401, 100.2 ); 
      CAN_received_flag = false;                        
      }

  // this costs 10% of Flash memory
  /*
  cntToSdCardSave++;          
  // logging data to SD Card
  if((sdcard_logging == 1) && ( sdcard_log_intervall == cntToSdCardSave)){
      cntToSdCardSave = 0;          
      logDataToSdCard();
      }
  */


  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval_5000ms){     
        previousMillis = currentMillis;
        
        getSensorData(); 
        printTime();
        printSensorData();
        // sendSensorData();
        /*
        sendSensorData(  0x0101, 22.13 );
        sendSensorData(  0x0201, 23.13 );
        sendSensorData(  0x0301, 80.1 );
        sendSensorData(  0x0401, 100.2 );
        */
        if (digitalRead (TFT_EN) == HIGH)
            updateTFT();
            
        if( operationMode_int != MODE_MASTER_DISABLED ){            
            if(i == 0){
                setNeoPixColor_BLUE;
                i++;
                }
            else if(i == 1){  
                setNeoPixColor_GREEN;
                i = 0;
                }                 
            }
         else
            setNeoPixColor_RED;    
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
  DateTime now = rtc.now();
  // string: station,behiveName,date,time,
  
  dataString = String(stationNumber);
  dataString += ",";  
  dataString += stationName;
  dataString += ","; 
  dataString += now.year()-2000;
  dataString += "/";
  dataString += now.month();
  dataString += "/";
  dataString += now.day();
  dataString += ",";
  
  dataString += now.hour();
  dataString += ":";
  dataString += now.minute();
  dataString += ":";
  dataString += now.second();
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
  if( TEMP2_CON == SENSOR_CONNECTED){
      sensor2.requestTemperatures(); // Send the command to get temperatures
      BEEHIVE.temp2_float = getTemperature_sensor2(insideThermometer2); // Use a simple function to print out the data
      }
  if( SCALE_CON == SENSOR_CONNECTED){
      WEIGHT_SENSOR_float = scale_readings();
      
      BEEHIVE.weight_float = WEIGHT_OFFSET_float + WEIGHT_SENSOR_float;
      /*
      if(BEEHIVE.weight_float < 0.0)
         BEEHIVE.weight_float = 0.0;        // in case correction factor from SD CArd is slightly different
     */
      }
  
  BEEHIVE.sound_int = getSound(analogSoundPin) ; // analogRead(analogSoundPin);      
  
}

void updateTFT(){
  static float temp1C_float = 0.0;
  static float temp2C_float = 0.0;
  static float weight_float = 0.0;
  static int   sound_int  = 0;
  static timeStruct_t timeNow, timeOld;
  DateTime now = rtc.now();
   
  // erase values
  tft.setCursor(0,80);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("BeeHive T: ");
  tft.println(temp1C_float);
  tft.print("Scale   T: ");
  tft.println(temp2C_float);
  tft.print("Scale  kg: ");
  tft.println(weight_float);
  tft.print("Sound dec: ");
  tft.println(sound_int);
  // time and date 
  tft.println("");
  tft.print(timeOld.day);
  tft.print("/");
  tft.print(timeOld.month);
  tft.print("/");
  tft.print(timeOld.year);
  tft.print(" ");
  tft.print(timeOld.hour);
  tft.print(":");
  tft.print(timeOld.min);
  tft.print(":");
  tft.println(timeOld.sec);

  // ---------------------------------------------

  // write new values
  tft.setCursor(0,80);
  tft.setTextColor(ILI9341_GREEN);   
  tft.print("BeeHive T: ");
  tft.println(BEEHIVE.temp1_float);
  tft.print("Scale   T: ");
  tft.println(BEEHIVE.temp2_float);
  tft.print("Scale  kg: ");
  tft.println(BEEHIVE.weight_float);
  tft.print("Sound dec: ");
  tft.println(BEEHIVE.sound_int);

  // current time and date
  timeNow.year  = now.year()-2000;
  timeNow.month = now.month();
  timeNow.day   = now.day();
  timeNow.hour  = now.hour();
  timeNow.min   = now.minute();
  timeNow.sec   = now.second();
  tft.println("");
  tft.print(timeNow.day);
  tft.print("/");
  tft.print(timeNow.month);
  tft.print("/");
  tft.print(timeNow.year);
  tft.print(" ");
  tft.print(timeNow.hour);
  tft.print(":");
  tft.print(timeNow.min);
  tft.print(":");
  tft.println(timeNow.sec);
  
  timeOld = timeNow;
  
  temp1C_float = BEEHIVE.temp1_float;
  temp2C_float = BEEHIVE.temp2_float;
  weight_float = BEEHIVE.weight_float;
  sound_int  = BEEHIVE.sound_int;  
}
