#include "stdio.h"
#include <Wire.h>               
#include <SPI.h>
#include <SD.h>
#include <RTCZero.h>  // RTCZero is used for SAMD21 Boards internal RTC
#include <MKRGSM.h>             
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Adafruit_FRAM_SPI.h"
#include "HX711.h"
#include <Adafruit_NeoPixel.h>
#include <CAN.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>


#define FIRMWARE_VER 1

#define MODE_MASTER_ALONE    0
#define MODE_MASTER_NETWORK  1
#define MODE_MASTER_DISABLED 2

#define ONE_WIRE_SENSOR_1 4  // Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_SENSOR_2 5  // Data wire is plugged into port 2 on the Arduino

#define HX_DOUT      1
#define HX_CLK       2

#define TFT_RST      18
#define TFT_DC       19
//#define TFT_CS       13  // CS at new ver 3 board, pin 12 is SCL
#define TFT_CS       12 // CS at last modified hacked board
#define TFT_EN       20

#define FRAM_CS      21  // CS at new ver 3 board,

#define DEF_SWITCH   0
#define DEF_BUTTON1  16
#define DEF_BUTTON2  17

#define DEF_SDCARD_CS 6

// CAN PINS
#define CAN_CS 11
#define CAN_INTERRUPT 7

#define PIN_NEOPIX   3    // Pin connected
#define NUMPIXELS    1    // number of connected neopixels

Adafruit_NeoPixel neopix(NUMPIXELS, PIN_NEOPIX, NEO_GRB + NEO_KHZ800);

#define setNeoPixColor_RED    neopix.setPixelColor(0,neopix.Color(255,0,0));neopix.show();  
#define setNeoPixColor_GREEN  neopix.setPixelColor(0,neopix.Color(0,255,0));neopix.show();
#define setNeoPixColor_BLUE   neopix.setPixelColor(0,neopix.Color(0,0,255));neopix.show();

// --- Temperature Sensors --------------------------------------

OneWire oneWire1(ONE_WIRE_SENSOR_1);   // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensor1(&oneWire1);  // Pass our oneWire reference to Dallas Temperature. 
DeviceAddress insideThermometer1;      // arrays to hold device address

OneWire oneWire2(ONE_WIRE_SENSOR_2);    // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensor2(&oneWire2);  // Pass our oneWire reference to Dallas Temperature. 
DeviceAddress insideThermometer2;      // arrays to hold device address

// --------------------------------------------------------------

RTCZero rtc; // initialize the Time library instance

// internal time struct
typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec;
} timeStruct_t;


// --- GSM & MQTT -----------------------------------------------
GSM gsmAccess;                        // GSM access: include a 'true' parameter for debug enabled
GPRS gprsAccess;                      // GPRS access
GSMClient client;  // Client service for TCP connection
MySQL_Connection conn((Client *)&client);

char user[] = "k139859_n";              // MySQL user login username
char password[] = "22222";              // MySQL user login password

int databaseLog_int = 0;
byte mac_addr[] = {0xDE,0xAD,0xBE,0xEF,0xFE,0xED };
IPAddress server_addr(91,204,46,146);    // IP of the MySQL *server* here

char INSERT_SQL[200] = "INSERT INTO k139859_test_arduino.test_data (location,hive,temp1,temp2,weight,sound) VALUES ('Zuin',1,10.0,24.0,50.0,30.0)";


// --------------------------------------------------------------

Adafruit_ILI9341 tft   = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);  // using hardware SPI

Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS);                 // use hardware SPI
const int addr_serialNumber = 0;    // 1  byte
const int addr_scaleVale    = 2;    // 2 bytes
const int addr_stationName  = 10;   // 100 bytes


// --- Buttons ----------
const int SWITCH  = DEF_SWITCH ; //0;  // interrupt ?  
const int BUTTON1 = DEF_BUTTON1 ; //16; //20;  // interrupt ?
const int BUTTON2 = DEF_BUTTON2 ; //17; //21;  // interrupt ?


// --- Analog ------------
const int analogSoundPin = A0;    // select the input pin for the potentiometer

// --- Modes--- ----------
int operationMode_int = 0;         
int operationModeOld_int = 0;   
int serialNumber_int = 0;          
int calibration_factor_int = 0;   
int stationNumber = 1;    
char stationName[100] = ""; 
int number_beehives = 1;
int network_enabled = 0;
int upload_timing = 12;
int sdcard_logging=0;    
int sdcard_log_intervall=60;

char GSM_APN[20]      = "";          
char GSM_LOGIN[20]    = "";     
char GSM_PASSWORD[20] = "";     
char SIM_PIN[10]      = ""; 

#define SENSOR_DISCONNECTED   0
#define SENSOR_CONNECTED      1

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

beehive_t BEEHIVE[20];

float WEIGHT_OFFSET_float = 0.0;


// --- Misc ------------------
unsigned long previousMillis = 0;        // will store last time 
unsigned long previousMillis1Sec = 0;        // will store last time 
const long interval_1000ms = 1000;      
const long interval_2000ms = 2000;      
const long interval_5000ms = 5000;      
const long interval_10000ms = 10000;   

// CAN Bus struct
typedef struct {
  int     messageId;
  uint8_t receiveBuffer[10];
  float   value_float;
} canMessage_t;

canMessage_t CAN_MESSAGE;




char inputStream[100] = "";      // a String to hold incoming data from sensors
bool stringComplete = false;     // whether the string is complete

char Buffer_Terminal[100] = "";  // a String to hold incoming data from terminal
bool TerminalComplete = false;   // whether the string is complete

String dataString;              // data string which will be saved in File on SD Card
char logFileName_char[20];      // Log File Name

int  num_tft_beehive = 0;        // current screen number, switching screens via Button 


// int operationMode_int = 0;         
// int operationModeOld_int = 0;   
// Interrupt Service Routine (ISR)
void ISR_switch (){
  if (digitalRead (SWITCH) == LOW){   // System disabled
    operationModeOld_int = operationMode_int;
    operationMode_int = MODE_MASTER_DISABLED;
    Serial.println("Pin LOW");
    Serial.println("System Disabled"); 
    Serial.print("System mode: "); 
    Serial.println(operationMode_int); 
  }
  else{   // System enabled
    operationMode_int = operationModeOld_int;
    Serial.println("Pin HIGH");
    Serial.print("System mode: "); 
    Serial.println(operationMode_int); 
  } 
} 


// Interrupt Service Routine (ISR)
void ISR_button1_Pressed ()
{ 
  if (digitalRead (TFT_EN) == LOW){
    digitalWrite(TFT_EN, HIGH);   // turn the LED on (HIGH is the voltage level)
  }
  else{
    digitalWrite(TFT_EN, LOW);   // turn the LED on (HIGH is the voltage level)
  }
}  

// Interrupt Service Routine (ISR)
void ISR_button2_Pressed ()
{
  Serial.println("BUTTON 2");
  num_tft_beehive++;
  if(num_tft_beehive == number_beehives)
    num_tft_beehive = 0;
} 

void soft_reset() {  
   NVIC_SystemReset(); 
} 


int get_free_memory()
{
  extern char __bss_end;
  extern char *__brkval;
  int free_memory;
  if((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);
  return free_memory;
}


void setup()
{
    int i = 0;
    Serial.begin(9600);
    
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
    
    Serial.println("");
    Serial.println("-----------------");
    Serial.println("- System Start  -");
    Serial.println("-----------------");
    
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(TFT_EN,OUTPUT);

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
    init_Fram();
    sdCard_init();
    sdCard_readConfigFile();
    
    serialNumber_int = getFramSerialNumber();
    Serial.print("SERIAL intern = ");
    Serial.println(serialNumber_int);    
    Serial.println("---");   

    //databaseLog_int = 0;

    TEMP1_CON = init_DS18B20_Temp1();
    TEMP2_CON = init_DS18B20_Temp2();
    Serial.println("---");
    SCALE_CON = scale_initialize();
      
   // digitalWrite(TFT_EN, HIGH);  // turn the LED on (HIGH is the voltage level)
    digitalWrite(TFT_EN, LOW);  
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
    tft.print("FW ver:  ");
    tft.println(FIRMWARE_VER);
    
    if(SCALE_CON)
        tft.println("scale init OK");
    else
        tft.println("scale ERROR");
    if(TEMP1_CON)
        tft.println("Temp 1 OK");
    else
        tft.println("Temp 1 ERROR");    
    if(TEMP1_CON)
        tft.println("Temp 2 OK");
    else
        tft.println("Temp 2 ERROR");

    
    operationMode_int = MODE_MASTER_NETWORK;

    delay(5000);
    // --- Config TIME --------------------
    rtc.begin(); // initialize RTC
    rtc.setTime(12, 22, 33);
    rtc.setDate(1, 1, 20);
       
    Serial.println("---");
    
    if(databaseLog_int == 1){       
        InitModem();        
        rtc.setEpoch(gsmAccess.getTime());                                        
    }
    else{ // not connectetd Network
        rtc.setTime(12, 22, 33);
        rtc.setDate(1, 1, 20); 
        // rtc.setTime(hours, minutes, seconds);
        // rtc.setDate(day, month, year);      
    }

    // --- CAN Bus Init --------------------
        
    // start the CAN bus at 500 kbps
    CAN.setPins(CAN_CS, CAN_INTERRUPT);
    CAN.setSPIFrequency(4E6); // necessary foor MKR, lower SPI frequenzy then standard by library
    while (!CAN.begin(500E3)) {        
      delay(1000);    
      i++;
      Serial.println("Starting CAN failed!");
      CAN.setSPIFrequency(4E6); // necessary foor MKR, lower SPI frequenzy then standard by library
      CAN.begin(500E3);
      if(i > 20)
        break;
      }
     Serial.println("CAN Started!");   
  
    // --- LOG SCREEN -----------------------
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

    
    Serial.println("date,time,num_hive,temp1C,temp2C,weight,sound,"); 
    getLocalSensorData(stationNumber);
    updateTFT(0);

    CAN.onReceive(onReceive);   // register the receive callback
    
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
    if(i>9)
      break;
    Serial.print(" ");
  }
  Serial.println();
  

  CAN_MESSAGE.value_float = S4ByteToFloat32Bit(&CAN_MESSAGE.receiveBuffer[0]);
  Serial.print(" its float value: ");
  Serial.println(CAN_MESSAGE.value_float);
  Serial.println();
  
}


void receiveCanPoll(){
  // try to parse packet
  uint8_t i = 0;
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
      i = 0;
      // only print packet data for non-RTR packets
      while (CAN.available()) {
          CAN_MESSAGE.receiveBuffer[i] = CAN.read();
          Serial.print(CAN_MESSAGE.receiveBuffer[i], HEX);
          i++;
          if(i>9)
            break;
          Serial.print(" ");
      }
      CAN_MESSAGE.value_float = S4ByteToFloat32Bit(&CAN_MESSAGE.receiveBuffer[0]);
      Serial.print(" its float value: ");
      Serial.println(CAN_MESSAGE.value_float);
      Serial.println();      
    }
    Serial.println();
  }
}


void loop()
{
  
  static int k = 0;
  static int i = 0;

  // check if there is Terminal User data
  if (Serial.available()) {            // If anything comes in Serial (from master)
      char inChar = (char)Serial.read();      
      Buffer_Terminal[k] = inChar;
      k++;
      // if the incoming character is a newline, set a flag so the main loop can
      // do something about it:
      if (inChar == 0x0D) { // if (inChar == '\n') {
          Buffer_Terminal[k] = 0;  
          k = 0;
          processTermianData();
      }
  }
  switch (operationMode_int) {  
    case MODE_MASTER_ALONE: 
            loop_master_disabled();  
            break;  
    case MODE_MASTER_NETWORK: 
            loop_master_network(); 
            break;
   case MODE_MASTER_DISABLED: 
            loop_master_disabled(); 
            break;   
   }
   
}




void loop_master_disabled(){
  
  // here is the delayed code
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval_2000ms){   
      previousMillis = currentMillis;
      // code to be executed:
      
      getLocalSensorData(stationNumber);
      printTime();
      printSensorData(0);
      if (digitalRead (TFT_EN) == HIGH)
          updateTFT(0);
      setNeoPixColor_RED;                    
  } 
}



void loop_master_network(){
  
  static int i = 0, k = 0, num_fails = 0, num_beehive = 0, num_uploads = 0, cntToSdCardSave = 0;
  // here is where you'd put code that needs to be running all the time 


  // Timer 1 sec
  unsigned long currentMillis1Sec = millis();
  if(currentMillis1Sec - previousMillis1Sec >= interval_2000ms ){
      previousMillis1Sec = currentMillis1Sec;
      
      //cntToSdCardSave++;
      
      if (digitalRead (TFT_EN) == HIGH)
          updateTFT(num_tft_beehive); 
      /*
     
      // logging data to SD Card
      if((sdcard_logging == 1) && ( sdcard_log_intervall == cntToSdCardSave)){
          cntToSdCardSave = 0;          
          logDataToSdCard(0);
          }     
          */
      }

  //receiveCanPoll();

  // Timer 10 sec
  // here is the delayed code
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= (1*interval_5000ms)  ){      
      previousMillis = currentMillis;

      if(k == 0){
          setNeoPixColor_BLUE;
          k++;
          }
      else if(k == 1){  
          setNeoPixColor_GREEN;
          k = 0;
          }
      
     
      getLocalSensorData(stationNumber); 
      if( num_beehive != stationNumber){         // any other slave in the chain 
          getNetworkSensorData(num_beehive);    // call for sensor data 
           
//          receiveCanPoll();   // SPI Error mit TFT, wen TFT eingeschalten, dann dreht CAN SPI BUs durch

      }

      Serial.println("");
      Serial.println("Hive values: ");
      Serial.print("Time: ");
      printTime();     
      printSensorData(num_beehive);
      Serial.println(" --- ");
              
      num_beehive++;
      if(num_beehive == number_beehives){      
          num_beehive = 0;
      }
      
      /*
      if(databaseLog_int == 1){ // if logging to database is enabled
          i++;
          if(i == upload_timing){      // Uploadad Time
              i = 0;
              if (conn.connected()) {
                  for( num_uploads = 0; num_uploads < number_beehives; num_uploads++){                
                       uploadData2Mysql(num_uploads);
                       //Serial.print("upload number :");  
                       //Serial.println (num_uploads);  
                  }
                  //testMysql();
              } 
              else {
                  conn.close();
                  Serial.println("Connecting...");
                  if (conn.connect(server_addr, 3306, user, password)) {
                      delay(500);
                      Serial.println("Successful reconnect!");
                      for( num_uploads = 0; num_uploads < number_beehives; num_uploads++){                
                           uploadData2Mysql(num_uploads);
                           //Serial.print("upload number :");  
                           //Serial.println (num_uploads);  
                      }
                      num_fails = 0;
                  } 
                  else {
                      Serial.println("ERROR cannot reconnect!");
                      num_fails++;
                      if(num_fails > 10){
                            Serial.println("Ok, that's it. I'm outta here. Rebooting...");
                            delay(2000);
                            soft_reset();
                      }
                  }
              }                
          }
      }
      */         
  }  
  
}



void createDataString(int num_hive){
   
  dataString = String(stationNumber);
  dataString += ",";  
  dataString += stationName;
  dataString += ","; 
  dataString += rtc.getYear();
  dataString += "/";
  dataString += rtc.getMonth();
  dataString += "/";
  dataString += rtc.getDay();
  dataString += ",";
  
  dataString += rtc.getHours();
  dataString += ":";
  dataString += rtc.getMinutes();
  dataString += ":";
  dataString += rtc.getSeconds();
  dataString += ",";
  dataString += BEEHIVE[num_hive].temp1_float;
  dataString += ",";  
  dataString += BEEHIVE[num_hive].temp2_float;
  dataString += ",";  
  dataString += BEEHIVE[num_hive].weight_float;
  dataString += ",";  
  dataString += BEEHIVE[num_hive].sound_int;
  dataString += ",";  
  
}

/*
void printSensorData(int num_beehive){

  Serial.print("print sensor data Beehive number: ");  
  Serial.println (num_beehive);   

  Serial.print (num_beehive);    // to Terminal
  Serial.print (","); 
  Serial.print (BEEHIVE[num_beehive].temp1_float);    // to Terminal
  Serial.print (","); 
  Serial.print (BEEHIVE[num_beehive].temp2_float);    // to Terminal
  Serial.print (",");   
  Serial.print (BEEHIVE[num_beehive].weight_float);   // to Terminal
  Serial.print (",");   
  Serial.print (BEEHIVE[num_beehive].sound_int);      // to Terminal  
  Serial.print (",");    
  Serial.println ("");      // to Terminal      
}
*/

void printSensorData(int num_beehive){
  Serial.println ("");   
  Serial.print("print sensor data Beehive number: ");  
  Serial.println (num_beehive);
     
  Serial.print ("hive: ");
  Serial.print (num_beehive);    // to Terminal
  Serial.print (", temp1: "); 
  Serial.print (BEEHIVE[num_beehive].temp1_float);    // to Terminal
  Serial.print (", temp2: "); 
  Serial.print (BEEHIVE[num_beehive].temp2_float);    // to Terminal
  Serial.print (", weight: ");   
  Serial.print (BEEHIVE[num_beehive].weight_float);   // to Terminal
  Serial.print (", sound: ");   
  Serial.print (BEEHIVE[num_beehive].sound_int);      // to Terminal  
  Serial.println (",");   
  
}


void updateTFT(int num_beehive){
  
  static float temp1C_float = 0.0;
  static float temp2C_float = 0.0;
  static float weight_float = 0.0;
  static int   sound_int  = 0; 
  static int   num_beehive_old  = 0;
  static timeStruct_t timeNow, timeOld;

   
  // erase values
  tft.setCursor(0,70);
  tft.setTextColor(ILI9341_BLACK);
  tft.print("Hive numb: ");
  tft.println(num_beehive_old);
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
  
  // ----------------------------------------------------
  // write new values
  tft.setCursor(0,70);
  tft.setTextColor(ILI9341_GREEN);   
  tft.print("Hive numb: ");
  tft.println(num_beehive);
  tft.print("BeeHive T: ");
  tft.println(BEEHIVE[num_beehive].temp1_float);
  tft.print("Scale   T: ");
  tft.println(BEEHIVE[num_beehive].temp2_float);
  tft.print("Scale  kg: ");
  tft.println(BEEHIVE[num_beehive].weight_float);
  tft.print("Sound dec: ");
  tft.println(BEEHIVE[num_beehive].sound_int);

  // current time and date
   
  timeNow.year  = rtc.getYear();
  timeNow.month = rtc.getMonth();
  timeNow.day   = rtc.getDay();
  timeNow.hour  = rtc.getHours();
  timeNow.min   = rtc.getMinutes();
  timeNow.sec   = rtc.getSeconds();
  
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
  
  num_beehive_old = num_beehive;
  temp1C_float = BEEHIVE[num_beehive].temp1_float;
  temp2C_float = BEEHIVE[num_beehive].temp2_float;
  weight_float = BEEHIVE[num_beehive].weight_float;
  sound_int  = BEEHIVE[num_beehive].sound_int;  
  
}
