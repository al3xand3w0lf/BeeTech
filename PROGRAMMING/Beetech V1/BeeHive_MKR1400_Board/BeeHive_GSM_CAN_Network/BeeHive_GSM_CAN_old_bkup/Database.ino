// https://github.com/ChuckBell/MySQL_Connector_Arduino/wiki/Common-Solutions


// things mobile
#define SECRET_PINNUMBER     ""
#define SECRET_GPRS_APN      "TM"  // replace your GPRS APN
#define SECRET_GPRS_LOGIN    ""    // replace with your GPRS login
#define SECRET_GPRS_PASSWORD ""    // replace with your GPRS password

/*
// swisscom
#define SECRET_PINNUMBER     ""
#define SECRET_GPRS_APN      "gprs.swisscom.ch" // replace your GPRS APN
#define SECRET_GPRS_LOGIN    "gprs"             // replace with your GPRS login
#define SECRET_GPRS_PASSWORD "gprs"             // replace with your GPRS password
*/

const char PINNUMBER[]     = SECRET_PINNUMBER;
const char GPRS_APN[]      = SECRET_GPRS_APN;
const char GPRS_LOGIN[]    = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASSWORD;



// initialize the library instance
GSMSSLClient client2;
GPRS gprs2;
GSM gsmAccess2;


void InitModem() {
 // Initialize serial and wait for port to open:

 Serial.println("---");
 Serial.println("Starting Arduino GPRS ping.");
 // connection state
 bool connected = false;

 // After starting the modem with GSM.begin()
 // attach the shield to the GPRS network with the APN, login and password
 while (!connected) {
   if ((gsmAccess2.begin(PINNUMBER) == GSM_READY) &&
       (gprs2.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
     connected = true;
   } else {
     Serial.println("Not connected");
     delay(1000);
   }
 }
 Serial.println("Modem started");
 Serial.println("---");
}



void uploadData2Mysql(int num_beehive){
  char *result_ptr = 0;
  char stn_out[20];
  char INSERT_SQL_temporary[150];
  //Serial.println("preparing data for upload 2 Database");
  // char INSERT_SQL[200] = "INSERT INTO k139859_test_arduino.test_data (location,hive,temp1,temp2,weight,sound) VALUES ('Zuin',1,10.0,24.0,50.0,30.0)";
  
  strcpy( INSERT_SQL_temporary, INSERT_SQL);
  // Serial.println(INSERT_SQL_temporary);
  Serial.println("---");
  
  result_ptr = strstr(INSERT_SQL_temporary,"VALUES");
  if(result_ptr > 0){
 
     *result_ptr = 0;

     strcat(INSERT_SQL_temporary , "VALUES ('");
     
     strcat( INSERT_SQL_temporary ,stationName);     // station name
     
     sprintf(stn_out, "',%.0f,", (float) num_beehive);  // station number
     strcat( INSERT_SQL_temporary ,stn_out);

     sprintf(stn_out, "%.1f,", BEEHIVE[num_beehive].temp1_float);      // temp1
     strcat( INSERT_SQL_temporary ,stn_out);

     sprintf(stn_out, "%.1f,", BEEHIVE[num_beehive].temp2_float);      // temp2
     strcat( INSERT_SQL_temporary ,stn_out);

     sprintf(stn_out, "%.1f,", BEEHIVE[num_beehive].weight_float);     // weight
     strcat( INSERT_SQL_temporary ,stn_out);

     sprintf(stn_out, "%.0f)", (float)BEEHIVE[num_beehive].sound_int);  // sound
     strcat( INSERT_SQL_temporary ,stn_out);          
     
     Serial.println(INSERT_SQL_temporary);    
  }
  else
     Serial.print("ERROR creating database string"); 
  
  Serial.println("---");

  // Initiate the query class instance
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  // Execute the query
  cur_mem->execute(INSERT_SQL_temporary);
  // Note: since there are no results, we do not need to read any data
  // Deleting the cursor also frees up memory used
  delete cur_mem;

}


void testMysql(){
  Serial.println("Recording data");

  // Initiate the query class instance
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  // Execute the query
  cur_mem->execute(INSERT_SQL);
  // Note: since there are no results, we do not need to read any data
  // Deleting the cursor also frees up memory used
  delete cur_mem;
}
